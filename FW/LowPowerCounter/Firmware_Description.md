# Firmware Description — LowPowerCounter PIC16F18345

**Toolchain:** XC8 / MPLAB X  
**Framework:** MCC Melody (generated drivers)  
**Target:** PIC16F18345-E/SO  
**Date analyzed:** 2026-05-03

---

## 1. File Structure

```
default/mcc/
├── main.c                          Application entry point & state machine
├── nrf24l01.c                      nRF24L01 radio driver (partial)
├── nrf24l01.h                      nRF24L01 driver types & API declarations
└── mcc_generated_files/
    ├── system/
    │   ├── system.c / system.h     Top-level SYSTEM_Initialize()
    │   ├── clock.c / clock.h       Oscillator config (32 MHz HFINTOSC)
    │   ├── pins.c / pins.h         Pin directions, PPS, IOC — all GPIO macros
    │   ├── interrupt.c / .h        Global/peripheral interrupt enable
    │   └── config_bits.c / .h      Device configuration fuses
    ├── power/
    │   └── power.c / power.h       PMD enable/disable, SLEEP() wrapper
    ├── timer/
    │   ├── tmr1.c / tmr1.h         16-bit timer, 32 MHz input (~2 ms overflow)
    │   └── tmr2.c / tmr2.h         8-bit timer, 7812.5 Hz input (~33 ms overflow)
    ├── spi/
    │   └── mssp2.c / mssp2.h       SPI2 (polling), wired to nRF24L01 pins
    ├── peripheral/
    │   └── mssp1.c / mssp1.h       MSSP1 — initialized but not used in app
    └── uart/
        └── eusart.c / eusart.h     EUSART — initialized but not used in app
```

---

## 2. Hardware Abstraction (Pins)

Resolved from `pins.c` (PPS assignments) and `pins.h` (named macros):

| Logical Name  | MCU Pin | Direction | Function                          |
|---------------|---------|-----------|-----------------------------------|
| `REED`        | RC0     | Input     | Reed switch — wired to external INT |
| `LED`         | RB7     | Output    | Yellow status LED                 |
| `PWR_EN`      | RC7     | Output    | Radio power enable (Q2 gate)      |
| SPI2 SCK      | RB6     | Output    | nRF24L01 clock (MSSP2)            |
| SPI2 MOSI/SDO | RB5     | Output    | nRF24L01 data out (MSSP2)         |
| SPI2 MISO/SDI | RB4     | Input     | nRF24L01 data in (MSSP2)          |
| UART TX       | RA5     | Output    | EUSART transmit (not used in app) |
| UART RX       | RA4     | Input     | EUSART receive (not used in app)  |

> **Note:** CSN and CE pins for the nRF24L01 module are **not named** in `pins.h`. The application references `NRF24L01_CSN_SetHigh` and `NRF24L01_CE_SetHigh` which are **undefined symbols**.

---

## 3. Clock Configuration

Set in `clock.c`:
- **Source:** HFINTOSC, no divider (NDIV=1, NOSC=6)
- **Frequency:** 32 MHz

TMR1 and TMR2 are clocked from Fosc:
- **TMR1** — 16-bit, prescaled input = 32 MHz → overflow period ≈ **2.048 ms**
- **TMR2** — 8-bit, prescaled input = 7812.5 Hz → period match ≈ **32.6 ms** (full 255 count)

---

## 4. Peripheral Initialization Sequence

`SYSTEM_Initialize()` in `system.c` calls in order:

1. `CLOCK_Initialize()` — 32 MHz HFINTOSC
2. `PIN_MANAGER_Initialize()` — TRIS, LAT, ANS, PPS, IOC registers
3. `TMR1_Initialize()` — configures TMR1
4. `TMR2_Initialize()` — configures TMR2
5. `EUSART_Initialize()` — UART (unused in application)
6. `MSSP1_Initialize()` — SPI/I2C port 1 (unused in application)
7. `PMD_Initialize()` — enables all peripherals initially (PMD regs all 0, except TMR2 disabled at init: PMD1=0x04)
8. `POWER_Initialize()` — power module init
9. `SPI2_Initialize()` — resets MSSP2 registers (SPI not opened here — must call `SPI2_Open()` before use)
10. `INTERRUPT_Initialize()` — clears interrupt flags, sets priorities

---

## 5. Power Management

Implemented in `power.c` via the PIC16F Peripheral Module Disable (PMD) registers:

