/**
 * @file nrf24l01_ag.c
 * @ingroup nrf24l01_ag
 *
 * @brief Implementation of the nRF24L01 transmitter driver.
 *
 * All SPI transactions follow the nRF24L01(+) Product Specification v1.0
 * protocol:
 *  - CSN is asserted (LOW) for the duration of a complete command.
 *  - The first byte is always the command/register byte; data follows.
 *  - STATUS is returned on MISO during every command byte.
 *
 * Internal helper functions are grouped between @cond INTERNAL / @endcond
 * Doxygen tags so they do not appear in the public API documentation.
 *
 * @version 1.0.0
 * @date    2026-05-04
 */

/* The host application must define _XTAL_FREQ and include <xc.h> before this
 * translation unit if using the default NRF24L01_AG_DELAY_MS/US macros. */
#include <xc.h>
#include <stddef.h>
#include <string.h>   /* memcpy */

#include "nrf24l01_ag.h"

/* =========================================================================
 * @cond INTERNAL
 * Internal helper functions — not exposed in the public API.
 * ========================================================================= */

/**
 * @brief Read a single 8-bit register over SPI.
 *
 * Issues R_REGISTER | reg, clocks out one dummy byte (0xFF) and returns the
 * received data byte.  The STATUS byte received during the command phase is
 * discarded.
 *
 * @param dev  Device instance.
 * @param reg  5-bit register address (bits [4:0] used).
 * @return     Register value.
 */
static uint8_t nrf24_read_reg(nrf24l01_ag_dev_t *dev, uint8_t reg)
{
    uint8_t tx[2] = { NRF24L01_AG_CMD_R_REGISTER | (reg & 0x1Fu),
                      NRF24L01_AG_CMD_NOP };
    uint8_t rx[2] = { 0u, 0u };

    dev->csn_set(false);
    dev->spi_transfer(tx, rx, 2u);
    dev->csn_set(true);

    return rx[1];  /* rx[0] = STATUS (discarded), rx[1] = register data */
}

/**
 * @brief Write a single 8-bit register over SPI.
 *
 * Issues W_REGISTER | reg followed by the data byte.
 *
 * @param dev   Device instance.
 * @param reg   5-bit register address.
 * @param value Byte to write.
 */
static void nrf24_write_reg(nrf24l01_ag_dev_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = { NRF24L01_AG_CMD_W_REGISTER | (reg & 0x1Fu), value };

    dev->csn_set(false);
    dev->spi_transfer(tx, NULL, 2u);
    dev->csn_set(true);
}

/**
 * @brief Write multiple bytes to a register (e.g. multi-byte addresses).
 *
 * Issues W_REGISTER | reg, then clocks out @p len bytes from @p data.
 * Used for TX_ADDR (5 bytes) and RX_ADDR_P0 (5 bytes).
 *
 * @param dev   Device instance.
 * @param reg   5-bit register address.
 * @param data  Pointer to the bytes to write.
 * @param len   Number of bytes to write.
 */
static void nrf24_write_reg_mb(nrf24l01_ag_dev_t *dev, uint8_t reg,
                                const uint8_t *data, uint8_t len)
{
    uint8_t cmd = NRF24L01_AG_CMD_W_REGISTER | (reg & 0x1Fu);

    dev->csn_set(false);
    dev->spi_transfer(&cmd, NULL, 1u);      /* command byte */
    dev->spi_transfer(data, NULL, len);     /* address bytes */
    dev->csn_set(true);
}

/**
 * @brief Issue a single-byte command and return the STATUS byte.
 *
 * Suitable for FLUSH_TX, FLUSH_RX, NOP, etc.
 *
 * @param dev  Device instance.
 * @param cmd  Command byte.
 * @return     STATUS register value clocked out during the command byte.
 */
static uint8_t nrf24_command(nrf24l01_ag_dev_t *dev, uint8_t cmd)
{
    uint8_t status = 0u;

    dev->csn_set(false);
    dev->spi_transfer(&cmd, &status, 1u);
    dev->csn_set(true);

    return status;
}

/**
 * @brief Core configuration sequence — shared by init and wake.
 *
 * Writes all radio registers from the supplied configuration.  Does NOT
 * power up the radio; CONFIG with PWR_UP is written by the caller.
 *
 * @param dev  Device instance.
 * @param cfg  Configuration to apply.
 */
