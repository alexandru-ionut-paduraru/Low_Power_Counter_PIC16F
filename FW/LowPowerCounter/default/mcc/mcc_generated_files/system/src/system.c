/**
 * System Driver Source File
 * 
 * @file system.c
 * 
 * @ingroup systemdriver
 * 
 * @brief This file contains the API implementation for the System driver.
 *
 * @version Driver Version 1.0.1
 *
 * @version Package Version 1.0.1
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

#include "../system.h"

/** 
* @ingroup systemdriver
* @brief Initializes the Peripheral Module Disable (PMD) module.
* @param None.
* @return None.
*/
void PMD_Initialize(void);


void SYSTEM_Initialize(void)
{
    CLOCK_Initialize();
    PIN_MANAGER_Initialize();
    TMR1_Initialize();
    TMR2_Initialize();
    EUSART_Initialize();
    MSSP1_Initialize();
    PMD_Initialize();
    POWER_Initialize();
    SPI2_Initialize();
    INTERRUPT_Initialize();
}

void PMD_Initialize(void)
{
    //IOCMD IOC enabled; CLKRMD CLKR enabled; NVMMD NVM enabled; FVRMD FVR enabled; SYSCMD SYSCLK enabled; 
    PMD0 = 0x0;
    //TMR0MD TMR0 enabled; TMR1MD TMR1 enabled; TMR2MD TMR2 disabled; NCOMD DDS(NCO) enabled; TMR3MD TMR3 enabled; TMR4MD TMR4 enabled; TMR5MD TMR5 enabled; TMR6MD TMR6 enabled; 
    PMD1 = 0x4;
    //CMP1MD CMP1 enabled; ADCMD ADC enabled; CMP2MD CMP2 enabled; DACMD DAC enabled; 
    PMD2 = 0x0;
    //CCP1MD CCP1 enabled; CCP2MD CCP2 enabled; PWM5MD PWM5 enabled; PWM6MD PWM6 enabled; CWG1MD CWG1 enabled; CCP3MD CCP3 enabled; CCP4MD CCP4 enabled; CWG2MD CWG2 enabled; 
    PMD3 = 0x0;
    //MSSP1MD MSSP1 enabled; UART1MD EUSART enabled; MSSP2MD MSSP2 enabled; 
    PMD4 = 0x0;
    //CLC1MD CLC1 enabled; CLC2MD CLC2 enabled; CLC3MD CLC3 enabled; CLC4MD CLC4 enabled; DSMMD DSM enabled; 
    PMD5 = 0x0;
}

