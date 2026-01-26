/**
 * Generated Pins header File
 * 
 * @file pins.h
 * 
 * @defgroup  pinsdriver Pins Driver
 * 
 * @brief This is generated driver header for pins. 
 *        This header file provides APIs for all pins selected in the GUI.
 *
 * @version Driver Version  3.0.0
*/

/*
© [2026] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/

#ifndef PINS_H
#define PINS_H

#include <xc.h>

#define INPUT   1
#define OUTPUT  0

#define HIGH    1
#define LOW     0

#define ANALOG      1
#define DIGITAL     0

#define PULL_UP_ENABLED      1
#define PULL_UP_DISABLED     0

// get/set IO_RB7 aliases
#define LED_TRIS                 TRISBbits.TRISB7
#define LED_LAT                  LATBbits.LATB7
#define LED_PORT                 PORTBbits.RB7
#define LED_WPU                  WPUBbits.WPUB7
#define LED_OD                   ODCONBbits.ODCB7
#define LED_ANS                  ANSELBbits.ANSB7
#define LED_SetHigh()            do { LATBbits.LATB7 = 1; } while(0)
#define LED_SetLow()             do { LATBbits.LATB7 = 0; } while(0)
#define LED_Toggle()             do { LATBbits.LATB7 = ~LATBbits.LATB7; } while(0)
#define LED_GetValue()           PORTBbits.RB7
#define LED_SetDigitalInput()    do { TRISBbits.TRISB7 = 1; } while(0)
#define LED_SetDigitalOutput()   do { TRISBbits.TRISB7 = 0; } while(0)
#define LED_SetPullup()          do { WPUBbits.WPUB7 = 1; } while(0)
#define LED_ResetPullup()        do { WPUBbits.WPUB7 = 0; } while(0)
#define LED_SetPushPull()        do { ODCONBbits.ODCB7 = 0; } while(0)
#define LED_SetOpenDrain()       do { ODCONBbits.ODCB7 = 1; } while(0)
#define LED_SetAnalogMode()      do { ANSELBbits.ANSB7 = 1; } while(0)
#define LED_SetDigitalMode()     do { ANSELBbits.ANSB7 = 0; } while(0)
// get/set IO_RC0 aliases
#define REED_TRIS                 TRISCbits.TRISC0
#define REED_LAT                  LATCbits.LATC0
#define REED_PORT                 PORTCbits.RC0
#define REED_WPU                  WPUCbits.WPUC0
#define REED_OD                   ODCONCbits.ODCC0
#define REED_ANS                  ANSELCbits.ANSC0
#define REED_SetHigh()            do { LATCbits.LATC0 = 1; } while(0)
#define REED_SetLow()             do { LATCbits.LATC0 = 0; } while(0)
#define REED_Toggle()             do { LATCbits.LATC0 = ~LATCbits.LATC0; } while(0)
#define REED_GetValue()           PORTCbits.RC0
#define REED_SetDigitalInput()    do { TRISCbits.TRISC0 = 1; } while(0)
#define REED_SetDigitalOutput()   do { TRISCbits.TRISC0 = 0; } while(0)
#define REED_SetPullup()          do { WPUCbits.WPUC0 = 1; } while(0)
#define REED_ResetPullup()        do { WPUCbits.WPUC0 = 0; } while(0)
#define REED_SetPushPull()        do { ODCONCbits.ODCC0 = 0; } while(0)
#define REED_SetOpenDrain()       do { ODCONCbits.ODCC0 = 1; } while(0)
#define REED_SetAnalogMode()      do { ANSELCbits.ANSC0 = 1; } while(0)
#define REED_SetDigitalMode()     do { ANSELCbits.ANSC0 = 0; } while(0)
// get/set IO_RC7 aliases
#define PWR_EN_TRIS                 TRISCbits.TRISC7
#define PWR_EN_LAT                  LATCbits.LATC7
#define PWR_EN_PORT                 PORTCbits.RC7
#define PWR_EN_WPU                  WPUCbits.WPUC7
#define PWR_EN_OD                   ODCONCbits.ODCC7
#define PWR_EN_ANS                  ANSELCbits.ANSC7
#define PWR_EN_SetHigh()            do { LATCbits.LATC7 = 1; } while(0)
#define PWR_EN_SetLow()             do { LATCbits.LATC7 = 0; } while(0)
#define PWR_EN_Toggle()             do { LATCbits.LATC7 = ~LATCbits.LATC7; } while(0)
#define PWR_EN_GetValue()           PORTCbits.RC7
#define PWR_EN_SetDigitalInput()    do { TRISCbits.TRISC7 = 1; } while(0)
#define PWR_EN_SetDigitalOutput()   do { TRISCbits.TRISC7 = 0; } while(0)
#define PWR_EN_SetPullup()          do { WPUCbits.WPUC7 = 1; } while(0)
#define PWR_EN_ResetPullup()        do { WPUCbits.WPUC7 = 0; } while(0)
#define PWR_EN_SetPushPull()        do { ODCONCbits.ODCC7 = 0; } while(0)
#define PWR_EN_SetOpenDrain()       do { ODCONCbits.ODCC7 = 1; } while(0)
#define PWR_EN_SetAnalogMode()      do { ANSELCbits.ANSC7 = 1; } while(0)
#define PWR_EN_SetDigitalMode()     do { ANSELCbits.ANSC7 = 0; } while(0)
/**
 * @ingroup  pinsdriver
 * @brief GPIO and peripheral I/O initialization
 * @param none
 * @return none
 */
void PIN_MANAGER_Initialize (void);

/**
 * @ingroup  pinsdriver
 * @brief Interrupt on Change Handling routine
 * @param none
 * @return none
 */
void PIN_MANAGER_IOC(void);


#endif // PINS_H
/**
 End of File
*/