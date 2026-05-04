/**
 * @file nrf24l01_ag.h
 * @defgroup nrf24l01_ag nRF24L01 Transmitter Driver
 * @{
 *
 * @brief Hardware-agnostic transmitter driver for the Nordic nRF24L01/nRF24L01+
 *        2.4 GHz radio module.
 *
 * The driver is portable across MCU platforms: SPI communication and GPIO pin
 * control are injected at runtime via function pointers, so no platform-specific
 * code lives inside the library itself.
 *
 * <b>Fixed compile-time defaults:</b>
 * | Parameter         | Value                 |
 * |-------------------|-----------------------|
 * | RF Channel        | 76  (2476 MHz)        |
 * | TX Output Power   | 0 dBm (maximum)       |
 * | Air Data Rate     | 1 Mbps                |
 * | CRC               | 2-byte                |
 * | Auto-Acknowledge  | Enabled on pipe 0     |
 * | Max Retransmits   | 15                    |
 * | Auto-Retry Delay  | 500 µs                |
 * | Address Width     | 5 bytes               |
 * | Static Payload    | 4 bytes               |
 *
 * All defaults can be overridden by populating @ref nrf24l01_ag_config_t
 * manually before calling @ref nrf24l01_ag_init().
 *
 * @par Prerequisites
 * - `_XTAL_FREQ` must be defined (e.g. `#define _XTAL_FREQ 32000000UL`) and
 *   `<xc.h>` must be included **before** this header when using the default
 *   delay macros.  Override @ref NRF24L01_AG_DELAY_MS / @ref NRF24L01_AG_DELAY_US
 *   to remove the XC8 dependency.
 * - `SPI2_Open(MSSP2_DEFAULT)` must be called before any library function.
 * - The CSN pin must idle **high** (deasserted) before calling
 *   @ref nrf24l01_ag_init().
 *
 * @par Example — PIC16F18345, MCC-generated SPI2
 * @code
 * #define _XTAL_FREQ  32000000UL
 * #include <xc.h>
 * #include "nrf24l01_ag.h"
 * #include "mcc_generated_files/system/system.h"
 * #include "mcc_generated_files/spi/mssp2.h"
 * #include "mcc_generated_files/system/pins.h"
 *
 * // -----------------------------------------------------------------
 * // Hardware wrappers — adapt to your MCU/SPI driver
 * // -----------------------------------------------------------------
 *
 * // Maps the driver's separate (tx, rx, len) buffers to SPI2_ByteExchange.
 * // tx == NULL  → send 0xFF (dummy byte, discards received data not needed).
 * // rx == NULL  → received byte is discarded.
 * static void spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len) {
 *     for (size_t i = 0; i < len; i++) {
 *         uint8_t byte = SPI2_ByteExchange((tx != NULL) ? tx[i] : 0xFFu);
 *         if (rx != NULL) rx[i] = byte;
 *     }
 * }
 *
 * // CSN: active-low chip select (false = assert low, true = deassert high).
 * static void csn_set(bool level) {
 *     if (level) NRF_CSN_SetHigh(); else NRF_CSN_SetLow();
 * }
 *
 * // CE: chip enable for TX (true = high starts transmission).
 * static void ce_set(bool level) {
 *     if (level) NRF_CE_SetHigh(); else NRF_CE_SetLow();
 * }
 *
 * // PWR_EN: hardware power gate via Q2 (pass NULL if not fitted).
 * static void pwr_set(bool level) {
 *     if (level) PWR_EN_SetHigh(); else PWR_EN_SetLow();
 * }
 *
 * // -----------------------------------------------------------------
 * // Application
 * // -----------------------------------------------------------------
 *
 * int main(void) {
 *     SYSTEM_Initialize();
 *     SPI2_Open(MSSP2_DEFAULT);
 *
 *     // Device instance: inject hardware functions.
 *     nrf24l01_ag_dev_t radio =
 *         NRF24L01_AG_DEV_INIT(spi_transfer, csn_set, ce_set, pwr_set);
 *
 *     // Configuration: channel 76, max power, 1 Mbps, custom address.
 *     nrf24l01_ag_config_t cfg =
 *         NRF24L01_AG_CONFIG_DEFAULT(0x12, 0x34, 0x56, 0x78, 0x9A);
 *
 *     nrf24l01_ag_init(&radio, &cfg);
 *
 *     uint32_t counter = 0;
 *     while (1) {
 *         counter++;
 *         uint8_t payload[4] = {
 *             (uint8_t)(counter >> 24),
 *             (uint8_t)(counter >> 16),
 *             (uint8_t)(counter >>  8),
 *             (uint8_t)(counter)
 *         };
 *         nrf24l01_ag_result_t r = nrf24l01_ag_transmit(&radio, payload, sizeof(payload));
 *         (void)r;  // handle NRF24L01_AG_OK / MAX_RT / TIMEOUT as needed
 *
 *         nrf24l01_ag_sleep(&radio);   // power down between transmissions
 *         // ... MCU enters SLEEP here ...
 *         nrf24l01_ag_wake(&radio);    // restore radio before next transmission
 *     }
 * }
 * @endcode
 *
 * @version 1.0.0
 * @date    2026-05-04
 */

