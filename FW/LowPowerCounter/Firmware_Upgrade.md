# Firmware Upgrade Plan — LowPowerCounter PIC16F18345

**Based on:** Firmware_Description.md analysis  
**Primary goal:** Implement working nRF24L01 TX communication  
**Secondary goals:** Fix structural bugs, improve low-power correctness

---

## Priority Classification

- **[BLOCKER]** — The firmware will not compile or the radio will not work at all without this fix
- **[MAJOR]** — Significant functional gap, must be addressed for a deployable product
- **[MINOR]** — Quality or robustness improvement

---

## 1. Fix SPI Wiring to the Driver — [BLOCKER]

### Problem

`main.c` registers `SPI1_Exchange8bitBuffer` as the SPI transfer function:

```c
nrf24l01_spi_transfer_fn_register(&radio_dev, &SPI1_Exchange8bitBuffer);
```

`SPI1_Exchange8bitBuffer` does not exist. The radio module is physically wired to **MSSP2** (RB4=SDI, RB5=SDO, RB6=SCK).

### Fix

The `spi_transfer_fn` signature in `nrf24l01.h` is:

```c
typedef void (*spi_transfer_fn)(const uint8_t *out, uint8_t *in, size_t len);
```

`SPI2_BufferExchange()` exchanges a single in-place buffer — signature mismatch. Write a thin wrapper that maps the driver's separate out/in buffers to SPI2:

```c
static void SPI2_TransferWrapper(const uint8_t *out, uint8_t *in, size_t len) {
    SPI2_Open(MSSP2_DEFAULT);
    for (size_t i = 0; i < len; i++) {
        uint8_t tx = (out != NULL) ? out[i] : 0xFF;
        uint8_t rx = SPI2_ByteExchange(tx);
        if (in != NULL) in[i] = rx;
    }
    SPI2_Close();
}
```

Then register it:

```c
nrf24l01_spi_transfer_fn_register(&radio_dev, &SPI2_TransferWrapper);
```

> **Note:** `SPI2_Open()` must be called before the first SPI transaction. Opening/closing per transaction is safe here since transmission is infrequent; alternatively open once at startup and never close.

---

## 2. Define and Wire CSN and CE GPIO Pins — [BLOCKER]

### Problem

`main.c` references `NRF24L01_CSN_SetHigh` and `NRF24L01_CE_SetHigh` — neither exists in `pins.h`. No GPIO macros are defined for the nRF24L01 chip-select (CSN) or chip-enable (CE) pins.

From the schematic, the unassigned right-side MCU pins carry CSN, CE, and IRQ. Based on the PPS configuration in `pins.c`, the SPI bus uses RB4/RB5/RB6, leaving available GPIOs on port C (RC1, RC2, RC3, RC4, RC5, RC6) and port A (RA2).

### Fix — Step A: Identify and Name the Pins

In MCC, assign names to the CSN, CE, and IRQ pins (RC1, RC2, RA2 — or whichever match your board). This generates macros in `pins.h`. Suggested names:

| Signal | Pin  | Suggested MCC alias |
|--------|------|---------------------|
| CSN    | RC2  | `NRF_CSN`           |
| CE     | RC5  | `NRF_CE`            |
| IRQ    | RC1  | `NRF_IRQ`           |

### Fix — Step B: Write Proper Callback Functions

The `pin_set_fn` type takes a `bool` — it must **set or clear** the pin depending on the argument:

```c
static void NRF_CSN_Control(bool level) {
    if (level) NRF_CSN_SetHigh(); else NRF_CSN_SetLow();
}

static void NRF_CE_Control(bool level) {
    if (level) NRF_CE_SetHigh(); else NRF_CE_SetLow();
}
```

Register these instead of the non-existent `SetHigh`-only variants:

```c
nrf24l01_pin_set_fn_csn_register(&radio_dev, &NRF_CSN_Control);
nrf24l01_pin_set_fn_ce_register(&radio_dev, &NRF_CE_Control);
```

### Fix — Step C: Ensure CSN Idles High

Add `NRF_CSN_SetHigh()` at startup (before `nrf24l01_init()`). CSN is active-low; it must be deasserted when the bus is idle.

---

## 3. Implement nRF24L01 Transmitter Initialization — [BLOCKER]

### Problem

- `nrf24l01_init()` is **never called** in `main.c`.
- The init function configures the radio as a **receiver** (sets `PRIM_RX=1`). This device is a **transmitter**.
- The full RF register set (channel, data rate, TX address, payload width, auto-ACK, retransmit) is never configured.

### Fix — Expand the Register Map in `nrf24l01.h`

Add the missing register addresses and bit definitions:

