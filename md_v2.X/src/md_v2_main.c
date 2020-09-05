/* 
 * File:   newmain.c
 * Author: owner
 *
 * Created on August 17, 2020, 3:16 PM
 */

// ----------------------------------------------------------------------------
// Configuration Bits
//DEVICE CONFIGURATION WORD 0
#pragma config CP = OFF, BWP = OFF, PWP = OFF, ICESEL = ICS_PGx1, DEBUG = OFF

//DEVICE CONFIGURATION WORD 1
#pragma config FWDTEN = OFF, WDTPS = PS4096, FCKSM = CSDCMD, FPBDIV = DIV_1, OSCIOFNC = OFF, POSCMOD = HS, IESO = OFF, FSOSCEN = OFF, FNOSC = PRIPLL

// DEVICE CONFIGURATION WORD 2
#pragma config FPLLODIV = DIV_1, UPLLEN = OFF, UPLLIDIV = DIV_1, FPLLMUL = MUL_20, FPLLIDIV = DIV_2

// DEVICE CONFIGURATION WORD 3
#pragma config FVBUSONIO = OFF, FUSBIDIO = OFF, FMIIEN = ON, FSRSSEL = PRIORITY_7

#define _SUPPRESS_PLIB_WARNING 1


//#include <stdio.h>
//#include <stdlib.h>
#include <xc.h>
#include <plib.h>

#include "../include/md.h"
#include "../include/pic32mx695f_config.h"
#include "../include/uart_ctrl.h"
#include "../include/dynamixel_protocol_v2.h"


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
// this is the interrupt to handle the comms between the pic and the pc
void __ISR(32, IPL4AUTO) UART2_Rx(void)
{
    char tmp_data = 0;

    mU2RXIntEnable(0);          // disable UART Rx interrupt
    
    tmp_data = get_char(U2);

    if(tmp_data == '$')
    {
        data_ready = 1;
        rx_data[0] = get_char(U2);
        rx_data[1] = get_char(U2);
    }

    mU2RXClearIntFlag();
}   // end of UART2 interrupt

// this is the interrupt that handles the comms between the pic and the motors
void __ISR(49, IPL5AUTO) UART1_RX(void)
{
    int idx = 0;
    unsigned short length;
    
    mU1RXIntEnable(0);          // disable UART Rx interrupt
    
    // get the header and reserved byte
    rx_data[0] = get_char(U1);
    rx_data[1] = get_char(U1);
    rx_data[2] = get_char(U1);
    rx_data[3] = get_char(U1);
    
    // get the id
    rx_data[4] = get_char(U1);
    
    // get the length
    rx_data[5] = get_char(U1);
    rx_data[6] = get_char(U1);
    length = (rx_data[6]<<8) | rx_data[5];
    
    // get the data
    for(idx=0; idx<length-3; ++idx)
    {
        rx_data[idx+7] = get_char(U1);
    }
        
    mU1RXClearIntFlag();
    mU1RXIntEnable(1);          // enable UART Rx interrupt
    
}// end of UART2 interrupt



// ----------------------------------------------------------------------------
int main(int argc, char** argv) 
{
    int idx;
    int length = 0;

    unsigned char temp;
    unsigned char packet_data[PKT_SIZE] = {0};
    
    data_packet motor_packet = initialize_packet();
    
    unsigned char p2[PKT_SIZE] = {0x74,0x00,0x00,0x02,0x00,0x00};
    
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
    TRISB = 0x00FFBF;               // RB6 => outputs
    TRISC = 0x009FFF;               // RC13 & RC14 => outputs
    TRISD = 0x00FFF4;               // RD0, RD1 & RD3 => outputs
    TRISE = 0x00FFDF;               // RE5 => outputs
    TRISF = 0x00FFDF;               // RF5 => output
    TRISG = 0x00FFFF;               // All Inputs
    
    // make sure that the motor is disabled on startup
    //MOT_EN_PIN = 1;

    // configure interrupt sources
    mU2SetIntPriority(4);       // configure UART2 Interrupt priority 4
    mU1SetIntPriority(5);       // configure UART1 Interrupt priority 5
    
    //mT2SetIntPriority(6);       // Configure Timer2 Interrupt priority to 6
    //mT3SetIntPriority(7);       // Configure Timer3 Interrupt priority to 7
	
    mU2RXClearIntFlag();
    mU1RXClearIntFlag();
    //mT3ClearIntFlag();
    //mT2ClearIntFlag();
    
    INTEnableSystemMultiVectoredInt();

    // turn off  LEDs
    RED_LED = 0;
    GREEN_LED = 0;
    BLUE_LED = 0;

    for(idx=10; idx>0; --idx)              	// wait 250ms to begin
    {
        
        RED_LED = 1;
        delay_ms(20*idx);
        RED_LED = 0;
        
        GREEN_LED = 1;
        delay_ms(20*idx);
        GREEN_LED = 0;
        
        BLUE_LED = 1;
        delay_ms(20*idx);        
        BLUE_LED = 0;     
        
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
           

           // read in the remaining data bytes
           for(idx=0; idx<rx_data[1]; ++idx)
           {
               rx_data[2+idx] = get_char(U2);
           }

           // This is the main command number
           switch(rx_data[0])
           {
               
               
               
               
                case MOTOR_CTRL:
                   build_packet(10, 6, DYN_WRITE, p2 , motor_packet);
                   send_motor_packet(U1, motor_packet);
                   break;
// ----------------------------------------------------------------------------
/*************************Engineering Operations******************************/
// ----------------------------------------------------------------------------
                        
                // read the controller firmware
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