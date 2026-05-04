# Schematic Description — LowPowerRadioComm Main Board

**File:** Schematic.pdf  
**Board:** Main Board (Page P1 of 1)  
**Project:** LowPowerRadioComm  
**Version:** V1.0  
**Created:** 2026-01-09 | **Updated:** 2026-01-28  
**Tool:** EasyEDA

---

## Overview

This is a single-page schematic for a low-power wireless counter/sensor node. The design centers on a **PIC16F18345** microcontroller that reads a reed switch input, drives an indicator LED, and transmits data wirelessly via an **nRF24L01** 2.4 GHz radio module. The board is battery-powered with a software-controlled power enable circuit to minimize current draw when idle.

---

## Components

### Microcontroller — U3: PIC16F18345-E/SO

20-pin SOIC package. Core of the design.

| Pin | Name             | Net / Function                    |
|-----|------------------|-----------------------------------|
| 1   | VDD              | VCC                               |
| 2   | RA5              | POW_EN — controls Q2 gate         |
| 3   | RA4              | (not labeled in schematic)        |
| 4   | MCLR#/VPP/RA3   | RST — reset line via R1 pull-up   |
| 5   | RC5              | (not labeled)                     |
| 6   | RC4              | (not labeled)                     |
| 7   | RC3              | (not labeled)                     |
| 8   | RC6              | (not labeled)                     |
| 9   | RC7              | (not labeled)                     |
| 10  | RB7              | (not labeled)                     |
| 11  | RB6              | SCK — SPI clock to nRF24L01       |
| 12  | RB5              | MOSI — SPI data out to nRF24L01   |
| 13  | RB4              | MISO — SPI data in from nRF24L01  |
| 14  | RC2              | CSN — nRF24L01 chip select        |
| 15  | RC1              | IRQ — nRF24L01 interrupt          |
| 16  | RC0              | CE — nRF24L01 chip enable         |
| 17  | RA2              | (not labeled / general GPIO)      |
| 18  | RA1/ICSPCLK     | ICSPCLK — ICSP programming clock  |
| 19  | RA0/ICSPDAT     | ICSPDAT — ICSP programming data   |
| 20  | VSS              | GND                               |

---

### Power Supply

| Ref | Part                | Description                                                  |
|-----|---------------------|--------------------------------------------------------------|
| BT1 | BH-AAA-B5AK001      | AAA battery holder — primary power source, supplies VCC     |
| SW1 | SMD Slide Switch    | Main power switch between battery and VCC rail              |
| J4  | PZ2.54-1*2          | 2-pin jumper — in series with SW1 power path                |
| C1  | 10 µF               | Bulk decoupling capacitor on VCC rail (near nRF24L01)       |

The battery positive terminal feeds through **SW1** (slide switch) in combination with **J4** (jumper) to produce the **VCC** rail. C1 provides bulk decoupling near the radio module.

---

### Power Enable Circuit — Q2: 12N65

| Ref | Part  | Value | Description                                     |
|-----|-------|-------|-------------------------------------------------|
| Q2  | 12N65 | —     | Transistor/MOSFET used as power switch          |
| R3  | —     | 10 kΩ | Gate/base resistor for Q2                       |

The MCU drives **POW_EN** (RA5) to turn Q2 on/off. This allows the firmware to cut power to a peripheral (likely the nRF24L01 module) when not in use, which is key to achieving low standby current. R3 limits gate/base drive current.

---

### nRF24L01 Radio Module Connector — J2: PM2.54-2*4-H5.7

8-pin (2×4) male header for a plug-in nRF24L01 or nRF24L01+ module.

| J2 Pin | Net  | Description                       |
|--------|------|-----------------------------------|
| 1      | VDD  | 3.3 V supply to module            |
| 2      | GND  | Ground                            |
| 3      | CE   | Chip Enable (from MCU RC0)        |
| 4      | CSN  | Chip Select Not (from MCU RC2)    |
| 5      | SCK  | SPI Clock (from MCU RB6)          |
| 6      | MOSI | SPI Data Out (from MCU RB5)       |
| 7      | MISO | SPI Data In (to MCU RB4)          |
| 8      | IRQ  | Interrupt Request (to MCU RC1)    |

The nRF24L01 is controlled via hardware SPI. CE and CSN are dedicated GPIO lines. IRQ feeds an interrupt-capable MCU pin to allow wake-from-sleep on radio events.

---

### Reed Switch Input — J3: DB127L-5.08-2P-GN-S

| Ref | Part                    | Description                                              |
|-----|-------------------------|----------------------------------------------------------|
| J3  | DB127L-5.08-2P-GN-S     | 2-pin 5.08 mm screw terminal, labeled "REED Connector"  |
| R4  | —                       | 10 kΩ pull-down resistor on reed switch input line       |

An external reed switch connects here. One terminal goes to VCC, the other to an MCU GPIO through R4 (pull-down to GND). When the reed switch closes (e.g., triggered by a magnet), the MCU pin sees a logic high — this is the primary counting/detection input.

---

### Status LED

| Ref  | Part       | Value  | Description                             |
|------|------------|--------|-----------------------------------------|
| R2   | —          | 200 Ω  | Current-limiting resistor for LED       |
| LED1 | YLED0805R  | —      | Yellow LED, 0805 SMD package            |

R2 and LED1 are connected in series from an MCU GPIO to GND. Used as a visual status/activity indicator.

---

### ICSP Programming Header — J1: PZ2.54-1X6P-H25

6-pin single-row 2.54 mm header for in-circuit serial programming (MPLAB SNAP / PICkit).

| Pin | Net      |
|-----|----------|
| 1   | VCC (via R1 pull-up node) |
| 2   | (GND)    |
| 3   | RST      |
| 4   | ICSPDAT  |
| 5   | ICSPCLK  |
| 6   | (GND/NC) |

**R1 (10 kΩ)** is a pull-up resistor on the RST line (MCLR#), keeping the MCU out of reset during normal operation.

---

## Signal Net Summary

| Net      | Source          | Destination(s)              |
|----------|-----------------|-----------------------------|
| VCC      | BT1 / SW1       | U3 VDD, J2 VDD, J3, R1, R4 |
| GND      | BT1 negative    | U3 VSS, J2 GND, LED1, C1   |
| POW_EN   | U3 RA5          | R3 → Q2 gate                |
| RST      | U3 MCLR (pin 4) | R1 pull-up, J1              |
| ICSPDAT  | U3 RA0 (pin 19) | J1                          |
| ICSPCLK  | U3 RA1 (pin 18) | J1                          |
| CE       | U3 RC0 (pin 16) | J2 pin 3                    |
| CSN      | U3 RC2 (pin 14) | J2 pin 4                    |
| SCK      | U3 RB6 (pin 11) | J2 pin 5                    |
| MOSI     | U3 RB5 (pin 12) | J2 pin 6                    |
| MISO     | U3 RB4 (pin 13) | J2 pin 7                    |
| IRQ      | J2 pin 8        | U3 RC1 (pin 15)             |

---

## Design Intent

The board is purpose-built for **low-power event counting** — the name "LowPowerRadioComm" and the presence of a reed switch input strongly suggest this device counts magnetic pulses (e.g., from a utility meter) and reports them wirelessly. Key design decisions supporting low power:

- **Battery powered** via AAA cells with a manual slide switch cutoff.
- **Q2 power gate** (POW_EN) allows the MCU to completely de-energize the nRF24L01 between transmissions.
- **IRQ line from nRF24L01** allows the MCU to sleep and wake only on relevant radio events.
- **Reed switch** is a passive, zero-power sensor that can wake the MCU via interrupt.