```c
// Additional register addresses
#define NRF24_REG_EN_AA         0x01  // Auto-Acknowledgment
#define NRF24_REG_EN_RXADDR     0x02  // Enabled RX addresses
#define NRF24_REG_SETUP_AW      0x03  // Address widths
#define NRF24_REG_SETUP_RETR    0x04  // Retransmit config
#define NRF24_REG_RF_CH         0x05  // RF channel (0–125)
#define NRF24_REG_RF_SETUP      0x06  // Data rate, TX power
#define NRF24_REG_TX_ADDR       0x10  // Transmit address (5 bytes)
#define NRF24_REG_RX_ADDR_P0    0x0A  // RX pipe 0 address (for ACK)
#define NRF24_REG_RX_PW_P0      0x11  // Payload width pipe 0

// Additional commands
#define NRF24_CMD_NOP           0xFF  // No operation (read STATUS)
#define NRF24_CMD_WRITE_PAYLOAD 0xA0  // W_TX_PAYLOAD

// CONFIG bits
#define NRF24_CONFIG_EN_CRC     0x08
#define NRF24_CONFIG_CRC0       0x04

// STATUS bits
#define NRF24_STATUS_TX_DS      0x20  // TX data sent
#define NRF24_STATUS_MAX_RT     0x10  // Max retransmits reached
#define NRF24_STATUS_RX_DR      0x40  // RX data ready

// RF_SETUP values
#define NRF24_RF_PWR_0DBM       0x06
#define NRF24_DR_250KBPS        0x20  // 250 kbps (best range/sensitivity)
#define NRF24_DR_1MBPS          0x00

// Multi-byte register write command
#define NRF24_CMD_WRITE_REG_MB(reg)  (NRF24_CMD_WRITE_REG | ((reg) & 0x1F))
```

### Fix — Add Multi-byte Register Write to `nrf24l01.c`

```c
static void nrf24_reg_write_mb(nrf24l01_device_t *dev, uint8_t reg,
                                const uint8_t *data, uint8_t len) {
    uint8_t cmd = NRF24_CMD_WRITE_REG | (reg & 0x1F);
    dev->csn_set(false);
    dev->spi_transfer(&cmd, NULL, 1);
    dev->spi_transfer(data, NULL, len);
    dev->csn_set(true);
}
```

### Fix — Implement TX Initialization

Replace or extend `nrf24l01_init()` with a proper TX setup:

```c
void nrf24l01_init_tx(nrf24l01_device_t *dev, uint8_t rf_channel,
                      const uint8_t *tx_addr, uint8_t addr_len) {
    // Power down first for clean init
    nrf24_reg_write(dev, NRF24_REG_CONFIG, NRF24_CONFIG_EN_CRC);
    __delay_ms(2);

    // Flush FIFOs
    uint8_t cmd;
    cmd = NRF24_CMD_FLUSH_TX;
    dev->csn_set(false); dev->spi_transfer(&cmd, NULL, 1); dev->csn_set(true);
    cmd = NRF24_CMD_FLUSH_RX;
    dev->csn_set(false); dev->spi_transfer(&cmd, NULL, 1); dev->csn_set(true);

    // Clear status flags
    nrf24_reg_write(dev, NRF24_REG_STATUS,
                    NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT | NRF24_STATUS_RX_DR);

    // RF channel and data rate (250 kbps for best range)
    nrf24_reg_write(dev, NRF24_REG_RF_CH, rf_channel & 0x7F);
    nrf24_reg_write(dev, NRF24_REG_RF_SETUP, NRF24_RF_PWR_0DBM | NRF24_DR_250KBPS);

    // 5-byte addresses, auto-ACK on pipe 0, enable pipe 0
    nrf24_reg_write(dev, NRF24_REG_SETUP_AW, 0x03);      // 5-byte addr
    nrf24_reg_write(dev, NRF24_REG_EN_AA, 0x01);          // ACK on pipe 0
    nrf24_reg_write(dev, NRF24_REG_EN_RXADDR, 0x01);      // pipe 0 enabled
    nrf24_reg_write(dev, NRF24_REG_SETUP_RETR, 0x1F);    // 500 µs, 15 retries

    // TX and RX pipe 0 address (same for auto-ACK to work)
    nrf24_reg_write_mb(dev, NRF24_REG_TX_ADDR, tx_addr, addr_len);
    nrf24_reg_write_mb(dev, NRF24_REG_RX_ADDR_P0, tx_addr, addr_len);
    nrf24_reg_write(dev, NRF24_REG_RX_PW_P0, 4); // 4-byte payload (counter)

    // Power up, TX mode (PRIM_RX = 0)
    nrf24_reg_write(dev, NRF24_REG_CONFIG,
                    NRF24_CONFIG_EN_CRC | NRF24_CONFIG_PWR_UP);
    __delay_ms(2); // tpd2stby
}
```