#ifndef NRF24L01_AG_H
#define NRF24L01_AG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* =========================================================================
 * Version
 * ========================================================================= */

/** @brief Library major version number. */
#define NRF24L01_AG_VERSION_MAJOR  1u
/** @brief Library minor version number. */
#define NRF24L01_AG_VERSION_MINOR  0u
/** @brief Library patch version number. */
#define NRF24L01_AG_VERSION_PATCH  0u

/* =========================================================================
 * Delay macros — override before including this header to remove XC8
 * dependency (requires _XTAL_FREQ and <xc.h> in the default form).
 * ========================================================================= */

/**
 * @brief Blocking millisecond delay.
 * @note  Default implementation uses XC8 `__delay_ms()`.  Override by
 *        defining this macro before including the header:
 * @code
 * #define NRF24L01_AG_DELAY_MS(x)  my_delay_ms(x)
 * #include "nrf24l01_ag.h"
 * @endcode
 */
#ifndef NRF24L01_AG_DELAY_MS
#define NRF24L01_AG_DELAY_MS(x)   __delay_ms(x)
#endif

/**
 * @brief Blocking microsecond delay.
 * @note  Same override rules as @ref NRF24L01_AG_DELAY_MS.
 */
#ifndef NRF24L01_AG_DELAY_US
#define NRF24L01_AG_DELAY_US(x)   __delay_us(x)
#endif

/* =========================================================================
 * nRF24L01 register addresses (5-bit, ORed with command)
 * ========================================================================= */

/** @name Register Addresses
 *  @{ */
#define NRF24L01_AG_REG_CONFIG          0x00u  /**< Configuration register */
#define NRF24L01_AG_REG_EN_AA           0x01u  /**< Enhanced ShockBurst auto-ACK */
#define NRF24L01_AG_REG_EN_RXADDR       0x02u  /**< Enabled RX pipe addresses */
#define NRF24L01_AG_REG_SETUP_AW        0x03u  /**< Address widths */
#define NRF24L01_AG_REG_SETUP_RETR      0x04u  /**< Auto retransmit config */
#define NRF24L01_AG_REG_RF_CH           0x05u  /**< RF channel (0–125) */
#define NRF24L01_AG_REG_RF_SETUP        0x06u  /**< RF data rate and TX power */
#define NRF24L01_AG_REG_STATUS          0x07u  /**< Status register */
#define NRF24L01_AG_REG_OBSERVE_TX      0x08u  /**< TX observe (lost/retransmit count) */
#define NRF24L01_AG_REG_RPD             0x09u  /**< Received power detector */
#define NRF24L01_AG_REG_RX_ADDR_P0     0x0Au  /**< RX pipe 0 address (5 bytes) */
#define NRF24L01_AG_REG_RX_ADDR_P1     0x0Bu  /**< RX pipe 1 address (5 bytes) */
#define NRF24L01_AG_REG_TX_ADDR         0x10u  /**< TX address (5 bytes, LSB first) */
#define NRF24L01_AG_REG_RX_PW_P0       0x11u  /**< Payload width for pipe 0 (1–32) */
#define NRF24L01_AG_REG_FIFO_STATUS     0x17u  /**< TX/RX FIFO status */
#define NRF24L01_AG_REG_DYNPD           0x1Cu  /**< Dynamic payload enable per pipe */
#define NRF24L01_AG_REG_FEATURE         0x1Du  /**< Feature register */
/** @} */

/* =========================================================================
 * SPI command bytes
 * ========================================================================= */