static void nrf24_configure(nrf24l01_ag_dev_t *dev,
                             const nrf24l01_ag_config_t *cfg)
{
    /* Flush FIFOs to start from a known state */
    nrf24_command(dev, NRF24L01_AG_CMD_FLUSH_TX);
    nrf24_command(dev, NRF24L01_AG_CMD_FLUSH_RX);

    /* Clear any pending interrupt flags (write 1 to STATUS bits 6:4) */
    nrf24_write_reg(dev, NRF24L01_AG_REG_STATUS,
                    NRF24L01_AG_STATUS_RX_DR |
                    NRF24L01_AG_STATUS_TX_DS |
                    NRF24L01_AG_STATUS_MAX_RT);

    /* Auto-acknowledge: enable on pipe 0 (required for auto-retransmit) */
    nrf24_write_reg(dev, NRF24L01_AG_REG_EN_AA, 0x01u);

    /* Enable RX address pipe 0 (needed to receive the ACK packet) */
    nrf24_write_reg(dev, NRF24L01_AG_REG_EN_RXADDR, 0x01u);

    /* Address width: 5 bytes */
    nrf24_write_reg(dev, NRF24L01_AG_REG_SETUP_AW, NRF24L01_AG_AW_5BYTES);

    /* Auto-retransmit settings (delay + count) */
    nrf24_write_reg(dev, NRF24L01_AG_REG_SETUP_RETR, cfg->setup_retr);

    /* RF channel */
    nrf24_write_reg(dev, NRF24L01_AG_REG_RF_CH, cfg->rf_channel & 0x7Fu);

    /* RF data rate and output power */
    nrf24_write_reg(dev, NRF24L01_AG_REG_RF_SETUP, cfg->rf_setup);

    /* TX address (5 bytes, LSB first) */
    nrf24_write_reg_mb(dev, NRF24L01_AG_REG_TX_ADDR,
                       cfg->tx_addr, NRF24L01_AG_ADDR_WIDTH);

    /* RX pipe 0 address = TX address (mandatory for Enhanced ShockBurst ACK) */
    nrf24_write_reg_mb(dev, NRF24L01_AG_REG_RX_ADDR_P0,
                       cfg->tx_addr, NRF24L01_AG_ADDR_WIDTH);

    /* Static payload width for pipe 0 — clamp to valid range */
    uint8_t pw = cfg->payload_width;
    if (pw < 1u)  pw = 1u;
    if (pw > NRF24L01_AG_MAX_PAYLOAD_WIDTH) pw = NRF24L01_AG_MAX_PAYLOAD_WIDTH;
    nrf24_write_reg(dev, NRF24L01_AG_REG_RX_PW_P0, pw);
}

/* @endcond */

/* =========================================================================
 * Public API implementation
 * ========================================================================= */

/**
 * @brief Initialise the nRF24L01 as a transmitter.
 * @see nrf24l01_ag.h for full parameter and return-value documentation.
 */
void nrf24l01_ag_init(nrf24l01_ag_dev_t *dev, const nrf24l01_ag_config_t *cfg)
{
    if ((dev == NULL) || (cfg == NULL)) {
        return;
    }

    /* Save configuration — reused by nrf24l01_ag_wake() */
    memcpy(&dev->config, cfg, sizeof(nrf24l01_ag_config_t));

    /* Step 1: ensure CE is low (standby / power-down state) */
    dev->ce_set(false);

    /* Step 2: assert CSN high (bus idle) */
    dev->csn_set(true);

    /* Step 3: if a hardware power gate is available, enable it and wait for
     *         the radio's power-on reset to complete (tPOR = 100 ms max). */
    if (dev->pwr_set != NULL) {
        dev->pwr_set(true);
        NRF24L01_AG_DELAY_MS(NRF24L01_AG_POWER_ON_RESET_MS);
    }

    /* Step 4: write CONFIG in power-down state first (PWR_UP = 0) so that
     *         all subsequent writes are accepted by the radio. */
    nrf24_write_reg(dev, NRF24L01_AG_REG_CONFIG, NRF24L01_AG_CFG_TX_PWRDOWN);

    /* Step 5: apply the full register configuration */
    nrf24_configure(dev, cfg);

    /* Step 6: power up the radio in PTX mode (PRIM_RX = 0) */
    nrf24_write_reg(dev, NRF24L01_AG_REG_CONFIG, NRF24L01_AG_CFG_TX_ACTIVE);

    /* Step 7: wait for crystal oscillator start-up (tpd2stby ≥ 1.5 ms) */
    NRF24L01_AG_DELAY_MS(NRF24L01_AG_STBY_ENTRY_MS);

    /* Radio is now in Standby-I: CE low, ready to load and transmit a packet */
}