---

## 4. Implement Payload Transmission — [BLOCKER]

### Problem

State 2/3 in `main.c` only blink the LED. No SPI transactions are performed. The `counter` value is never sent.

### Fix — Add `nrf24l01_send_data()` to `nrf24l01.c`

```c
uint8_t nrf24l01_send_data(nrf24l01_device_t *dev,
                           const uint8_t *payload, uint8_t len) {
    uint8_t cmd = NRF24_CMD_WRITE_PAYLOAD;
    uint8_t status;

    // Load payload into TX FIFO
    dev->csn_set(false);
    dev->spi_transfer(&cmd, NULL, 1);
    dev->spi_transfer(payload, NULL, len);
    dev->csn_set(true);

    // Pulse CE high for ≥10 µs to start transmission
    dev->ce_set(true);
    __delay_us(15);
    dev->ce_set(false);

    // Poll STATUS for TX_DS (sent) or MAX_RT (failed) — max ~5 ms with retries
    uint16_t timeout = 5000;
    do {
        status = nrf24_reg_read(dev, NRF24_REG_STATUS);
        __delay_us(1);
    } while (!(status & (NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT)) && --timeout);

    // Clear flags
    nrf24_reg_write(dev, NRF24_REG_STATUS,
                    NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT);

    if (status & NRF24_STATUS_MAX_RT) {
        uint8_t flush = NRF24_CMD_FLUSH_TX;
        dev->csn_set(false); dev->spi_transfer(&flush, NULL, 1); dev->csn_set(true);
        return 0; // failure
    }
    return 1; // success
}
```

### Fix — Call it from `main.c` State 2

Replace the dummy LED blink in state 2/3 with:

```c
case 2: {
    uint8_t payload[4];
    payload[0] = (uint8_t)(counter >> 24);
    payload[1] = (uint8_t)(counter >> 16);
    payload[2] = (uint8_t)(counter >>  8);
    payload[3] = (uint8_t)(counter);
    LED_SetHigh();
    PWR_EN_SetHigh();          // power on the radio module via Q2
    __delay_ms(10);            // wait for radio to stabilize after power-on
    nrf24l01_init_tx(&radio_dev, RF_CHANNEL, tx_address, 5);
    nrf24l01_send_data(&radio_dev, payload, 4);
    nrf24l01_sleep(&radio_dev);
    PWR_EN_SetLow();           // cut radio power
    LED_SetLow();
    state = 1;
    break;
}
```

> This removes states 3 and the `dataSent` mechanism for the transmission path — a blocking send is acceptable for a low-frequency counter node. If a non-blocking approach is needed, states 3+ can be repurposed to wait on IRQ.

---

## 5. Fix the PWR_EN Pin — [MAJOR]

### Problem

The schematic labels **RA5** as `POW_EN` (Q2 gate). The firmware assigns RA5 to **UART TX** (`RA5PPS = 20` in `pins.c`) and defines `PWR_EN` macros on **RC7**. Additionally, `PWR_EN_SetHigh/Low` is never called in `main.c`.

There are two sub-issues:
1. Which pin actually drives Q2 — RA5 (schematic) or RC7 (firmware)?
2. The application never activates the power gate regardless.

### Fix

**Verify the board** to determine which MCU pin connects to Q2's gate. Then reconcile:

- If the hardware follows the schematic (RA5 → Q2), remove the UART PPS assignment from RA5 (`RA5PPS = 0`) and reconfigure RA5 as a GPIO output named `PWR_EN`.
- If the board was updated to use RC7, update the schematic. Either way, ensure MCC names it `PWR_EN` and the application calls `PWR_EN_SetHigh()` before radio use and `PWR_EN_SetLow()` after.

> The UART is not used by the application at all — EUSART can be removed from the MCC configuration unless it is needed for debug output.

---

## 6. Add Delay Before Radio Initialization After Power-On — [MAJOR]

### Problem

The nRF24L01 requires **100 ms** power-on reset time before accepting SPI commands. After `PWR_EN_SetHigh()` nothing waits.

### Fix

```c
PWR_EN_SetHigh();
__delay_ms(100);   // nRF24L01 power-on reset (datasheet: tPOR = 100 ms)
nrf24l01_init_tx(...);
```

---

## 7. Fix the External Interrupt Configuration — [MAJOR]

### Problem