/** @name SPI Commands
 *  @{ */
#define NRF24L01_AG_CMD_R_REGISTER      0x00u  /**< Read register  (OR with 5-bit addr) */
#define NRF24L01_AG_CMD_W_REGISTER      0x20u  /**< Write register (OR with 5-bit addr) */
#define NRF24L01_AG_CMD_R_RX_PAYLOAD    0x61u  /**< Read RX payload (1–32 bytes) */
#define NRF24L01_AG_CMD_W_TX_PAYLOAD    0xA0u  /**< Write TX payload (1–32 bytes) */
#define NRF24L01_AG_CMD_FLUSH_TX        0xE1u  /**< Flush TX FIFO */
#define NRF24L01_AG_CMD_FLUSH_RX        0xE2u  /**< Flush RX FIFO */
#define NRF24L01_AG_CMD_REUSE_TX_PL     0xE3u  /**< Reuse last transmitted payload */
#define NRF24L01_AG_CMD_NOP             0xFFu  /**< No operation — returns STATUS byte */
/** @} */

/* =========================================================================
 * CONFIG register bit masks (address 0x00)
 * ========================================================================= */

/** @name CONFIG Register Bits
 *  @{ */
#define NRF24L01_AG_CONFIG_MASK_RX_DR   0x40u  /**< Mask RX_DR interrupt on IRQ pin */
#define NRF24L01_AG_CONFIG_MASK_TX_DS   0x20u  /**< Mask TX_DS interrupt on IRQ pin */
#define NRF24L01_AG_CONFIG_MASK_MAX_RT  0x10u  /**< Mask MAX_RT interrupt on IRQ pin */
#define NRF24L01_AG_CONFIG_EN_CRC       0x08u  /**< Enable CRC (forced if EN_AA != 0) */
#define NRF24L01_AG_CONFIG_CRCO         0x04u  /**< CRC scheme: 0 = 1-byte, 1 = 2-byte */
#define NRF24L01_AG_CONFIG_PWR_UP       0x02u  /**< Power up (1 = active, 0 = power down) */
#define NRF24L01_AG_CONFIG_PRIM_RX      0x01u  /**< RX/TX role: 1 = RX, 0 = TX */
/** @} */

/* =========================================================================
 * STATUS register bit masks (address 0x07)
 * ========================================================================= */

/** @name STATUS Register Bits
 *  @{ */
#define NRF24L01_AG_STATUS_RX_DR        0x40u  /**< RX data ready (write 1 to clear) */
#define NRF24L01_AG_STATUS_TX_DS        0x20u  /**< TX data sent — ACK received (write 1 to clear) */
#define NRF24L01_AG_STATUS_MAX_RT       0x10u  /**< Max retransmits reached (write 1 to clear) */
#define NRF24L01_AG_STATUS_RX_P_NO_MSK 0x0Eu  /**< Pipe number of received payload */
#define NRF24L01_AG_STATUS_TX_FULL      0x01u  /**< TX FIFO full */
/** @} */

/* =========================================================================
 * RF_SETUP register bit masks (address 0x06)
 * ========================================================================= */

/** @name RF_SETUP Register Bits
 *  @{ */
#define NRF24L01_AG_RF_CONT_WAVE        0x80u  /**< Continuous carrier transmit (test only) */
#define NRF24L01_AG_RF_DR_LOW           0x20u  /**< Set 250 kbps when RF_DR_HIGH = 0 */
#define NRF24L01_AG_RF_DR_HIGH          0x08u  /**< Set 2 Mbps when RF_DR_LOW = 0 */
/** @} */

/**
 * @name TX Output Power Options (RF_SETUP bits [2:1])
 * @{ */
#define NRF24L01_AG_RF_PWR_N18DBM       0x00u  /**< -18 dBm */
#define NRF24L01_AG_RF_PWR_N12DBM       0x02u  /**< -12 dBm */
#define NRF24L01_AG_RF_PWR_N6DBM        0x04u  /**<  -6 dBm */
#define NRF24L01_AG_RF_PWR_0DBM         0x06u  /**<   0 dBm — maximum output power */
/** @} */

/**
 * @name Air Data Rate Combinations (RF_DR_LOW | RF_DR_HIGH)
 * Apply to RF_SETUP by OR-ing with the desired power level.
 * @{ */