| Function                    | Effect                                    |
|-----------------------------|-------------------------------------------|
| `POWER_PeripheralDisableAll()` | Sets all PMD0–PMD5 = 0xFF (cuts clock to every peripheral) |
| `POWER_PeripheralEnable(x)` | Clears the corresponding PMD bit (enables one peripheral) |
| `POWER_PeripheralDisable(x)`| Sets the corresponding PMD bit (gates off one peripheral) |
| `POWER_LowPowerModeEnter()` | Executes `SLEEP()` + `NOP()` — device wakes on any enabled interrupt |

The application uses this aggressively: at startup, after initialization it calls `POWER_PeripheralDisableAll()` and then re-enables only what it needs (TMR1, TMR2, IOC).

---

## 6. Interrupt Service Routines

Three interrupt sources are wired to application callbacks in `main.c`:

| ISR / Callback | Source         | Registered via                              | Action                                |
|----------------|----------------|---------------------------------------------|---------------------------------------|
| `TMR1_CB`      | TMR1 overflow  | `TMR1_OverflowCallbackRegister(&TMR1_CB)`   | Increments `led_count`; sets `dataSent=1` every 10 overflows (~20 ms) |
| `TMR2_CB`      | TMR2 period match | `TMR2_OverflowCallbackRegister(&TMR2_CB)` | Sets `tmr2_event=1`; disables TMR2   |
| `INT_CB`       | External INT (RC0) | `INT_SetInterruptHandler(&INT_CB)`       | Re-enables TMR1 (wakes state machine from sleep) |

---

## 7. Application State Machine (`main.c`)

Global variables used:

| Variable          | Type       | Purpose                                         |
|-------------------|------------|-------------------------------------------------|
| `state`           | `uint8_t`  | Current state machine state (static local)      |
| `counter`         | `uint32_t` | Total reed switch pulse count (never reset)     |
| `sendData_counter`| `uint32_t` | Pulses since last transmission (resets at 5)    |
| `dataSent`        | `uint8_t`  | Flag: TMR1_CB signals LED timeout complete      |
| `tmr2_event`      | `uint8_t`  | Flag: TMR2_CB signals debounce timeout expired  |
| `radio_dev`       | `nrf24l01_device_t` | nRF24L01 device instance (global)       |

### State Diagram

```
        Power-on
            │
            ▼
    ┌──────────────┐
    │   State 0    │  Wait for startup LED to turn off
    │  (LED on)    │  (TMR1 runs ~20 ms, sets dataSent=1)
    └──────┬───────┘
           │ dataSent==1 → LED off
           ▼
    ┌──────────────┐   ◄──────────────────────────────────────────────┐
    │   State 1    │  SLEEP (wait for reed switch interrupt on RC0)    │
    │  (idle/sleep)│  Wake: counter++, sendData_counter++              │
    └──────┬───────┘                                                    │
           │                                                            │
           ├─ sendData_counter < 5 ──► State 5 (debounce)  ───────────►┤
           │                                                            │
           └─ sendData_counter >= 5 → reset counter ──► State 2        │
                                                                        │
    ┌──────────────┐                                                    │
    │   State 2    │  LED on, TMR1 start (simulates TX in progress)    │
    └──────┬───────┘                                                    │
           ▼                                                            │
    ┌──────────────┐                                                    │
    │   State 3    │  Wait for dataSent==1 (TMR1 ~20 ms timeout)       │
    │  (fake TX)   │  → LED off → back to State 1  ────────────────────┘
    └──────────────┘

    ┌──────────────┐
    │   State 5    │  Wait for TMR2 to expire (~33 ms debounce)
    │  (debounce)  │  → back to State 1 when tmr2_event==1
    └──────────────┘
```

### Behavior Summary

- On **power-on**: LED turns on, TMR1 runs for ~20 ms to turn it off (startup blink).
- In **State 1**: the MCU enters deep sleep (`SLEEP()`). TMR1 is disabled before sleeping. The device wakes only when RC0 sees a rising/falling edge from the reed switch.
- Each wake increments `counter` (total pulses) and `sendData_counter`.
- Every **5th pulse**: transitions to State 2 to simulate a radio transmission (LED blinks for ~20 ms using TMR1).
- Between pulses 1–4: transitions to State 5 — starts TMR2 for a ~33 ms debounce window to reject noise before going back to sleep.
- **No actual radio data is transmitted** — State 2/3 only toggle the LED.

---

## 8. nRF24L01 Driver (`nrf24l01.c` / `nrf24l01.h`)

### Design Pattern

A hardware-agnostic driver using function pointers for portability:

```c
typedef struct {
    spi_transfer_fn  spi_transfer;  // void (*)(const uint8_t *out, uint8_t *in, size_t len)
    pin_set_fn       csn_set;       // void (*)(bool level)
    pin_set_fn       ce_set;        // void (*)(bool level)
} nrf24l01_device_t;
```

The user injects hardware functions at runtime via three registration functions.

### Implemented Functions

| Function                            | Status    | Description                                      |
|-------------------------------------|-----------|--------------------------------------------------|
| `nrf24l01_spi_transfer_fn_register` | Working   | Stores SPI transfer function pointer             |
| `nrf24l01_pin_set_fn_csn_register`  | Working   | Stores CSN pin control function pointer          |
| `nrf24l01_pin_set_fn_ce_register`   | Working   | Stores CE pin control function pointer           |
| `nrf24l01_init()`                   | Skeleton  | Flushes FIFOs, sets RX mode — **never called**   |
| `nrf24l01_data_available()`         | Skeleton  | Reads FIFO_STATUS, checks RX_EMPTY bit           |
| `nrf24l01_read_data()`              | Skeleton  | Issues R_RX_PAYLOAD command, reads payload       |
| `nrf24l01_sleep()`                  | Skeleton  | Clears PWR_UP bit in CONFIG register             |
| `nrf24l01_wake()`                   | Skeleton  | Sets PWR_UP bit in CONFIG register               |

### Defined Register/Command Macros

Only a minimal subset is defined:

| Macro                    | Value  | Register / Command           |
|--------------------------|--------|------------------------------|
| `NRF24_CMD_READ_REG`     | 0x00   | R_REGISTER command           |
| `NRF24_CMD_WRITE_REG`    | 0x20   | W_REGISTER command           |
| `NRF24_CMD_FLUSH_RX`     | 0xE2   | FLUSH_RX command             |
| `NRF24_CMD_FLUSH_TX`     | 0xE1   | FLUSH_TX command             |
| `NRF24_CMD_READ_PAYLOAD` | 0x61   | R_RX_PAYLOAD command         |
| `NRF24_CMD_WRITE_PAYLOAD`| 0xA0   | W_TX_PAYLOAD command         |
| `NRF24_REG_CONFIG`       | 0x00   | CONFIG register address      |
| `NRF24_REG_STATUS`       | 0x07   | STATUS register address      |
| `NRF24_REG_FIFO_STATUS`  | 0x17   | FIFO_STATUS register address |
| `NRF24_CONFIG_PWR_UP`    | 0x02   | CONFIG: PWR_UP bit           |
| `NRF24_CONFIG_PRIM_RX`   | 0x01   | CONFIG: PRIM_RX bit (RX mode)|
| `NRF24_FIFO_RX_EMPTY`    | 0x01   | FIFO_STATUS: RX_EMPTY bit    |

The entire RF configuration register set (RF_CH, RF_SETUP, TX_ADDR, RX_ADDR_Px, RX_PW_Px, EN_AA, EN_RXADDR, SETUP_RETR, etc.) is **not defined**.

---

## 9. Known Issues and Gaps

| # | Location | Issue |
|---|----------|-------|
| 1 | `main.c:97` | `SPI1_Exchange8bitBuffer` does not exist — SPI2 is the radio SPI bus |
| 2 | `main.c:98–99` | `NRF24L01_CSN_SetHigh` and `NRF24L01_CE_SetHigh` are undefined — no GPIO macros for CSN/CE |
| 3 | `main.c` | `nrf24l01_init()` is **never called** — the radio is never initialized |
| 4 | `main.c` | `PWR_EN_SetHigh/Low` is **never called** — Q2 power gate is never toggled |
| 5 | `nrf24l01.c` | Pin control callbacks are registered as `SetHigh`-only functions but are called with a `bool` argument — the `bool` is ignored |
| 6 | `nrf24l01.c` | `nrf24l01_init()` configures the radio as a **receiver** (PRIM_RX=1) — a counter node should transmit |
| 7 | `pins.c` | All IOC registers (IOCCP/IOCCN etc.) are zero — IOC is armed but no pin is configured to generate an IOC interrupt |
| 8 | `pins.c` | `RA5` is mapped to EUSART TX, but schematic labels RA5 as POW_EN |
| 9 | `nrf24l01.h` | Only 3 register addresses defined — full register map missing |
| 10 | `main.c` | State 4 does not exist — creates an implicit dead-state in the switch |
| 11 | `system.c` | `SPI2_Open()` is never called — SPI2 peripheral is initialized but not opened/enabled |
