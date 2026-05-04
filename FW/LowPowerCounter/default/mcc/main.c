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
#include "mcc_generated_files/spi/mssp2.h"
#include "mcc_generated_files/system/interrupt.h"
#include "mcc_generated_files/system/pins.h"
#include "mcc_generated_files/system/system.h"
#include "mcc_generated_files/timer/tmr1.h"
#include "mcc_generated_files/power/power.h"
#include "mcc_generated_files/timer/tmr2_deprecated.h"
#include "nrf24l01.h"
/*
    Main application
*/
uint8_t dataSent=0;
uint8_t tmr2_event=0;
nrf24l01_device_t radio_dev; // Global instance of the nRF24L01 device structure
/***********************************************************************
    Radio module nRF24L01 functions
***********************************************************************/



void TMR1_CB(void){
    static uint16_t led_count=0;
    led_count++;
    if(led_count>=10){
        led_count=0;

        // TO DO - delete the fallowing test lines of code
        dataSent=1;
    }
}

void TMR2_CB(void){
    tmr2_event=1;
    POWER_PeripheralDisable(POWER_TMR2);
}

void INT_CB(void){
    POWER_PeripheralEnable(POWER_TMR1);
    // LED_Toggle();
}

void NRF24L01_CSN_Set(bool level){
    if (level){
        CSN_SetHigh();
    }else{
        CSN_SetLow(); 
    }
}

void NRF24L01_CE_Set(bool level){
    if (level){
        CE_SetHigh();
    }else{
        CE_SetLow(); 
    }
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

    //initialize nrf24l01 device structure with user defined SPI and pin control functions
    nrf24l01_spi_transfer_fn_register(&radio_dev, &SPI2_BufferExchange);
    nrf24l01_pin_set_fn_csn_register(&radio_dev, &NRF24L01_CSN_Set); // Example: CSN pin active low
    nrf24l01_pin_set_fn_ce_register(&radio_dev, &NRF24L01_CE_Set); // Example: CE pin active high
    
    POWER_PeripheralDisableAll();
    POWER_PeripheralEnable(POWER_TMR1);
    POWER_PeripheralEnable(POWER_TMR2);
    POWER_PeripheralEnable(POWER_IOC);
    TMR1_TMRInterruptEnable();
    TMR1_OverflowCallbackRegister(&TMR1_CB);
    EXT_INT_InterruptEnable();
    INT_SetInterruptHandler(&INT_CB);
    TMR1_Start();
    TMR2_OverflowCallbackRegister(&TMR2_CB);
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
                }else{
                    POWER_PeripheralEnable(POWER_TMR2);
                    TMR2_Start();
                    state=5; //prevent next pulse within 30ms
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
            case 5:{ //wait timer 2 to expire
                // POWER_LowPowerModeEnter();
                //when exiting power mode
                if (tmr2_event==1){
                    tmr2_event=0;
                    state=1;
                }
                break;
            }
        }
    }    
}
