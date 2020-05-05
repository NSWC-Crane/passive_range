/* 
 * File:   trigger_main.c
 * Author: david.r.emerson
 *
 * Created on April 08, 2020
 */



// PIC12F1822 Configuration Bit Settings
// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable (Brown-out Reset disabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = OFF       // Internal/External Switchover (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = ON       // PLL Enable (4x PLL enabled)
#pragma config STVREN = OFF     // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will not cause a Reset)
#pragma config BORV = HI        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), high trip point selected.)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

#include <xc.h>

#include "../include/trigger.h"
#include "../include/pic12f1822_config.h"


/*************************Declare Global Variables*****************************/

unsigned char data_ready = 0;
unsigned char rx_data[PKT_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0};

unsigned char t1_polarity = 0;
unsigned short t1_offset = 0;
unsigned short t1_length = 50000;
unsigned char t1_out = 0;

unsigned char t2_polarity = 0;
unsigned short t2_offset = 0;
unsigned short t2_length = 50000;
unsigned char t2_out = 0;

//unsigned char t3_polarity = 0;
//unsigned short t3_offset = 0;
//unsigned short t3_length = 50000;
//unsigned char t3_out = 0;
    
const unsigned short trigger_interval = 50000;
const unsigned char firmware[2] = {1, 0};
const unsigned char serial_num[1] = {2};

/***************************Interrupt Definition*******************************/
void __interrupt () ISR(void)
{
    unsigned char idx;
    unsigned char tmp_rx;
    
    // check for the UART Rx interrupt to trigger
    if(PIR1bits.RCIF == 1)
    {       
        TRIG3 = 0;

        // disable interrupt
        PIE1bits.RCIE = 0;
        
        tmp_rx = RCREG;
        if(RCSTAbits.OERR == 1)
        {
            RCSTAbits.CREN = 0;
            asm("nop");
            //rx_data[0] = RCREG;
            RCSTAbits.CREN = 1;            
        }
        
        // check the first byte to make sure that it is a valid command byte
        if(tmp_rx == '$')
        {
            // read in the command byte
            while(PIR1bits.RCIF == 0);
            rx_data[0] = RCREG;
            if(RCSTAbits.OERR == 1)
            {
                RCSTAbits.CREN = 0;
                asm("nop");
                //rx_data[0] = RCREG;
                RCSTAbits.CREN = 1;            
            }

            // read in the byte count
            while(PIR1bits.RCIF == 0);
            rx_data[1] = RCREG;
            if(RCSTAbits.OERR == 1)
            {
                RCSTAbits.CREN = 0;
                asm("nop");
                //rx_data[1] = RCREG;
                RCSTAbits.CREN = 1;            
            }
            
            // expect to receive a total of rx_data[1] characters in the packet
            for(idx = 0; idx < rx_data[1]; ++idx)
            {
                // wait for the next character to be received
                while(PIR1bits.RCIF == 0);
                
                rx_data[idx+2] = RCREG;
                if(RCSTAbits.OERR == 1)
                {
                    RCSTAbits.CREN = 0;
                    asm("nop");
                    //rx_data[idx+2] = RCREG;
                    RCSTAbits.CREN = 1;            
                }                
            }
            data_ready = 1;
            
        }
        else
        {
            // re-enable the UART Rx interrupt
            PIE1bits.RCIE = 1;          // enable UART Rx Interrupt                      
        }
        
        TRIG3 = 1;
    }   // end of PIR1bits.RCIF

    // re-enable the interrupt
    //PIE1bits.RCIE = 1;
    
}   // end of ISR