#define NRF24L01_AG_DR_1MBPS            0x00u  /**< 1 Mbps  (RF_DR_LOW=0, RF_DR_HIGH=0) */
#define NRF24L01_AG_DR_2MBPS            0x08u  /**< 2 Mbps  (RF_DR_LOW=0, RF_DR_HIGH=1) */
#define NRF24L01_AG_DR_250KBPS          0x20u  /**< 250 kbps (nRF24L01+ only; RF_DR_LOW=1) */
/** @} */

/* =========================================================================
 * FIFO_STATUS register bit masks (address 0x17)
 * ========================================================================= */

/** @name FIFO_STATUS Register Bits
 *  @{ */
#define NRF24L01_AG_FIFO_TX_REUSE       0x40u  /**< Reuse last transmitted payload */
#define NRF24L01_AG_FIFO_TX_FULL        0x20u  /**< TX FIFO full */
#define NRF24L01_AG_FIFO_TX_EMPTY       0x10u  /**< TX FIFO empty */
#define NRF24L01_AG_FIFO_RX_FULL        0x02u  /**< RX FIFO full */
#define NRF24L01_AG_FIFO_RX_EMPTY       0x01u  /**< RX FIFO empty */
/** @} */

/* =========================================================================
 * SETUP_RETR register helpers (address 0x04)
 * Upper nibble = ARD (Auto Retransmit Delay), lower nibble = ARC (count).
 * ========================================================================= */

/** @name Auto Retransmit Delay (SETUP_RETR bits [7:4])
 *  Step = 250 µs.  Minimum recommended value for 250 kbps = 0x60 (1750 µs).
 *  @{ */
#define NRF24L01_AG_RETR_ARD_250US      0x00u  /**< ARD = 250 µs  */
#define NRF24L01_AG_RETR_ARD_500US      0x10u  /**< ARD = 500 µs  */
#define NRF24L01_AG_RETR_ARD_750US      0x20u  /**< ARD = 750 µs  */
#define NRF24L01_AG_RETR_ARD_1000US     0x30u  /**< ARD = 1000 µs */
#define NRF24L01_AG_RETR_ARD_2000US     0x70u  /**< ARD = 2000 µs */
#define NRF24L01_AG_RETR_ARD_4000US     0xF0u  /**< ARD = 4000 µs */
/** @} */

/** @name Auto Retransmit Count (SETUP_RETR bits [3:0])
 *  @{ */
#define NRF24L01_AG_RETR_ARC_DISABLE    0x00u  /**< Retransmit disabled */
#define NRF24L01_AG_RETR_ARC_3          0x03u  /**< Up to  3 retransmits */
#define NRF24L01_AG_RETR_ARC_15         0x0Fu  /**< Up to 15 retransmits (maximum) */
/** @} */

/* =========================================================================
 * SETUP_AW: address width (address 0x03)
 * ========================================================================= */

/** @name Address Width Options
 *  @{ */
#define NRF24L01_AG_AW_3BYTES           0x01u  /**< 3-byte address */
#define NRF24L01_AG_AW_4BYTES           0x02u  /**< 4-byte address */
#define NRF24L01_AG_AW_5BYTES           0x03u  /**< 5-byte address (recommended) */
/** @} */

/* =========================================================================
 * Default configuration constants
 * ========================================================================= */

/** @name Default Configuration Values
 *  @{ */
#define NRF24L01_AG_DEFAULT_CHANNEL     76u    /**< RF channel index → 2476 MHz */
#define NRF24L01_AG_DEFAULT_RF_SETUP \
    (NRF24L01_AG_DR_1MBPS | NRF24L01_AG_RF_PWR_0DBM) /**< 1 Mbps, 0 dBm */
#define NRF24L01_AG_DEFAULT_RETR \
    (NRF24L01_AG_RETR_ARD_500US | NRF24L01_AG_RETR_ARC_15) /**< 500 µs, 15 retries */
#define NRF24L01_AG_ADDR_WIDTH          5u     /**< Fixed address width in bytes */
#define NRF24L01_AG_MAX_PAYLOAD_WIDTH   32u    /**< Maximum static payload size (bytes) */
#define NRF24L01_AG_DEFAULT_PAYLOAD_W   4u     /**< Default payload size (bytes) */
/** @} */

/* =========================================================================
 * Timing constants (datasheet values with safe margin)
 * ========================================================================= */

/** @name Timing Constants
 *  @{ */