/**
 * @brief Transmit a payload and wait for the result.
 * @see nrf24l01_ag.h for full parameter and return-value documentation.
 */
nrf24l01_ag_result_t nrf24l01_ag_transmit(nrf24l01_ag_dev_t *dev,
                                           const uint8_t *payload,
                                           uint8_t len)
{
    if ((dev == NULL) || (payload == NULL)) {
        return NRF24L01_AG_ERR;
    }

    /* ------------------------------------------------------------------ */
    /* 1. Load payload into TX FIFO using W_TX_PAYLOAD command             */
    /* ------------------------------------------------------------------ */
    uint8_t cmd = NRF24L01_AG_CMD_W_TX_PAYLOAD;

    dev->csn_set(false);
    dev->spi_transfer(&cmd, NULL, 1u);          /* command byte */
    dev->spi_transfer(payload, NULL, len);      /* payload bytes */
    dev->csn_set(true);

    /* ------------------------------------------------------------------ */
    /* 2. Pulse CE HIGH for ≥10 µs to move radio from Standby-I → TX mode */
    /* ------------------------------------------------------------------ */
    dev->ce_set(true);
    NRF24L01_AG_DELAY_US(NRF24L01_AG_CE_PULSE_US);
    dev->ce_set(false);   /* return to Standby-I after pulse */

    /* ------------------------------------------------------------------ */
    /* 3. Poll STATUS until TX_DS (ACK received) or MAX_RT (failed)        */
    /*                                                                     */
    /* Worst-case time with defaults (15 retries @ 500 µs ARD, 1 Mbps):  */
    /*   15 × (transmission ~0.5 ms + ARD 0.5 ms) ≈ 15 ms                */
    /* 5 000 iterations @ ~3 µs each (SPI read) ≈ 15 ms — safe margin.   */
    /* ------------------------------------------------------------------ */
    uint16_t timeout = 5000u;
    uint8_t  status;

    do {
        status = nrf24_read_reg(dev, NRF24L01_AG_REG_STATUS);
        timeout--;
    } while (!(status & (NRF24L01_AG_STATUS_TX_DS | NRF24L01_AG_STATUS_MAX_RT))
             && (timeout > 0u));

    /* ------------------------------------------------------------------ */
    /* 4. Clear TX_DS and MAX_RT flags (write 1 to clear)                  */
    /* ------------------------------------------------------------------ */
    nrf24_write_reg(dev, NRF24L01_AG_REG_STATUS,
                    NRF24L01_AG_STATUS_TX_DS | NRF24L01_AG_STATUS_MAX_RT);

    /* ------------------------------------------------------------------ */
    /* 5. Evaluate result                                                   */
    /* ------------------------------------------------------------------ */
    if (timeout == 0u) {
        /* STATUS never changed — SPI wiring issue or radio hung */
        nrf24_command(dev, NRF24L01_AG_CMD_FLUSH_TX);
        return NRF24L01_AG_TIMEOUT;
    }

    if (status & NRF24L01_AG_STATUS_MAX_RT) {
        /* ACK was not received after max retransmits; flush the stale payload */
        nrf24_command(dev, NRF24L01_AG_CMD_FLUSH_TX);
        return NRF24L01_AG_MAX_RT;
    }

    /* TX_DS set — transmission confirmed by ACK */
    return NRF24L01_AG_OK;
}

/**
 * @brief Put the radio into power-down mode.
 * @see nrf24l01_ag.h for full parameter documentation.
 */
