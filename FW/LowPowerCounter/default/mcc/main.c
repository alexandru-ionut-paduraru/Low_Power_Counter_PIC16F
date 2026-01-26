/*
 * MAIN Generated Driver File
 * 
 * @file main.c
 * 
 * @defgroup main MAIN
 * 
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.0
*/

/*
� [2026] Microchip Technology Inc. and its subsidiaries.

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
#include "mcc_generated_files/system/interrupt.h"
#include "mcc_generated_files/system/pins.h"
#include "mcc_generated_files/system/system.h"
#include "mcc_generated_files/timer/tmr1.h"
#include "mcc_generated_files/power/power.h"

/*
    Main application
*/
uint8_t dataSent=0;

void TMR1_CB(void){
    static uint16_t led_count=0;
    led_count++;
    if(led_count>=10){
        led_count=0;

        // TO DO - delete the fallowing test lines of code
        dataSent=1;
    }
}

void INT_CB(void){
    POWER_PeripheralEnable(POWER_TMR1);
    // LED_Toggle();
}

int main(void)
{
    static uint8_t state=0;
    static uint32_t counter=0;
    static uint32_t sendData_counter=0;
    SYSTEM_Initialize();

    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts 
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global and Peripheral Interrupts 
    // Use the following macros to: 

    // Enable the Global Interrupts 
    INTERRUPT_GlobalInterruptEnable(); 

    // Disable the Global Interrupts 
    //INTERRUPT_GlobalInterruptDisable(); 

    // Enable the Peripheral Interrupts 
    INTERRUPT_PeripheralInterruptEnable(); 

    // Disable the Peripheral Interrupts 
    //INTERRUPT_PeripheralInterruptDisable(); 

    TMR1_TMRInterruptEnable();
    TMR1_OverflowCallbackRegister(&TMR1_CB);
    EXT_INT_InterruptEnable();
    INT_SetInterruptHandler(&INT_CB);
    TMR1_Start();
    LED_SetHigh();
    while(1)
    {
        switch(state){
            case 0:{//wait to power off the LED
                if(dataSent==1){
                    LED_SetLow();
                    dataSent=0; //flag clear
                    state=1;
                }
                break;
            }
            case 1:{ //Initial State
                POWER_PeripheralDisable(POWER_TMR1);
                POWER_LowPowerModeEnter();
                //the program will continue when a pulse is received
                counter++;
                sendData_counter++;
                if (sendData_counter>=5){
                    sendData_counter=0;
                    state=2;
                }
                break;
            }
            case 2:{ //send data
                LED_SetHigh();
                TMR1_Start();
                state=3;
                break;
            }
            case 3:{//wait data transmission to end
                if(dataSent==1){
                    LED_SetLow();
                    dataSent=0; //flag clear
                    state=1;
                }
                break;
            }
        }
    }    
}