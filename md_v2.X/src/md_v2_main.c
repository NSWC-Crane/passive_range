/* 
 * File:   newmain.c
 * Author: owner
 *
 * Created on August 17, 2020, 3:16 PM
 */

// ----------------------------------------------------------------------------
// Configuration Bits
#pragma config CP = OFF, BWP = OFF, PWP = OFF, ICESEL = ICS_PGx2, DEBUG = OFF
#pragma config FWDTEN = OFF, WDTPS = PS4096, FCKSM = CSDCMD, FPBDIV = DIV_1, OSCIOFNC = OFF, POSCMOD = HS, IESO = OFF, FSOSCEN = OFF, FNOSC = PRIPLL
#pragma config FPLLODIV = DIV_1, UPLLEN = OFF, UPLLIDIV = DIV_1, FPLLMUL = MUL_20, FPLLIDIV = DIV_2
#pragma config FVBUSONIO = OFF, FUSBIDIO = OFF, FMIIEN = ON, FSRSSEL = PRIORITY_7

#define _SUPPRESS_PLIB_WARNING 1

//#include <stdio.h>
//#include <stdlib.h>
#include <xc.h>
#include <plib.h>

#include "../include/md.h"
#include "../include/pic32mx695f_config.h"
#include "../include/uart_ctrl.h"



// ----------------------------------------------------------------------------
// Global Definitions
// ----------------------------------------------------------------------------
unsigned char rx_data[PKT_SIZE] = {0};
unsigned char data_ready = 0;
const unsigned char firmware[2] = {0,50};
const unsigned char serial_num[1] = {1};

int current_focus_step = 0;
int current_zoom_step = 0;

const int max_focus_step = 40575;
const int max_zoom_step = 4628;
const int min_step = 0;


// ----------------------------------------------------------------------------
// Interrupt Definitions
// ----------------------------------------------------------------------------
void __ISR(32,IPL4) UART2_Rx(void)
{
    char tmp_data = 0;

    tmp_data = get_char(2);

    if(tmp_data == '$')
    {
        data_ready = 1;
        rx_data[0] = get_char(2);
        rx_data[1] = get_char(2);
        //rx_data[2] = get_U2_char();
    }

    mU2RXClearIntFlag();
}   // end of UART2 interrupt


// ----------------------------------------------------------------------------
int main(int argc, char** argv) 
{
    int idx;
    int length = 0;

    unsigned char temp;
    unsigned char packet_data[PKT_SIZE] = {0};
    
    if(RCON & 0x18)             // The WDT caused a wake from Sleep
    {
        asm volatile("eret");   // return from interrupt
    }
    
    DDPCONbits.JTAGEN=0;        // disable JTAG

    init_PRECACHE();            // setup wait states
    init_CLOCK();               // Setup and Initialize Main and Secondary Clock
    init_ADC();                 // Setup and initialize ADC Module
    //init_RTCC();                // Setup and Initialize RTCC
    //init_ETH();                 // Setup Ethernet module
    init_TMR1();              // Setup and Initialize Timer Modules
    //init_Comparator();          // Setup Comparator Module
    
    init_UART1();                // Setup and Initialize UART
    init_UART2();                // Setup and Initialize UART
    //init_SPI3();                // Setup and Initialize SPI3 Module

    // TRIS Configurations
    TRISB = 0x00FAEB;             // RB2, RB4, RB8 & RB10 set to outputs
    TRISE = 0x00FFE3;             // RE2, RE3 & RE4 => outputs
    TRISF = 0x00FFF7;             // RF3 => output
    TRISG = 0x00FFBF;             // RG6 => output
    
    // make sure that the motor is disabled on startup
    MOT_EN_PIN = 1;

    // configure interrupt sources
    mU2SetIntPriority(4);       // configure UART2 Interrupt priority 4
    //mT2SetIntPriority(6);       // Configure Timer2 Interrupt priority to 6
    //mT3SetIntPriority(7);       // Configure Timer3 Interrupt priority to 7
	
    //mU2RXClearIntFlag();
    //mT3ClearIntFlag();
    //mT2ClearIntFlag();
    
    INTEnableSystemMultiVectoredInt();

    // Configure LEDs
    Green_LED = 0;                          // turn off green LED
    Blue_LED = 1;                           // turn on blue led

    for(idx=0; idx<5; ++idx)              	// wait 250ms to begin
    {
        delay_ms(50);
        
        Blue_LED = ~Blue_LED;
        Green_LED = ~Green_LED;
    }
    
    // clear out the UART2 interrupts and enable
    temp = U2RXREG;
    mU2RXClearIntFlag();
    mU2RXIntEnable(1);          // enable UART Rx interrupt

// ----------------------------------------------------------------------------
//                               MAIN LOOP 
// ----------------------------------------------------------------------------
    while(1)
    {   
    
       if(data_ready == 1)
       {
           mU2RXIntEnable(0);          // disable UART Rx interrupt

           // read in the remaining data bytes
           for(idx=0; idx<rx_data[1]; ++idx)
           {
               rx_data[2+idx] = get_char(2);
           }

           // This is the main command number
           switch(rx_data[0])
           {
               
               
               
               
               
// ----------------------------------------------------------------------------
/*************************Engineering Operations******************************/
// ----------------------------------------------------------------------------
                        
                    // read the control firmware
                    case FIRM_READ:
                        length = 2;
                        send_packet(2, FIRM_READ, length, firmware);
                        break;  

                    // read the controller serial number
                    case SER_NUM_READ:
                        length = 1;
                        send_packet(2, SER_NUM_READ, length, serial_num);
                        break;
                        
                    // send back a connected message
                    case CONNECT:
                        length = 4;
                        packet_data[0] = 1;
                        packet_data[1] = serial_num[0];
                        packet_data[2] = firmware[0];
                        packet_data[3] = firmware[1];
                        send_packet(2, CONNECT, length, packet_data);
                        break;               
           }    // end of switch
           
        }   // end of if(data_ready == 1)
    
    }   // end of while(1)
    
    return (EXIT_SUCCESS);
}   // end of main



// ----------------------------------------------------------------------------
void delay_ms(int count)
{
    int idx;
    
    for(idx=0; idx<count; ++idx)
    {
        TMR1 = 0;
        while(TMR1 < 10000);        		// wait 1ms before starting
    }
}