void nrf24l01_ag_sleep(nrf24l01_ag_dev_t *dev)
{
    if (dev == NULL) {
        return;
    }

    /* Ensure CE is low (exit TX/RX active mode into standby first) */
    dev->ce_set(false);

    /* Clear PWR_UP bit: Standby-I → Power Down (register values retained) */
    nrf24_write_reg(dev, NRF24L01_AG_REG_CONFIG, NRF24L01_AG_CFG_TX_PWRDOWN);

    /* If a hardware power gate is connected, cut power completely.
     * After this point all register contents are lost; nrf24l01_ag_wake()
     * will re-run the full initialisation sequence. */
    if (dev->pwr_set != NULL) {
        dev->pwr_set(false);
    }
}

/**
 * @brief Wake the radio and restore transmitter configuration.
 * @see nrf24l01_ag.h for full parameter documentation.
 */
void nrf24l01_ag_wake(nrf24l01_ag_dev_t *dev)
{
    if (dev == NULL) {
        return;
    }

    /* CE must be low before SPI access */
    dev->ce_set(false);
    dev->csn_set(true);

    /* If hardware power was cut, turn it back on and wait for power-on reset */
    if (dev->pwr_set != NULL) {
        dev->pwr_set(true);
        NRF24L01_AG_DELAY_MS(NRF24L01_AG_POWER_ON_RESET_MS);
    }

    /* Replay the full configuration using the stored config */
    nrf24_write_reg(dev, NRF24L01_AG_REG_CONFIG, NRF24L01_AG_CFG_TX_PWRDOWN);
    nrf24_configure(dev, &dev->config);
    nrf24_write_reg(dev, NRF24L01_AG_REG_CONFIG, NRF24L01_AG_CFG_TX_ACTIVE);
    NRF24L01_AG_DELAY_MS(NRF24L01_AG_STBY_ENTRY_MS);
}

/**
 * @brief Read and return the current STATUS register value.
 * @see nrf24l01_ag.h for full documentation.
 */
uint8_t nrf24l01_ag_get_status(nrf24l01_ag_dev_t *dev)
{
    if (dev == NULL) {
        return 0u;
    }
    /* NOP command: radio clocks out STATUS on MISO while we send 0xFF */
    return nrf24_command(dev, NRF24L01_AG_CMD_NOP);
}

/**
 * @brief Flush the TX FIFO.
 * @see nrf24l01_ag.h for full documentation.
 */
void nrf24l01_ag_flush_tx(nrf24l01_ag_dev_t *dev)
{
    if (dev == NULL) {
        return;
    }
    nrf24_command(dev, NRF24L01_AG_CMD_FLUSH_TX);
}

/* =========================================================================
 * Example usage — see also the @par Example block in nrf24l01_ag.h
 *
 * The snippet below shows a complete integration for the LowPowerCounter
 * project (PIC16F18345, MCC-generated SPI2, reed-switch counter node).
 * It is #if 0 guarded so it never compiles; copy the relevant parts into
 * your own main.c or application file.
 * =========================================================================
 *
 * @example nrf24l01_ag_example.c
 * @brief   TX-only counter node — channel 76, 0 dBm, 1 Mbps.
 */
#if 0  /* ---- EXAMPLE START (not compiled) -------------------------------- */

#define _XTAL_FREQ  32000000UL
#include <xc.h>
#include "nrf24l01_ag.h"
#include "mcc_generated_files/system/system.h"
#include "mcc_generated_files/spi/mssp2.h"
#include "mcc_generated_files/system/pins.h"
#include "mcc_generated_files/power/power.h"
#include "mcc_generated_files/timer/tmr1.h"

/* -----------------------------------------------------------------------
 * Hardware wrappers — adapt pin names to your MCC pin aliases
 * ----------------------------------------------------------------------- */

/**
 * Maps the driver's separate tx/rx buffers to SPI2_ByteExchange.
 * tx == NULL → send 0xFF (dummy).
 * rx == NULL → discard received byte.
 */
static void spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    for (size_t i = 0u; i < len; i++) {
        uint8_t byte = SPI2_ByteExchange((tx != NULL) ? tx[i] : 0xFFu);
        if (rx != NULL) rx[i] = byte;
    }
}

/** CSN: active-low chip-select for nRF24L01. */
static void csn_set(bool level)
{
    if (level) IO_RC2_SetHigh(); else IO_RC2_SetLow();   /* NRF_CSN → RC2 */
}

