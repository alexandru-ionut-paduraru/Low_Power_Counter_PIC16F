
# LowPowerCounter

## Table of Contents
- [Overview](#overview)
- [Structure](#structure)

## Overview
This project is a simple example of a low power counter application for the PIC16F18446 microcontroller. It demonstrates how to use the MPLAB X IDE and the XC8 compiler to create a low power application that counts up to 10 and then resets. This project uses an external interrupt that wakes the microcontroller from sleep mode to increment the counter. The counter value is stored in non-volatile memory, allowing it to retain its value even when the power is removed. This project is intended for educational purposes and can be used as a starting point for more complex low power applications.

After a preset number of pulses (10 in this case), data is transmitted over the radio interface to the Raspberry Pi server. The Raspberry Pi server is responsible for receiving the data and processing it as needed. This project can be used as a basis for developing more complex applications that require low power consumption and wireless communication.

## Structure

| Path                               | Purpose                                                                                                                             |
|------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------|
| _build                             | The [CMake build tree](https://cmake.org/cmake/help/latest/manual/cmake.1.html#introduction-to-cmake-buildsystems), can be deleted. |
| cmake                              | Generated [CMake](https://cmake.org/) files. May be deleted if user.cmake has not been added                                        |
| .vscode                            | See [VSCode](https://code.visualstudio.com/docs/getstarted/settings)                                                                |
| .vscode\settings.json              | Workspace specific settings                                                                                                         |
| .vscode\LowPowerCounter.mplab.json | The MPLAB project file, should not be deleted                                                                                       |
| out                                | Final build artifacts                                                                                                               |

## Hardware Architecture
The hardware architecture of this project consists of a PIC16F18446 microcontroller, which is responsible for counting the pulses and transmitting the data over the radio interface. The microcontroller is connected to an external interrupt source, which wakes it from sleep mode to increment the counter. The counter value is stored in non-volatile memory, allowing it to retain its value even when the power is removed. The microcontroller also has a radio interface that allows it to transmit data to a Raspberry Pi server. The Raspberry Pi server is responsible for receiving the data and processing it as needed. This hardware architecture is designed to be low power, allowing the microcontroller to operate for extended periods of time on a single battery charge.

## Software Architecture
The software architecture of this project is based on a simple state machine that manages the different states of the application. The main states include:
- **Sleep State**: The microcontroller is in a low power sleep mode, waiting for an external interrupt to wake it up.
- **Count State**: When the external interrupt occurs, the microcontroller wakes up and increments the counter. If the counter reaches the preset number of pulses (10), it transitions to the Transmit State.
- **Transmit State**: In this state, the microcontroller transmits the counter value over the radio interface to the Raspberry Pi server. After transmission, it resets the counter and transitions back to the Sleep State.

Implementation details

### Sleep State
In the Sleep State, the microcontroller is configured to enter a low power sleep mode. The external interrupt is set up to wake the microcontroller when a pulse is detected. This allows the microcontroller to conserve power while waiting for events.

the Code snippet for entering sleep mode:
```c
void enterSleepMode() {
    // Configure the microcontroller to enter sleep mode
    SLEEP();
}
```

### Count State
When the external interrupt occurs, the microcontroller wakes up and increments the counter. The counter value is stored in non-volatile memory to ensure it retains its value even when power is removed.
```c    
void incrementCounter() {
    counter++;
    // Store the counter value in non-volatile memory
    storeCounterValue(counter);
}
```

### Transmit State
When the counter reaches the preset number of pulses, the microcontroller transitions to the Transmit State
```c
void transmitData() {
    // Transmit the counter value over the radio interface
    transmitCounterValue(counter);
    // Reset the counter
    counter = 0;
}
```

### The main loop

The main loop of the application manages the state transitions based on the events that occur (external interrupts and counter value).
```c 
int main() {
    while (1) {
        switch (currentState) {
            case SLEEP_STATE:
                enterSleepMode();
                break;
            case COUNT_STATE:
                incrementCounter();
                if (counter >= 10) {
                    currentState = TRANSMIT_STATE;
                }
                break;
            case TRANSMIT_STATE:
                transmitData();
                currentState = SLEEP_STATE;
                break;
        }
    }
    return 0;
}
```

## Auxiliary systems
### Radio Interface
The radio interface is responsible for transmitting the counter value to the Raspberry Pi server. This can be implemented using a variety of wireless communication protocols, such as Bluetooth, Wi-Fi, or Zigbee. The specific implementation will depend on the hardware capabilities of the microcontroller and the requirements of the application.

### Raspberry Pi Server
The Raspberry Pi server is responsible for receiving the data transmitted by the microcontroller and processing it as needed. This can be implemented using a variety of programming languages and frameworks, such as Python with Flask or Node.js with Express. The server can be designed to store the received data in a database, display it on a web interface, or perform any other necessary processing.

### On the board

#### LED indicator
An LED indicator can be used to provide visual feedback on the status of the application. For example, the LED can be turned on when the counter is incremented, but only for a short duration (e.g., 100 milliseconds) to save battery power. This can help users understand the current state of the application and provide a simple way to verify that it is functioning correctly. In the future, this feature can be elliminated to further reduce power consumption, as the LED will no longer be needed once the application is fully tested and deployed.

#### Radio module - power control
The board is fetured with a transistor that cuts off the power supply to the radio module when it is not in use. This allows the microcontroller to further reduce power consumption by completely shutting down the radio module when it is not needed. The transistor can be controlled by a GPIO pin on the microcontroller, allowing it to be turned on and off as needed based on the state of the application. This feature can help extend battery life and improve the overall efficiency of the system.

Unfortunately, the first hardware revision has an error that prevents this transistor to turn off, so the radio module is permanetly powered on. The good news is that the radio chis itself has a low power mode that can be activated using a specific SPI command, which allows the system to still achieve low power consumption despite the hardware issue. In future hardware revisions, this issue can be resolved to allow for even greater power savings.