`pins.c` sets all IOC registers to zero (`IOCCP=0`, `IOCCN=0`, etc.). `POWER_PeripheralEnable(POWER_IOC)` is called, and `EXT_INT_InterruptEnable()` is also called. The external INT uses `INTPPS = 0x10` (RC0). So the external INT path should work — but the IOC peripheral enable is wasted since no IOC pin is configured.

Separately: the interrupt handler `INT_CB` only re-enables TMR1 — it does not clear any interrupt flag. On PIC16, external INT requires clearing `INTF` (PIR0bits.INTF) in the handler to avoid continuous re-entry. Verify that the MCC-generated interrupt framework clears this flag automatically; if not, add it.

### Fix

In `INT_CB` (or ensure the MCC interrupt dispatcher does it):

```c
void INT_CB(void) {
    POWER_PeripheralEnable(POWER_TMR1);
    // INTF is cleared by the MCC framework before calling this handler — verify
}
```

Remove `POWER_PeripheralEnable(POWER_IOC)` from main if pure IOC is not used — the external INT on RC0 does not require the IOC peripheral.

---

## 8. Persist the Counter in EEPROM — [MAJOR]

### Problem

`counter` is a RAM variable. A power loss or battery swap resets the count to zero permanently.

### Fix

Write `counter` to EEPROM before entering sleep, after each transmission. On startup, read the saved value back:

```c
// Startup
counter = EEPROM_Read32(EEPROM_COUNTER_ADDR);

// After successful transmission in state 2
EEPROM_Write32(EEPROM_COUNTER_ADDR, counter);
```

Implement a simple 32-bit EEPROM helper using the XC8 `__EEPROM_DATA` mechanism or the NVM driver.

> Write only after transmission, not on every pulse, to limit EEPROM wear (rated 100k cycles on PIC16F18345).

---

## 9. Add a Transmission Interval Configuration — [MINOR]

### Problem

`sendData_counter >= 5` is hardcoded. Changing the reporting interval requires a recompile.

### Fix

Define a named constant at the top of `main.c`:

```c
#define TX_PULSE_INTERVAL  5u   // transmit every N reed pulses
```

Use it in the condition:

```c
if (sendData_counter >= TX_PULSE_INTERVAL) {
```

---

## 10. Remove Unused Peripherals from SYSTEM_Initialize — [MINOR]

`EUSART_Initialize()` and `MSSP1_Initialize()` are called at startup but the UART and MSSP1 are never used. Remove them from `system.c` and from the MCC configuration to save a few microseconds of startup time and reduce confusion.

---

## 11. Define a Packet Structure for the Radio Payload — [MINOR]

### Problem

The counter is sent as 4 raw bytes with no framing, device ID, or integrity check. A receiver has no way to distinguish this device from others or detect a corrupted packet.

### Fix

Define a small packet:

```c
typedef struct {
    uint8_t  device_id;    // unique node identifier (compile-time constant)
    uint32_t counter;      // pulse count (big-endian)
    uint8_t  checksum;     // XOR of previous bytes
} __attribute__((packed)) radio_packet_t;
```

Compute checksum before sending:

```c
radio_packet_t pkt;
pkt.device_id = DEVICE_ID;
pkt.counter   = counter;
pkt.checksum  = pkt.device_id
              ^ (uint8_t)(counter >> 24)
              ^ (uint8_t)(counter >> 16)
              ^ (uint8_t)(counter >>  8)
              ^ (uint8_t)(counter);
nrf24l01_send_data(&radio_dev, (uint8_t *)&pkt, sizeof(pkt));
```

---

## Summary Table

| # | Area | Priority | Description |
|---|------|----------|-------------|
| 1 | SPI wiring | BLOCKER | Replace `SPI1_Exchange8bitBuffer` with SPI2 wrapper |
| 2 | GPIO pins | BLOCKER | Define CSN/CE macros, write `bool`-capable wrappers |
| 3 | Radio init | BLOCKER | Implement TX init with full register config; call it |
| 4 | TX function | BLOCKER | Implement `nrf24l01_send_data()`; call from state 2 |
| 5 | PWR_EN pin | MAJOR | Resolve RA5/RC7 discrepancy; activate Q2 before TX |
| 6 | Power-on delay | MAJOR | 100 ms wait after PWR_EN before SPI access |
| 7 | INT handler | MAJOR | Verify INTF clear; remove unused IOC peripheral enable |
| 8 | EEPROM | MAJOR | Persist counter across power cycles |
| 9 | TX interval | MINOR | Replace magic number 5 with a named constant |
| 10 | Unused init | MINOR | Remove UART and MSSP1 from system init |
| 11 | Packet format | MINOR | Add device ID and checksum to payload |
