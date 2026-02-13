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
#include "../mssp1.h"

// Set the MSSP1 module to the options selected in the user interface.
void MSSP1_Initialize(void) {
    // SMP High Speed; CKE disabled; 
    SSP1STAT = 0x0;
    // SSPEN disabled; CKP Idle:Low, Active:High; SSPM FOSC/4; 
    SSP1CON1 = 0x0;
    // GCEN disabled; ACKDT acknowledge; ACKEN disabled; RCEN disabled; PEN disabled; RSEN disabled; SEN disabled; 
    SSP1CON2 = 0x0;
    // PCIE disabled; SCIE disabled; BOEN disabled; SDAHT 100ns; SBCDE disabled; AHEN disabled; DHEN disabled; 
    SSP1CON3 = 0x0;
    // SSPMSK 0xff; 
    SSP1MSK = 0xFF;
    // SSPBUF 0x0; 
    SSP1BUF = 0x0;
    // SSPADD 0x0; 
    SSP1ADD = 0x0;
}
