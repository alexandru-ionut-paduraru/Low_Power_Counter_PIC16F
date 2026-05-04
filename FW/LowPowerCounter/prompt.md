# Project Generation Prompt

# Project description 
This project demonstrates the use of a low power counter on a PIC16F microcontroller. The code initializes the necessary peripherals, including the SPI interface for communication with an NRF24L01 radio module. The main loop of the program already contains the power management and counter logic, but does not yet include the RF module communication code.

# List of features to be implemented
1. **Counter Increment**: Implement the logic to increment the counter when an external interrupt occurs (e.g., a button press or a sensor trigger).
2. **RF Communication**: Implement the logic to transmit the counter value over the NRF24L01 radio module when the counter reaches a certain threshold (e.g., 10).
3. **Power Management**: Ensure that the microcontroller enters a low power sleep mode when not actively counting or transmitting data.

# Schematic diagram
For more details regarding the schematic of this device refer to the `Schematic.pdf` file in the rout of the project directory.

# Hardware and Software Architecture
The hardware architecture of this project consists of a PIC16F18446 microcontroller, which is responsible for counting the pulses and transmitting the data over the radio interface. The microcontroller is connected to an external interrupt source, which wakes it from sleep mode to increment the counter. The counter value is stored in non-volatile memory, allowing it to retain its value even when the power is removed. The microcontroller also has a radio interface that allows it to transmit data to a Raspberry Pi server. The Raspberry Pi server is responsible for receiving the data and processing it as needed. This hardware architecture is designed to be low power, allowing the microcontroller to operate for extended periods of time on a single battery charge.

The software architecture of this project is based on a simple state machine that manages the different states of the application. The main states include:
- **Sleep State**: The microcontroller is in a low power sleep mode, waiting for an external interrupt to wake it up.
- **Count State**: When the external interrupt occurs, the microcontroller wakes up and increments the counter. If the counter reaches the preset number of pulses (10), it transitions to the Transmit State.
- **Transmit State**: In this state, the microcontroller transmits the counter value over the radio interface to the Raspberry Pi server. 

# Firmware development
The firmware is developed using VSC environment with the MPLAB add-in extension installed. The code is written in C and is structured to follow the state machine architecture described above. The main loop of the application manages the state transitions based on the events that occur (external interrupts and counter value). The code also includes functions for entering sleep mode, incrementing the counter, and transmitting data over the radio interface.

All the important files in the firmware development are located in the `FW/LowPowerCounter/default/mcc` directory. The main application code is in the `main.c` file, while the peripheral initialization and configuration code is in the `mcc.c` file. The project also includes header files for defining constants and function prototypes.

# General rules
1. Build your oun *.md file to keep truck of what is done and what rmains to be done so you can analyze it later and resume the work from there, if needed.
2. Make sure to follow the project structure and coding conventions used in the existing codebase to maintain consistency and readability.

# What I need from you
1. Analyze the project structure and configurations to understand how the firmware is organized and how the different components interact with each other.
2. Analyze the schematic diagram and check the correlation between the hardware components and the firmware code to ensure that the correct pins and peripherals are being used.
3. Implement the missing features in the firmware code, including the counter increment logic, RF communication logic.

# What I will do after you implement the code
After you implement the code, I will test the firmware on the actual hardware to ensure that it conts the external pulses, transfers the the current value of the counter to the Test Raspberry Pi server and gets back to sleep mode until the next pulse is detected.
