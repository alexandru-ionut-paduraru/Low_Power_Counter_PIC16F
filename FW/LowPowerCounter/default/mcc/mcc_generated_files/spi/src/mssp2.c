/**
 * SPI2 Generated Driver File
 *
 * @file mssp2.c
 *
 * @ingroup spi2
 *
 * @brief This file contains the driver code for the SPI2 module.
 *
 * @version SPI2 Driver Version v5.0.0
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

#include <xc.h>
#include "../mssp2.h"
#include "../spi_polling_types.h"

const struct SPI_INTERFACE SPI2_Client = {
    .Initialize = SPI2_Initialize,
    .Deinitialize = SPI2_Deinitialize,
    .Open = SPI2_Open,
    .Close = SPI2_Close,
    .BufferExchange = SPI2_BufferExchange,
    .BufferWrite = SPI2_BufferWrite,
    .BufferRead = SPI2_BufferRead,
    .ByteExchange = SPI2_ByteExchange,
    .ByteWrite = SPI2_ByteWrite,
    .ByteRead = SPI2_ByteRead,
    .IsRxReady = SPI2_IsRxReady,
    .IsTxReady = SPI2_IsTxReady,
    .RxCompleteCallbackRegister = NULL,
    .TxCompleteCallbackRegister = NULL
};

static const spi_configuration_t spi2_configuration[] = {
    { 0x0, 0x5, 0x10, 0x01 },
    { 0x64, 0x5, 0x10, 0x0 }
};

void SPI2_Initialize(void)
{
    // Return mssp2 registers to reset state
    PIE2bits.SSP2IE  = 0;
    PIR2bits.SSP2IF    = 0;

    SSP2STAT = 0x00;
    SSP2CON1 = 0x00;
    SSP2CON3 = 0x00;
    SSP2ADD  = 0x00;
}

void SPI2_Deinitialize(void)
{
    // Return mssp2 registers to reset state
    SSP2STAT = 0x00;
    SSP2CON1 = 0x00;
    SSP2CON3 = 0x00;
    SSP2ADD  = 0x00;
}

bool SPI2_Open(uint8_t spiConfigIndex)
{
    bool returnValue = false;
    if (SSP2CON1bits.SSPEN == false)
    {
        SSP2STAT = spi2_configuration[spiConfigIndex].stat;
        SSP2CON1 = spi2_configuration[spiConfigIndex].con1;
        SSP2CON3 = spi2_configuration[spiConfigIndex].con3;
        SSP2ADD  = spi2_configuration[spiConfigIndex].baud;

        SSP2CON1bits.SSPEN = 1;

        returnValue = true;
    }
    else
    {
        returnValue = false;
    }
    return returnValue;
}

void SPI2_Close(void)
{
    SSP2CON1bits.SSPEN = 0;
}

void SPI2_BufferExchange(void *bufferData, size_t bufferSize)
{
    uint8_t *bufferInput = bufferData;
    size_t bufferInputSize = bufferSize;
    while (0U != bufferInputSize)
    {
        SSP2BUF = *bufferInput;
        while (!PIR2bits.SSP2IF)
        {
            // Wait for flag to get set
        }
        PIR2bits.SSP2IF = 0;
        *bufferInput = SSP2BUF;
        bufferInput++;
        bufferInputSize--;
    }
}

void SPI2_BufferWrite(void *bufferData, size_t bufferSize)
{
    uint8_t *bufferInput = bufferData;
    size_t bufferInputSize = bufferSize;
    while (0U != bufferInputSize)
    {
        SSP2BUF = *bufferInput;
        while (!PIR2bits.SSP2IF)
        {
            // Wait for flag to get set
        }
        PIR2bits.SSP2IF = 0;
        bufferInput++;
        bufferInputSize--;
    }
}

void SPI2_BufferRead(void *bufferData, size_t bufferSize)
{
    uint8_t *bufferInput = bufferData;
    size_t bufferInputSize = bufferSize;
    while (0U != bufferInputSize)
    {
        SSP2BUF = 0x00;
        while (!PIR2bits.SSP2IF)
        {
            // Wait for flag to get set
        }
        PIR2bits.SSP2IF = 0;
        *bufferInput = SSP2BUF;
        bufferInput++;
        bufferInputSize--;
    }
}

uint8_t SPI2_ByteExchange(uint8_t byteData)
{
    SSP2BUF = byteData;
    while (!PIR2bits.SSP2IF)
    {
        // Wait for flag to get set
    }
    PIR2bits.SSP2IF = 0;
    return SSP2BUF;
}

void SPI2_ByteWrite(uint8_t byteData)
{
    SSP2BUF = byteData;
}

uint8_t SPI2_ByteRead(void)
{
    if (1U == PIR2bits.SSP2IF)
    {
        PIR2bits.SSP2IF = 0;
    }
    return SSP2BUF;
}

bool SPI2_IsRxReady(void)
{
    bool returnValue = false;
    if (SSP2CON1bits.SSPEN == 1)
    {
        returnValue = ((PIR2bits.SSP2IF != 0U) ? true: false);
    }
    else
    {
        returnValue = false;
    }
    return returnValue;
}

bool SPI2_IsTxReady(void)
{
    bool returnValue = false;
    if (SSP2CON1bits.SSPEN == 1)
    {
        returnValue = ((PIR2bits.SSP2IF != 0U) ? false: true);
    }
    else
    {
        returnValue = false;
    }
    return returnValue;
}