#define NRF24L01_AG_POWER_ON_RESET_MS   100u   /**< tPOR: wait after hardware power-up (ms) */
#define NRF24L01_AG_STBY_ENTRY_MS       2u     /**< tpd2stby: wait after PWR_UP bit set (ms) */
#define NRF24L01_AG_CE_PULSE_US         15u    /**< CE high pulse to start TX (µs; min = 10 µs) */
/** @} */

/* =========================================================================
 * Internal config value assembled for CONFIG register
 * (TX mode, 2-byte CRC, power up — used internally by init/wake)
 * ========================================================================= */
#define NRF24L01_AG_CFG_TX_ACTIVE \
    (NRF24L01_AG_CONFIG_EN_CRC | NRF24L01_AG_CONFIG_CRCO | NRF24L01_AG_CONFIG_PWR_UP)

#define NRF24L01_AG_CFG_TX_PWRDOWN \
    (NRF24L01_AG_CONFIG_EN_CRC | NRF24L01_AG_CONFIG_CRCO)

/* =========================================================================
 * Function pointer types
 * ========================================================================= */

/**
 * @brief SPI full-duplex transfer function type.
 *
 * The registered function must exchange exactly @p len bytes over SPI:
 * - Transmit from @p tx_buf (send 0xFF if @p tx_buf is NULL).
 * - Store received bytes into @p rx_buf (discard if @p rx_buf is NULL).
 *
 * CSN (chip-select) management is handled by the driver; **do not** assert or
 * deassert CSN inside this function.
 *
 * @param tx_buf  Transmit buffer, or NULL for dummy bytes.
 * @param rx_buf  Receive buffer, or NULL if received data is not needed.
 * @param len     Number of bytes to transfer.
 */
typedef void (*nrf24l01_ag_spi_fn_t)(const uint8_t *tx_buf, uint8_t *rx_buf, size_t len);

/**
 * @brief GPIO pin control function type.
 *
 * @param level  `true`  → drive pin HIGH.
 *               `false` → drive pin LOW.
 */
typedef void (*nrf24l01_ag_pin_fn_t)(bool level);

/* =========================================================================
 * Result / return codes
 * ========================================================================= */

/**
 * @brief Return codes for @ref nrf24l01_ag_transmit().
 */
typedef enum {
    NRF24L01_AG_OK      = 0,  /**< Transmission succeeded; ACK received. */
    NRF24L01_AG_MAX_RT  = 1,  /**< Maximum retransmit count reached — no ACK. */
    NRF24L01_AG_TIMEOUT = 2,  /**< STATUS polling timed out (SPI/radio issue). */
    NRF24L01_AG_ERR     = 3   /**< Invalid argument (NULL device pointer). */
} nrf24l01_ag_result_t;

/* =========================================================================
 * Configuration structure
 * ========================================================================= */

/**
 * @brief Radio configuration parameters.
 *
 * Populate this structure and pass it to @ref nrf24l01_ag_init().
 * Use @ref NRF24L01_AG_CONFIG_DEFAULT to start from the recommended defaults.
 */
typedef struct {
    uint8_t rf_channel;                       /**< RF channel index 0–125. Frequency = 2400 + rf_channel MHz. */
    uint8_t rf_setup;                         /**< RF_SETUP register value. Combine an @ref NRF24L01_AG_DR_ constant with an @ref NRF24L01_AG_RF_PWR_ constant. */
    uint8_t setup_retr;                       /**< SETUP_RETR register value. Combine an @ref NRF24L01_AG_RETR_ARD_ with an @ref NRF24L01_AG_RETR_ARC_ constant. */
    uint8_t payload_width;                    /**< Static payload width in bytes (1–32). Must match receiver. */
    uint8_t tx_addr[NRF24L01_AG_ADDR_WIDTH];  /**< 5-byte TX address, LSB first. Must match receiver RX address. */
} nrf24l01_ag_config_t;

/* =========================================================================
 * Device structure
 * ========================================================================= */

/**
 * @brief nRF24L01 device instance.
 *
 * Allocate one instance per physical radio module.  Initialise with
 * @ref NRF24L01_AG_DEV_INIT, then call @ref nrf24l01_ag_init().
 *
 * @note The @p config field is written by @ref nrf24l01_ag_init() and used
 *       internally by @ref nrf24l01_ag_wake().  Do not modify it directly
 *       after initialisation.
 */