/*******************************Main Routine***********************************/
void main(void)
{
    unsigned char idx;
    unsigned char length;
    unsigned char temp;
    unsigned char packet_data[PKT_SIZE] = {0};
       
    register_config();                  // configure all registers
    timer_config();                     // configure TIMER1 and TIMER2
    config_uart();                      // configure the UART


    TRISA = 0b00000010;                 // Set these pins according to the data direction

    TRIG1 = 0;
    TRIG2 = 0;
    TRIG3 = 1;

    for(idx=0; idx<200; ++idx)
        send_char('a');
    
/**********************************MAIN LOOP**********************************/
    while(1)
    {

        if(data_ready == 1)
        {

            // This is the main command number
            switch(rx_data[0])
            {       
/**************************Connection Operations*******************************/
                        
                // send back a connected message
                case CONNECT:
                    length = 4;
                    packet_data[0] = 1;
                    packet_data[1] = serial_num[0];
                    packet_data[2] = firmware[0];
                    packet_data[3] = firmware[1];
                    send_packet(CONNECT, length, packet_data);
                    break;  

                // read the control firmware
                case FIRM_READ:
                    length = 2;
                    send_packet(FIRM_READ, length, firmware);
                    break;  

                // read the controller serial number
                case SER_NUM_READ:
                    length = 1;
                    send_packet(SER_NUM_READ, length, serial_num);
                    break;
                        
                
                case CONFIG_T1:
                    length = 1;

                    t1_polarity = rx_data[2] & 0x01;
                    t1_offset = rx_data[3]<<8 | rx_data[4];
                    t1_length = rx_data[5]<<8 | rx_data[6];
                    TRIG1 = 0 ^ t1_polarity;
                    
                    packet_data[0] = t1_polarity;
                    send_packet(CONFIG_T1, length, packet_data);
                    break;                
                
                case CONFIG_T2:
                    length = 1;

                    t2_polarity = rx_data[2] & 0x01;
                    t2_offset = rx_data[3]<<8 | rx_data[4];
                    t2_length = rx_data[5]<<8 | rx_data[6];
                    TRIG2 = 0 ^ t2_polarity;
                    
                    packet_data[0] = t2_polarity;
                    send_packet(CONFIG_T2, length, packet_data);
                    break;                  

//                case CONFIG_T3:
//                    length = 1;
//                    
//                    t3_polarity = rx_data[2] & 0x01;
//                    t3_offset = rx_data[3]<<8 | rx_data[4];
//                    t3_length = rx_data[5]<<8 | rx_data[6];
//                    TRIG3 = 0 ^ t3_polarity;
//                    
//                    packet_data[0] = t3_polarity;
//                    send_packet(CONFIG_T3, length, packet_data);
//                    break; 
                    
                case TRIG_INIT:
                    length = 1;
                    
                    initiate_trigger();
                    
                    packet_data[0] = 1;
                    send_packet(TRIG_INIT, length, packet_data);
                    break;
                    
                case TRIG_CH1:
                    length = 1;
                    
                    TRIG1 = 0 ^ t1_polarity;
                    TMR1 = 0;
                    while(TMR1 < trigger_interval)
                    {
                        t1_out = ((TMR1 >= t1_offset) && (TMR1 <= t1_length));
                        TRIG1 =  t1_out ^ t1_polarity;
                    }                   
                    TRIG1 = 0 ^ t1_polarity;
                    
                    packet_data[0] = 1;
                    send_packet(TRIG_CH1, length, packet_data);                    
                    break;
                    
                case TRIG_CH2:
                    length = 1;
                    
//                    TMR1 = 0;
//                    TRIG2 = 1;                   
//                    while(TMR1 < 50000);
//                    
//                    TMR1 = 0;                    
//                    TRIG2 = 0;
//                    while(TMR1 < 50000);
//                    
                    TRIG2 = 0 ^ t2_polarity;
                    TMR1 = 0;
                    while(TMR1 < trigger_interval)
                    {
                        t2_out = ((TMR1 >= t2_offset) && (TMR1 <= t2_length));
                        TRIG2 =  t2_out ^ t2_polarity;
                    }                   
                    TRIG2 = 0 ^ t2_polarity;                    
                    
                    packet_data[0] = 1;
                    send_packet(TRIG_CH2, length, packet_data); 
                break;
                    
            }   // end of switch(rx_data[0])
            
            data_ready = 0;
            
            // do a dummy read of the RCREG before enabling the interrupt
            //temp = RCREG;
            //temp = RCREG;

            PIE1bits.RCIE = 1;          // enable UART Rx Interrupt
            
        }   // end of if(data_ready == 1)      

    }   // end of while(1)

}   // end of main



/**********************************Functions***********************************/

/* Function: void send_char(unsigned char c)
 *
 * Arguments: 
 *  1. c: single byte of data
 *
 * Return Value: None
 *
 * Description: Function to send data out of UART2 to the user */
void send_char(unsigned char c)
{
    while(!TXSTAbits.TRMT);         // wait for at least one buffer slot to be open
    TXREG = c;
}   // end of send_char

//-----------------------------------------------------------------------------
/* Function: void send_packet(unsigned char code)
 *
 * Arguments:
 * 1. command: command code to be sent
 * 2. length: length of the data to be sent
 * 3. *data: pointer to the data to be sent
 *
 * Return Value: None
 *
 * Description: Send message with command header, packet byte size and data */
void send_packet(unsigned char command, unsigned short length, unsigned char data[])
{
    unsigned short idx;
    
    send_char(command);
    send_char(length);

    for(idx=0; idx<length; ++idx)                 // send data and perform CRC calculations
    {
        send_char(data[idx]);
    }

}   // end of send_packet


void initiate_trigger(void)
{
    TRIG1 = 0 ^ t1_polarity;
    TRIG2 = 0 ^ t2_polarity;
    
    // reset the counter
    TMR1 = 0;  
    
    while(TMR1 < trigger_interval)
    {
        t1_out = ((TMR1 >= t1_offset) && (TMR1 <= t1_length));
        TRIG1 = t1_out ^ t1_polarity;
        
        t2_out = ((TMR1 >= t2_offset) && (TMR1 <= t2_length));
        TRIG2 = t2_out ^ t2_polarity;
               
//        t3_out = ((TMR1 >= t3_offset) && (TMR1 <= t3_length));
//        TRIG3 = t3_out ^ t3_polarity;      
    }
    
    TRIG1 = 0 ^ t1_polarity;
    TRIG2 = 0 ^ t2_polarity;
    
}   // end of initiate_trigger