/** CE: chip-enable — pulse HIGH to start transmission. */
static void ce_set(bool level)
{
    if (level) IO_RC5_SetHigh(); else IO_RC5_SetLow();   /* NRF_CE  → RC5 */
}

/** PWR_EN: hardware power gate (Q2 on the main board). */
static void pwr_set(bool level)
{
    if (level) PWR_EN_SetHigh(); else PWR_EN_SetLow();   /* PWR_EN  → RC7 */
}

/* -----------------------------------------------------------------------
 * Application
 * ----------------------------------------------------------------------- */

/* Unique node identifier — change per physical device */
#define NODE_ID          0x01u

/* Number of reed pulses between radio transmissions */
#define TX_PULSE_INTERVAL  5u

/* 5-byte network address — must match the receiver configuration */
static const uint8_t node_tx_addr[NRF24L01_AG_ADDR_WIDTH] =
    { 0x12u, 0x34u, 0x56u, 0x78u, 0x9Au };

/* Global state */
static nrf24l01_ag_dev_t    g_radio;
static nrf24l01_ag_config_t g_radio_cfg;
static volatile uint8_t     g_reed_event = 0u;
static uint32_t             g_counter    = 0u;
static uint32_t             g_tx_counter = 0u;

/* Reed-switch interrupt callback (external INT on RC0, rising edge) */
void INT_CB(void)
{
    g_reed_event = 1u;
}

int main(void)
{
    SYSTEM_Initialize();
    SPI2_Open(MSSP2_DEFAULT);

    /* Wire hardware functions into the radio instance */
    g_radio = (nrf24l01_ag_dev_t)
        NRF24L01_AG_DEV_INIT(spi_transfer, csn_set, ce_set, pwr_set);

    /* Build configuration: channel 76, 0 dBm, 1 Mbps, 4-byte payload */
    g_radio_cfg = (nrf24l01_ag_config_t)
        NRF24L01_AG_CONFIG_DEFAULT(node_tx_addr[0], node_tx_addr[1],
                                   node_tx_addr[2], node_tx_addr[3],
                                   node_tx_addr[4]);

    /* Set-up external interrupt on RC0 (reed switch) */
    INT_SetInterruptHandler(&INT_CB);
    EXT_INT_InterruptEnable();
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    /* Shut down all peripherals for minimal sleep current */
    POWER_PeripheralDisableAll();
    POWER_PeripheralEnable(POWER_IOC);   /* keep interrupt-on-change alive */

    /* Radio starts off — powered up only during transmission */
    pwr_set(false);

    while (1)
    {
        /* Sleep until the reed switch fires */
        POWER_LowPowerModeEnter();

        if (g_reed_event == 0u) {
            continue;   /* spurious wake */
        }
        g_reed_event = 0u;

        g_counter++;
        g_tx_counter++;

        /* Transmit every TX_PULSE_INTERVAL pulses */
        if (g_tx_counter >= TX_PULSE_INTERVAL)
        {
            g_tx_counter = 0u;

            /* Enable SPI2 peripheral before radio use */
            POWER_PeripheralEnable(POWER_MSSP2);

            /* Full radio initialisation (includes 100 ms power-on wait) */
            nrf24l01_ag_init(&g_radio, &g_radio_cfg);

            /* Pack 32-bit counter big-endian into 4-byte payload */
            uint8_t payload[4] = {
                NODE_ID,                         /* byte 0: node identifier */
                (uint8_t)(g_counter >> 16),      /* byte 1: counter high    */
                (uint8_t)(g_counter >>  8),      /* byte 2: counter mid     */
                (uint8_t)(g_counter)             /* byte 3: counter low     */
            };

            LED_SetHigh();
            nrf24l01_ag_result_t result =
                nrf24l01_ag_transmit(&g_radio, payload, sizeof(payload));
            LED_SetLow();

            (void)result;   /* add error handling / retry logic here */

            /* Power down radio and SPI to minimise sleep current */
            nrf24l01_ag_sleep(&g_radio);
            POWER_PeripheralDisable(POWER_MSSP2);
        }
    }
}

#endif /* ---- EXAMPLE END ------------------------------------------------- */