typedef struct {
    nrf24l01_ag_spi_fn_t spi_transfer;  /**< SPI transfer function (must not be NULL). */
    nrf24l01_ag_pin_fn_t csn_set;       /**< CSN pin function — false = assert (LOW), true = deassert (HIGH). */
    nrf24l01_ag_pin_fn_t ce_set;        /**< CE pin function  — true = HIGH (TX start), false = LOW (standby). */
    nrf24l01_ag_pin_fn_t pwr_set;       /**< Hardware power-enable function.  Set to NULL if not used. */
    nrf24l01_ag_config_t config;        /**< Configuration copy stored during nrf24l01_ag_init(). */
} nrf24l01_ag_dev_t;

/* =========================================================================
 * Convenience initialisation macros
 * ========================================================================= */

/**
 * @brief Static initialiser for @ref nrf24l01_ag_dev_t.
 *
 * Injects hardware function pointers and zero-initialises the stored config.
 * Call @ref nrf24l01_ag_init() afterwards to configure the radio.
 *
 * @param _spi  SPI transfer function  (@ref nrf24l01_ag_spi_fn_t).
 * @param _csn  CSN pin function       (@ref nrf24l01_ag_pin_fn_t).
 * @param _ce   CE  pin function       (@ref nrf24l01_ag_pin_fn_t).
 * @param _pwr  Power-enable function  (@ref nrf24l01_ag_pin_fn_t), or NULL.
 *
 * @par Usage
 * @code
 * nrf24l01_ag_dev_t radio = NRF24L01_AG_DEV_INIT(spi_transfer, csn_set, ce_set, pwr_set);
 * @endcode
 */
#define NRF24L01_AG_DEV_INIT(_spi, _csn, _ce, _pwr) \
{                                                     \
    .spi_transfer = (_spi),                           \
    .csn_set      = (_csn),                           \
    .ce_set       = (_ce),                            \
    .pwr_set      = (_pwr),                           \
    .config       = { 0u, 0u, 0u, 0u, { 0u } }       \
}

/**
 * @brief Static initialiser for @ref nrf24l01_ag_config_t with defaults.
 *
 * Applies the recommended defaults (channel 76, 0 dBm, 1 Mbps, 2-byte CRC,
 * 15 retries, 500 µs ARD, 4-byte payload) with a user-supplied TX address.
 *
 * @param a0 Address byte 0 (LSB, sent first over the air).
 * @param a1 Address byte 1.
 * @param a2 Address byte 2.
 * @param a3 Address byte 3.
 * @param a4 Address byte 4 (MSB).
 *
 * @par Usage
 * @code
 * nrf24l01_ag_config_t cfg = NRF24L01_AG_CONFIG_DEFAULT(0x12, 0x34, 0x56, 0x78, 0x9A);
 * @endcode
 */
#define NRF24L01_AG_CONFIG_DEFAULT(a0, a1, a2, a3, a4) \
{                                                        \
    .rf_channel    = NRF24L01_AG_DEFAULT_CHANNEL,        \
    .rf_setup      = NRF24L01_AG_DEFAULT_RF_SETUP,       \
    .setup_retr    = NRF24L01_AG_DEFAULT_RETR,           \
    .payload_width = NRF24L01_AG_DEFAULT_PAYLOAD_W,      \
    .tx_addr       = { (a0), (a1), (a2), (a3), (a4) }   \
}

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Initialise the nRF24L01 as a transmitter.
 *
 * Performs the full power-up and configuration sequence:
 * - Optionally enables hardware power via @p dev->pwr_set (if not NULL) and
 *   waits @ref NRF24L01_AG_POWER_ON_RESET_MS for the radio to stabilise.
 * - Writes all configuration registers (channel, RF setup, address, retransmit,
 *   payload width, auto-ACK on pipe 0).
 * - Powers up the radio in PTX (transmitter) mode.
 * - Waits @ref NRF24L01_AG_STBY_ENTRY_MS for the oscillator to stabilise
 *   (tpd2stby per datasheet).
 *
 * On return the radio is in **Standby-I** mode, ready for @ref nrf24l01_ag_transmit().
 *
 * @param dev  Pointer to the device instance (must not be NULL).
 * @param cfg  Pointer to the configuration to apply (must not be NULL).
 *             A copy is stored inside @p dev for use by @ref nrf24l01_ag_wake().
 *
 * @note SPI must be open (@c SPI2_Open()) before calling this function.
 * @note CSN must be idle-high before this call.
 */
