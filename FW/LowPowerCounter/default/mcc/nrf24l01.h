#ifndef NRF24L01_H
#define NRF24L01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef void (*spi_transfer_fn)(void *in_out_buff, size_t len);
typedef void (*pin_set_fn)(bool level);

/*********************************************************************
    @def nrf24l01_device_t
    @brief Structure representing an nRF24L01 device instance, including function pointers for SPI
    communication and pin control.
    This structure allows for flexible integration of the nRF24L01 module by abstracting the hardware
    interactions through function pointers. The user must provide implementations for SPI transfer and
    pin control functions, which can be registered using the provided registration functions.
*********************************************************************/
typedef struct {
    spi_transfer_fn spi_transfer;
    pin_set_fn csn_set;
    pin_set_fn ce_set;
    // Optionally include config (addresses, channels, etc.)
} nrf24l01_device_t;

// Utility macros for nRF24L01 commands
#define NRF24_CMD_READ_REG    0x00
#define NRF24_CMD_WRITE_REG   0x20
#define NRF24_CMD_FLUSH_RX    0xE2
#define NRF24_CMD_FLUSH_TX    0xE1
#define NRF24_CMD_READ_PAYLOAD  0x61
#define NRF24_CMD_WRITE_PAYLOAD 0xA0

// Register addresses
#define NRF24_REG_CONFIG      0x00
#define NRF24_REG_STATUS      0x07
#define NRF24_REG_FIFO_STATUS 0x17

// CONFIG register bits
#define NRF24_CONFIG_PWR_UP   0x02
#define NRF24_CONFIG_PRIM_RX  0x01

// FIFO STATUS register bits
#define NRF24_FIFO_RX_EMPTY   0x01

/***************************************************************
    @def spi_transfer_fn_register
    @brief Registers a user-provided SPI transfer function for nRF24L01 communication.
    @param dev Pointer to the nrf24l01_device_t instance to configure.
    @param fn Pointer to the SPI transfer function to register. The function should match the
    spi_transfer_fn type, accepting an output buffer, an input buffer, and the length of
    the transfer.
    @return uint8_t Returns 1 on successful registration, or 0 if the provided
    parameters are invalid (e.g., NULL pointers).
    This function allows the user to provide a custom SPI transfer implementation, enabling
    the nRF24L01 module to communicate with the microcontroller using the user's specific SPI
    setup. The registered function will be called internally by the nRF24L01 driver when SPI
    communication is required.
***************************************************************/
uint8_t nrf24l01_spi_transfer_fn_register(nrf24l01_device_t *dev, spi_transfer_fn fn);

/****************************************************************
    @def pin_set_fn_csn_register
    @brief Registers a user-provided pin control function for the CSN (Chip Select Not) pin of the nRF24L01 module.
    @param dev Pointer to the nrf24l01_device_t instance to configure.
    @param fn Pointer to the pin control function to register. The function should match the
    pin_set_fn type, accepting a boolean level (true for high, false for low).
    @return uint8_t Returns 1 on successful registration, or 0 if the provided
    parameters are invalid (e.g., NULL pointers).
    This function allows the user to provide a custom pin control implementation for the CSN pin, which is
    essential for SPI communication with the nRF24L01 module. The registered function will be called
    internally by the nRF24L01 driver to control the CSN pin during SPI transactions, ensuring proper communication with the radio module.
*****************************************************************/
uint8_t nrf24l01_pin_set_fn_csn_register(nrf24l01_device_t *dev, pin_set_fn fn);

/******************************************************************
    @def pin_set_fn_ce_register
    @brief Registers a user-provided pin control function for the CE (Chip Enable) pin of the nRF24L01 module.
    @param dev Pointer to the nrf24l01_device_t instance to configure.
    @param fn Pointer to the pin control function to register. The function should match the
    pin_set_fn type, accepting a boolean level (true for high, false for low).
    @return uint8_t Returns 1 on successful registration, or 0 if the provided
    parameters are invalid (e.g., NULL pointers).
    This function allows the user to provide a custom pin control implementation for the CE pin, which is
    crucial for controlling the operating mode of the nRF24L01 module (e.g.,
    transmitting or receiving). The registered function will be called internally by the nRF24L01 driver to control the CE pin as needed during operation.
******************************************************************/
uint8_t nrf24l01_pin_set_fn_ce_register(nrf24l01_device_t *dev, pin_set_fn fn);

#endif /* NRF24L01_H */