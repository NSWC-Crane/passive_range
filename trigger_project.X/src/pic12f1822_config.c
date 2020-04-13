#include <xc.h>
#include <stdio.h>
#include "../include/pic12f1822_config.h"

/* Function: void register_config(void)
 *
 * Arguments:
 * 1. None
 *
 * Return Value:
 * 1. None
 *
 * Description: Code to handle the initial register configuration of the PIC */
void register_config(void)
{
    //OSCCON: Sets internal oscillator to 8 MHz - pg69
    OSCCON = 0x00;
    OSCCONbits.SPLLEN = 1;
    OSCCONbits.IRCF = 0x0E;
        
    
    OPTION_REG = 0b10000101;            // Turns internal pull up resistors off - pg175
    OPTION_REG = 0x00;
    OPTION_REGbits.nWPUEN = 1;
    
    // Interrupts: Enable global & peripheral interrupts - pg91
    INTCON = 0x00;               
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    
    // PIE1: Enable USART Receive Interrupt - pg92   
    PIE1 = 0x00;
    PIE1bits.RCIE = 1;
    PIE2 = 0x00;
    
    WDTCON = 0x00;                      // Watchdog Timer OFF - pg103
    
    APFCON = 0x00;                      // TX/CK is on RA0, RX/DT is on RA1 - pg121
    
    ANSELA = 0b00000000;                // Sets all Port A pins to digital mode - pg125
    
    WPUA = 0b00000000;                  // Port A Weak pull-up resistor - 0=disabled - pg126
    FVRCON = 0b00000000;                // Fixed Voltage Ref Ctrl - pg135
    ADCON0 = 0b00000000;                // Sets A/D parameters - ADC Off - pg145
    CPSCON0 = 0b00000000;               // Capacitive Touch disable - pg319
    
    CCP1CON = 0x00;                     // Capture/Compare/PWM off - pg226


}   // end of register_config


/* Function: void timer_config(void)
 *
 * Arguments:
 * 1. None
 *
 * Return Value:
 * 1. None
 *
 * Description: Code to configure Timers for delays on the PIC */
void timer_config(void)
{
    
    // T1CON: Timer on - pg185
    T1CONbits.TMR1CS = 0;               // Source = FOSC/4
    T1CONbits.T1CKPS = 3;               // Prescaler = 1:8, Each increment is 1us w/32MHz
    T1CONbits.T1OSCEN = 0;
    T1CONbits.nT1SYNC = 0;
    T1CONbits.TMR1ON = 1;
    
    T1GCON = 0b00000001;                // Timer0 overflow output - pg186
    TMR1IF = 0;                         // No Overflow
    TMR1 = 0;                           // Reset Counter

    T2CON = 0x00;
    //T2CON = 0b00000111;                 // Timer2 is on, Prescaler is 64 - pg190
    PR2 = 159;                          // Set period register for 5.12ms for FOSC = 8MHz - Figure 22-1

}   // end of timer_config


void config_uart(void)
{
    // TXSTA: Transmit enabled, 8-bit mode, High speed baud rate - pg294
    TXSTAbits.CSRC = 0;
    TXSTAbits.TX9 = 0;
    TXSTAbits.TXEN = 1;
    TXSTAbits.SYNC = 0;
    TXSTAbits.SENDB = 0;
    TXSTAbits.BRGH = 1;

    // RCSTA: Serial port enabled, 8-bit reception, receiver enabled - pg295
    RCSTAbits.SPEN = 1;
    RCSTAbits.RX9 = 0;
    RCSTAbits.SREN = 0;
    RCSTAbits.CREN = 1;
    RCSTAbits.ADDEN = 0;
    
    // BAUDCON: non-inverted Tx data, 16-bit Baud Rate Generator - pg296
    BAUDCONbits.SCKP = 0;
    BAUDCONbits.BRG16 = 1;
    BAUDCONbits.WUE = 0;
    BAUDCONbits.ABDEN = 0;
    
    SP1BRG = 31;                        // Fosc = 32MHz, baudrate = Fosc/(4(n+1))
                                        // 79 => 100,000
                                        // 39 => 200,000
                                        // 31 => 250,000
    
    
}