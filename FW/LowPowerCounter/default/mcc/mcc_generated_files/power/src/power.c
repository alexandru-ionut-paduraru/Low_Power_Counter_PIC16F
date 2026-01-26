/**
 * POWER Generated Driver File
 * 
 * @file power.c
 * 
 * @ingroup power
 * 
 * @brief This is the generated driver implementation file for the POWER driver.
 *
 * @version POWER Driver Version 1.1.0
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

/**
  Section: Included Files
*/
#include <xc.h>
#include "../power.h"

/**
  Section: POWER Macros
 */
#define SETTINGDATA_MASK (uint8_t)0xFF

/**
  Section: POWER APIs
*/
void POWER_LowPowerModeEnter(void) {

    SLEEP();
    NOP();
}


bool POWER_PeripheralEnable(power_peripheral_t peripheral) {
    uint8_t registerData = (uint8_t)((uint16_t)peripheral >> 8);
    uint8_t settingData = (uint8_t)((uint16_t)peripheral & SETTINGDATA_MASK);
    bool error = true;

    switch (registerData) {
        case 0:
            PMD0 &= ~(settingData);
            error = false;
            break;
        case 1:
            PMD1 &= ~(settingData);
            error = false;
            break;
        case 2:
            PMD2 &= ~(settingData);
            error = false;
            break;
        case 3:
            PMD3 &= ~(settingData);
            error = false;
            break;
        case 4:
            PMD4 &= ~(settingData);
            error = false;
            break;
        case 5:
            PMD5 &= ~(settingData);
            error = false;
            break;
        default:
            error = true; // Handle unexpected registerData values if necessary
            break;
    }
    return error;
}

bool POWER_PeripheralDisable(power_peripheral_t peripheral) {
    uint8_t registerData = (uint8_t)((uint16_t)peripheral >> 8);
    uint8_t settingData = (uint8_t)((uint16_t)peripheral & SETTINGDATA_MASK);
    bool error = true;

    switch (registerData) {
        case 0:
            PMD0 |= (settingData);
            error = false;
            break;
        case 1:
            PMD1 |= (settingData);
            error = false;
            break;
        case 2:
            PMD2 |= (settingData);
            error = false;
            break;
        case 3:
            PMD3 |= (settingData);
            error = false;
            break;
        case 4:
            PMD4 |= (settingData);
            error = false;
            break;
        case 5:
            PMD5 |= (settingData);
            error = false;
            break;
        default:
            error = true; // Handle unexpected registerData values if necessary
            break;
    }
    return error;
}

void POWER_PeripheralDisableAll(void) {
        PMD0 = (0xFF);
        PMD1 = (0xFF);
        PMD2 = (0xFF);
        PMD3 = (0xFF);
        PMD4 = (0xFF);
        PMD5 = (0xFF);
}
/**
 End of File
*/
