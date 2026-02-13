// ... existing code ...

#include "nrf24l01.h"
#include <stdint.h>

// Internal helper: Read nRF24L01 register
static uint8_t nrf24_reg_read(nrf24l01_device_t *dev, uint8_t reg) {
    uint8_t out[2] = {NRF24_CMD_READ_REG | (reg & 0x1F), 0xFF};
    uint8_t in[2];
    dev->csn_set(false);
    dev->spi_transfer(out, in, 2);
    dev->csn_set(true);
    return in[1];
}

// Internal helper: Write nRF24L01 register
static void nrf24_reg_write(nrf24l01_device_t *dev, uint8_t reg, uint8_t value) {
    uint8_t out[2] = {NRF24_CMD_WRITE_REG | (reg & 0x1F), value};
    uint8_t in[2];
    dev->csn_set(false);
    dev->spi_transfer(out, in, 2);
    dev->csn_set(true);
}

// 1. Device Initialization
void nrf24l01_init(nrf24l01_device_t *dev) {
    // Example config: power up, set RX mode, etc. Add more as needed.
    // Flush RX/TX FIFO
    dev->csn_set(false);
    uint8_t cmd_flush_rx = NRF24_CMD_FLUSH_RX;
    dev->spi_transfer(&cmd_flush_rx, NULL, 1);
    dev->csn_set(true);

    dev->csn_set(false);
    uint8_t cmd_flush_tx = NRF24_CMD_FLUSH_TX;
    dev->spi_transfer(&cmd_flush_tx, NULL, 1);
    dev->csn_set(true);

    // Set CONFIG register: power up, RX mode
    nrf24_reg_write(dev, NRF24_REG_CONFIG, NRF24_CONFIG_PWR_UP | NRF24_CONFIG_PRIM_RX);

    // CE high to enter receive mode
    dev->ce_set(true);
}

// 2. Retrieve amount of available data
uint8_t nrf24l01_data_available(nrf24l01_device_t *dev) {
    uint8_t fifo_status = nrf24_reg_read(dev, NRF24_REG_FIFO_STATUS);
    // RX FIFO is empty if bit 0 is set
    return (fifo_status & NRF24_FIFO_RX_EMPTY) ? 0 : 1; // 1 indicates at least one pipe has data
}

// 3. Read out available data, return amount received
uint8_t nrf24l01_read_data(nrf24l01_device_t *dev, uint8_t *rx_buf, uint8_t max_len) {
    uint8_t status = nrf24_reg_read(dev, NRF24_REG_STATUS);
    // Payload read
    dev->csn_set(false);
    uint8_t out[1] = {NRF24_CMD_READ_PAYLOAD};
    uint8_t *in = rx_buf;
    dev->spi_transfer(out, NULL, 1); // send command
    dev->spi_transfer(NULL, in, max_len); // receive payload
    dev->csn_set(true);

    // Clear RX_DR bit (write 1 to STATUS to reset)
    nrf24_reg_write(dev, NRF24_REG_STATUS, status | 0x40);

    // Return actual payload size (user must know RX payload width or get from device setup)
    return max_len; // Replace with actual payload size as needed
}

// 4. Set device to sleep mode
void nrf24l01_sleep(nrf24l01_device_t *dev) {
    uint8_t config = nrf24_reg_read(dev, NRF24_REG_CONFIG);
    config &= ~NRF24_CONFIG_PWR_UP;
    nrf24_reg_write(dev, NRF24_REG_CONFIG, config);
    dev->ce_set(false);
}

// 5. Wake up device from sleep mode
void nrf24l01_wake(nrf24l01_device_t *dev) {
    uint8_t config = nrf24_reg_read(dev, NRF24_REG_CONFIG);
    config |= NRF24_CONFIG_PWR_UP;
    nrf24_reg_write(dev, NRF24_REG_CONFIG, config);
    // Wait tpd2stby per datasheet (~1.5ms)
    // User should implement delay after this call
    dev->ce_set(true);
}

uint8_t nrf24l01_spi_transfer_fn_register(nrf24l01_device_t *dev, spi_transfer_fn *fn){
    if (dev == NULL || fn == NULL) {
        return 0; // Failure
    }
    dev->spi_transfer = *fn;
    return 1; // Success
}

uint8_t nrf24l01_pin_set_fn_csn_register(nrf24l01_device_t *dev, pin_set_fn *fn){
    if (dev == NULL || fn == NULL) {
        return 0; // Failure
    }
    dev->csn_set = *fn;
    return 1; // Success
}

uint8_t nrf24l01_pin_set_fn_ce_register(nrf24l01_device_t *dev, pin_set_fn *fn){
    if (dev == NULL || fn == NULL) {
        return 0; // Failure
    }
    dev->ce_set = *fn; 
    return 1; // Success
}


// ... end of source ...