void nrf24l01_ag_init(nrf24l01_ag_dev_t *dev, const nrf24l01_ag_config_t *cfg);

/**
 * @brief Transmit a payload and wait for the result.
 *
 * Loads @p len bytes from @p payload into the TX FIFO, pulses CE for
 * @ref NRF24L01_AG_CE_PULSE_US microseconds to initiate transmission, then
 * polls the STATUS register until the radio signals TX_DS (sent + ACK) or
 * MAX_RT (max retransmits reached).
 *
 * The STATUS flags are cleared before returning.  If MAX_RT is set, the TX
 * FIFO is flushed automatically.
 *
 * On return the radio is back in **Standby-I** mode (CE low).
 *
 * @param dev      Pointer to the device instance (must not be NULL).
 * @param payload  Buffer containing the bytes to transmit (must not be NULL).
 * @param len      Number of bytes to send.  Must match @p dev->config.payload_width
 *                 and the receiver's expected payload size (1–32 bytes).
 *
 * @return @ref NRF24L01_AG_OK      — transmission successful, ACK received.
 * @return @ref NRF24L01_AG_MAX_RT  — no ACK after maximum retransmits.
 * @return @ref NRF24L01_AG_TIMEOUT — STATUS register did not update in time.
 * @return @ref NRF24L01_AG_ERR     — NULL pointer passed.
 *
 * @warning If @p len differs from the receiver's configured payload width the
 *          payload will be silently truncated or padded at the receiver.
 */
nrf24l01_ag_result_t nrf24l01_ag_transmit(nrf24l01_ag_dev_t *dev,
                                           const uint8_t *payload,
                                           uint8_t len);

/**
 * @brief Put the radio into power-down mode.
 *
 * Clears the PWR_UP bit in CONFIG (hardware SPI power-down) and, if
 * @p dev->pwr_set is not NULL, drives the power-enable pin LOW to cut
 * hardware power to the module via the external power gate (Q2).
 *
 * Current consumption in power-down: 900 nA (register values retained).
 * When hardware power is cut the radio must be fully re-initialised with
 * @ref nrf24l01_ag_wake() before the next transmission.
 *
 * @param dev  Pointer to the device instance (must not be NULL).
 */
void nrf24l01_ag_sleep(nrf24l01_ag_dev_t *dev);

/**
 * @brief Wake the radio and restore transmitter configuration.
 *
 * If @p dev->pwr_set is not NULL, drives the power-enable pin HIGH and waits
 * @ref NRF24L01_AG_POWER_ON_RESET_MS for the radio hardware to boot.
 * Then replays the full initialisation sequence using the configuration
 * stored by the previous @ref nrf24l01_ag_init() call.
 *
 * On return the radio is in **Standby-I** mode, ready for transmission.
 *
 * @param dev  Pointer to the device instance (must not be NULL).
 *
 * @note This function re-uses the configuration saved during the last call to
 *       @ref nrf24l01_ag_init().  To change configuration between power cycles
 *       call @ref nrf24l01_ag_init() instead.
 */
void nrf24l01_ag_wake(nrf24l01_ag_dev_t *dev);

/**
 * @brief Read and return the current STATUS register value.
 *
 * Sends a NOP command and captures the STATUS byte that the nRF24L01 clocks
 * out on every SPI transaction.  Useful for debugging.
 *
 * @param dev  Pointer to the device instance (must not be NULL).
 * @return     Raw STATUS register byte.
 *             Test with @ref NRF24L01_AG_STATUS_TX_DS, @ref NRF24L01_AG_STATUS_MAX_RT, etc.
 */
uint8_t nrf24l01_ag_get_status(nrf24l01_ag_dev_t *dev);

/**
 * @brief Flush the TX FIFO.
 *
 * Issues a FLUSH_TX command.  Call this if @ref nrf24l01_ag_transmit() returns
 * @ref NRF24L01_AG_TIMEOUT and you suspect a stale payload is blocking the FIFO.
 *
 * @param dev  Pointer to the device instance (must not be NULL).
 */
void nrf24l01_ag_flush_tx(nrf24l01_ag_dev_t *dev);

/** @} */ /* end of nrf24l01_ag group */

#endif /* NRF24L01_AG_H */
