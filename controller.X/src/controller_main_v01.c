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
#include <proc/p32mx695f512h.h>

#include "../include/controller.h"
#include "../include/pic32mx695f_config.h"
#include "../include/uart_functions.h"
#include "../include/dynamixel_protocol_v2.h"


// ----------------------------------------------------------------------------
// Global Definitions
// ----------------------------------------------------------------------------
unsigned char rx_data[PKT_SIZE] = {0};
unsigned char data_ready = 0;
const unsigned char firmware[2] = {1, 5};
const unsigned char serial_num[1] = {2};

// motor parameters
int current_focus_step = 0;
int current_zoom_step = 0;

const int max_focus_step = 40575;
const int max_zoom_step = 4628;
const int min_step = 0;

// trigger parameters
unsigned char t1_out = 0;
unsigned char t1_polarity = 0;

unsigned int t1_offset = 0;
unsigned int t1_int_offset = 0;

unsigned int t1_length = 25000;
unsigned int t1_int_length = 250000;

unsigned char t2_out = 0;
unsigned char t2_polarity = 0;

unsigned int t2_offset = 0;
unsigned int t2_int_offset = 0;

unsigned int t2_length = 25000;
unsigned int t2_int_length = 250000;
    
const unsigned int trigger_interval = 500000;
const unsigned int max_trigger_offset = 500000;
const unsigned int max_trigger_length = 500000;

unsigned short focus_position_pid[3][3] = { {0, 0, 0},      // empty
                                            {0, 8, 800},    // sn-1
                                            {0, 20, 800}    // sn-2
};

unsigned short zoom_position_pid[3][3] = {  {0, 0, 0},      // empty
                                            {0, 2, 800},    // sn-1
                                            {0, 0, 800}     // sn-2
};


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
        rx_data[0] = get_char(U2);          // get the command byte
        rx_data[1] = get_char(U2);          // get the packet length byte
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
    unsigned int tmr_count = 0;

    unsigned char temp;
    unsigned char packet_data[PKT_SIZE] = {0};
    unsigned char params_data[16] = {0};
     
    //unsigned char TORQUE_ENABLE[]  = {0xFF, 0xFF, 0xFD, 0x00, 0x0A, 0x06, 0x00, 0x03, 0x40, 0x00, 0x01, 0x68, 0xED};
    //unsigned char STEP_FM[]        = {0xFF, 0xFF, 0xFD, 0x00, 0x0A, 0x09, 0x00, 0x03, 0x74, 0x00, 0xB8, 0x0B, 0x00, 0x00, 0xDD, 0xC9};
    //unsigned char TORQUE_DISABLE[] = {0xFF, 0xFF, 0xFD, 0x00, 0x0A, 0x06, 0x00, 0x03, 0x40, 0x00, 0x00, 0x6D, 0x6D};
    
    // SN-01
    // set the motor ID 10 position I gain to 8
    //unsigned char FOCUS_PID_I_ARRAY[] = {0xFF, 0xFF, 0xFD, 0x00, 0x0A, 0x07, 0x00, 0x03, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    // set the motor ID 20 position I gain to 2
    //unsigned char ZOOM_PID_I_ARRAY[] = {0xFF, 0xFF, 0xFD, 0x00, 0x14, 0x07, 0x00, 0x03, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00};
    
        
    if(RCON & 0x18)             // The WDT caused a wake from Sleep
    {
        asm volatile("eret");   // return from interrupt
    }
    
    DDPCONbits.JTAGEN=0;        // disable JTAG

    init_PRECACHE();            // setup wait states
    init_CLOCK();               // Setup and Initialize Main and Secondary Clock
    init_ADC();                 // Setup and initialize ADC Module
    init_TMR1();                // Setup and Initialize Timer Modules
    init_TMR23();               // Setup and Initialize Timer Modules
    
    init_UART1();               // Setup and Initialize UART
    init_UART2();               // Setup and Initialize UART

    // TRIS Configurations
    TRISB = 0x00FFBF;           // RB6 => outputs
    TRISC = 0x009FFF;           // RC13 & RC14 => outputs
    TRISD = 0x00FFF4;           // RD0, RD1 & RD3 => outputs
    TRISE = 0x00FFDF;           // RE5 => outputs
    TRISF = 0x00FFDF;           // RF5 => output
    TRISG = 0x00FFFF;           // All Inputs
    
    // make sure that the motor is disabled on startup
    //MOT_EN_PIN = 1;

    // configure interrupt sources
    mU2SetIntPriority(4);       // configure UART2 Interrupt priority 4
    mU1SetIntPriority(5);       // configure UART1 Interrupt priority 5
    
    //mT2SetIntPriority(6);       // Configure Timer2 Interrupt priority to 6
    //mT3SetIntPriority(7);       // Configure Timer3 Interrupt priority to 7
	
    mU2RXClearIntFlag();
    mU1RXClearIntFlag();
    
    INTEnableSystemMultiVectoredInt();

    // turn off  LEDs
    RED_LED = 0;
    GREEN_LED = 0;
    BLUE_LED = 0;
    DIR_485_PIN  = 0;

    for(idx=10; idx>0; --idx)              	// wait 250ms to begin
    {        
        RED_LED = 1;
        delay_ms(10*idx);
        RED_LED = 0;
        
        GREEN_LED = 1;
        delay_ms(10*idx);
        GREEN_LED = 0;
        
        BLUE_LED = 1;
        delay_ms(10*idx);        
        BLUE_LED = 0;            
    }
    RED_LED = 1;
    GREEN_LED = 1;
    BLUE_LED = 1;
    
    delay_ms(100);     

    RED_LED = 0;
    GREEN_LED = 0;
    BLUE_LED = 0;    
    
    // set the trigger pins to their initial configuration
    TRIG1_PIN = 0 ^ t1_polarity;
    TRIG2_PIN = 0 ^ t2_polarity;
    
    // clear out the UART2 interrupts and enable
    temp = U2RXREG;
    mU2RXClearIntFlag();
    mU2RXIntEnable(1);          // enable UART Rx interrupt

    // ----------------------------------------------------------------------------
    // config the motor PID setting based on the values and serial number   
    length = 14;
    split_uint16(ADD_POSITION_I_GAIN, &params_data[0], &params_data[1]);
    
    // build if position I gain packet for the focus motor
    split_uint16(focus_position_pid[serial_num[0]][1], &params_data[2], &params_data[3]);    
    build_packet(FOCUS_MOTOR_ID, 4, DYN_WRITE, params_data, packet_data);
    //send_packet(U2, MOTOR_CTRL_WR, length, packet_data);
    send_motor_packet(U1, length, packet_data);
    //receive_motor_packet(U1, WRITE_PACKET_LENGTH, packet_data);
    flush_uart(U1);
    
    // build if position I gain packet for the zoom motor 
    split_uint16(zoom_position_pid[serial_num[0]][1], &params_data[2], &params_data[3]);    
    build_packet(ZOOM_MOTOR_ID, 4, DYN_WRITE, params_data, packet_data);
    //send_packet(U2, MOTOR_CTRL_WR, length, packet_data);
    send_motor_packet(U1, length, packet_data);
    //receive_motor_packet(U1, WRITE_PACKET_LENGTH, packet_data);
    flush_uart(U1);

    RED_LED = 1;
             
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
                
// -----------------------------------------------------------------------------
//                              Trigger Operations
// -----------------------------------------------------------------------------
               case CONFIG_T1:
                    length = 1;

                    t1_polarity = rx_data[2] & 0x01;
                    t1_offset = (rx_data[3]<<24 | rx_data[4]<<16 | rx_data[5]<<8 | rx_data[6]);
                    t1_length = (rx_data[7]<<24 | rx_data[8]<<16 | rx_data[9]<<8 | rx_data[10]);
                    
                    t1_int_offset = min(max(t1_offset * 10, 0), max_trigger_offset);
                    t1_int_length = min(max(t1_length * 10 + t1_int_offset, 0), max_trigger_length);
                            
                    TRIG1_PIN = 0 ^ t1_polarity;
                    
                    packet_data[0] = t1_polarity;
                    send_packet(U2, CONFIG_T1, length, packet_data);
                    break;
                
                case CONFIG_T2:
                    length = 1;

                    t2_polarity = rx_data[2] & 0x01;
                    t2_offset = (rx_data[3]<<24 | rx_data[4]<<16 | rx_data[5]<<8 | rx_data[6]);
                    t2_length = (rx_data[7]<<24 | rx_data[8]<<16 | rx_data[9]<<8 | rx_data[10]);
                    
                    t2_int_offset = min(max(t2_offset * 10, 0), max_trigger_offset);
                    t2_int_length = min(max(t2_length * 10 + t2_int_offset, 0), max_trigger_length);
                    
                    TRIG2_PIN = 0 ^ t2_polarity;
                    
                    packet_data[0] = t2_polarity;
                    send_packet(U2, CONFIG_T2, length, packet_data);
                    break;
                    
                case TRIG_CONFIG:
                    length = 18;
                    
                    packet_data[0] = t1_polarity;
                    packet_data[1] = (t1_offset >> 24) & 0xFF;
                    packet_data[2] = (t1_offset >> 16) & 0xFF;
                    packet_data[3] = (t1_offset >> 8) & 0xFF;
                    packet_data[4] = (t1_offset) & 0xFF;
                    packet_data[5] = (t1_length >> 24) & 0xFF;
                    packet_data[6] = (t1_length >> 16) & 0xFF;
                    packet_data[7] = (t1_length >> 8) & 0xFF;
                    packet_data[8] = (t1_length) & 0xFF;
                            
                    packet_data[9] = t2_polarity;
                    packet_data[10] = (t2_offset >> 24) & 0xFF;
                    packet_data[11] = (t2_offset >> 16) & 0xFF;
                    packet_data[12] = (t2_offset >> 8) & 0xFF;
                    packet_data[13] = (t2_offset) & 0xFF;
                    packet_data[14] = (t2_length >> 24) & 0xFF;
                    packet_data[15] = (t2_length >> 16) & 0xFF;
                    packet_data[16] = (t2_length >> 8) & 0xFF;
                    packet_data[17] = (t2_length) & 0xFF;
                            
                    send_packet(U2, TRIG_CONFIG, length, packet_data);
                    break;
                    
                case TRIG_INIT:
                    length = 1;
                    
                    BLUE_LED = 1;
                    GREEN_LED = 1;
                    initiate_trigger();
                    BLUE_LED = 0;
                    GREEN_LED = 0;
                    
                    packet_data[0] = 1;
                    send_packet(U2, TRIG_INIT, length, packet_data);
                    break;
                    
                case TRIG_CH1:
                    length = 1;
                    BLUE_LED = 1;
                    
                    TRIG1_PIN = 0 ^ t1_polarity;
                    
                    TMR2 = 0;
                    while(TMR2 < trigger_interval)
                    {                        
                        t1_out = ((TMR2 >= t1_int_offset) && (TMR2 <= t1_int_length)) ^ t1_polarity;
                        TRIG1_PIN =  t1_out;   
                    }                   
                    TRIG1_PIN = 0 ^ t1_polarity;
                    BLUE_LED = 0;
                    
                    packet_data[0] = 1;
                    send_packet(U2, TRIG_CH1, length, packet_data);                    
                    break;
                    
                case TRIG_CH2:
                    length = 1;
                    GREEN_LED = 1;
                                        
                    TRIG2_PIN = 0 ^ t2_polarity;
                    TMR2 = 0;
                    while(TMR2 < trigger_interval)
                    {
                        t2_out = ((TMR2 >= t2_int_offset) && (TMR2 <= t2_int_length)) ^ t2_polarity;
                        TRIG2_PIN =  t2_out;
                    }                   
                    TRIG2_PIN = 0 ^ t2_polarity;                    
                    GREEN_LED = 0;
                    
                    packet_data[0] = 1;
                    send_packet(U2, TRIG_CH2, length, packet_data); 
                    break;
                
// -----------------------------------------------------------------------------
//                             Motor Operations
// -----------------------------------------------------------------------------
               case MOTOR_CTRL_PING:     
                    length = PING_PACKET_LENGTH;
                    mU1RXClearIntFlag();
                    flush_uart(U1);
                    
                    //DIR_485_PIN = 1;
                    send_motor_packet(U1, rx_data[1], &rx_data[2]);
                    //while(U1STAbits.TRMT == 0);
                    //DIR_485_PIN = 0;
                   
                    receive_motor_packet(U1, length, packet_data);
                    
                    send_packet(U2, MOTOR_CTRL_PING, length, packet_data);

                    break;
                   
               case MOTOR_CTRL_WR:
                    length = WRITE_PACKET_LENGTH;
                    mU1RXClearIntFlag();
                    flush_uart(U1);
                    
                    //DIR_485_PIN = 1;
                    send_motor_packet(U1, rx_data[1], &rx_data[2]);
                    //while(U1STAbits.TRMT == 0);
                    //DIR_485_PIN = 0;

                    receive_motor_packet(U1, length, packet_data);

                    send_packet(U2, MOTOR_CTRL_WR, length, packet_data);  
                   
                    break;

               case MOTOR_CTRL_RD1:
                    length = READ1_PACKET_LENGTH;
                    mU1RXClearIntFlag();
                    flush_uart(U1);
                    
                    //DIR_485_PIN = 1;
                    send_motor_packet(U1, rx_data[1], &rx_data[2]);
                    //while(U1STAbits.TRMT == 0);
                    //DIR_485_PIN = 0;

                    receive_motor_packet(U1, length, packet_data);
                    
                    send_packet(U2, MOTOR_CTRL_RD1, length, packet_data);                    
                                        
                    break;

               case MOTOR_CTRL_RD2:
                    length = READ2_PACKET_LENGTH;
                    mU1RXClearIntFlag();
                    flush_uart(U1);
                    
                    //DIR_485_PIN = 1;
                    send_motor_packet(U1, rx_data[1], &rx_data[2]);
                    //while(U1STAbits.TRMT == 0);
                    //DIR_485_PIN = 0;

                    receive_motor_packet(U1, length, packet_data);
                    
                    send_packet(U2, MOTOR_CTRL_RD2, length, packet_data);                    
                                        
                    break;
                    
               case MOTOR_CTRL_RD4:
                    length = READ4_PACKET_LENGTH;
                    mU1RXClearIntFlag();
                    flush_uart(U1);
                    
                    //DIR_485_PIN = 1;
                    send_motor_packet(U1, rx_data[1], &rx_data[2]);
                    //while(U1STAbits.TRMT == 0);
                    //DIR_485_PIN = 0;

                    receive_motor_packet(U1, length, packet_data);
                    
                    send_packet(U2, MOTOR_CTRL_RD4, length, packet_data);                    
                                        
                    break;
                    
// ----------------------------------------------------------------------------
//                        Engineering Operations
// ----------------------------------------------------------------------------
                        
                // read the controller firmware
                case FIRM_READ:
                    length = 2;
                    send_packet(U2, FIRM_READ, length, (unsigned char *)firmware);
                    break;  

                // read the controller serial number
                case SER_NUM_READ:
                    length = 1;
                    send_packet(U2, SER_NUM_READ, length, (unsigned char *)serial_num);
                    break;

                // send back a connected message
                case CONNECT:
                    length = 4;
                    packet_data[0] = 1;
                    packet_data[1] = serial_num[0];
                    packet_data[2] = firmware[0];
                    packet_data[3] = firmware[1];
                    send_packet(U2, CONNECT, length, packet_data);
                    break;               
            }    // end of switch

            data_ready = 0;
            mU2RXIntEnable(1);          // enable UART Rx Interrupt

        }   // end of if(data_ready == 1)
    
    }   // end of while(1)
    
    return (EXIT_SUCCESS);
    
}   // end of main



// ----------------------------------------------------------------------------
/* Function: void delay(int count)
 *
 * Arguments:
 * 1. count: number of milliseconds to delay
 *
 * Return Value: None
 *
 * Description: delay function in milliseconds
 */
void delay_ms(int count)
{
    int idx;
    
    for(idx=0; idx<count; ++idx)
    {
        TMR1 = 0;
        while(TMR1 < 10000);        		// wait 1ms
    }
}

//-----------------------------------------------------------------------------
/* Function: void send_motor_packet(unsigned char uart, unsigned char length, unsigned char* data)
 *
 * Arguments:
 * 1. uart: the uart number to send data to
 * 2. length: length of the data to be sent
 * 3. *data: pointer to the data to be sent
 *
 * Return Value: None
 *
 * Description: Send message with command header, packet byte size, data
 */
void send_motor_packet(unsigned char uart, unsigned short length, unsigned char* data)
{
    unsigned short idx;
    
    DIR_485_PIN = 1;
    
    for(idx=0; idx<length; ++idx)                 // send data
    {
        send_char(data[idx], uart);
    }
    
    while(U1STAbits.TRMT == 0);
    DIR_485_PIN = 0;
    
}   // end of send_motor_packet


void receive_motor_packet(unsigned char uart, unsigned short length, unsigned char* data)
{
    unsigned short idx;
    
    TMR2 = 0;

    while(IFS0bits.U1RXIF == 0)
    {
        if(TMR2 > 2000000)                        // wait ~200ms before kicking out of waiting for the interrupt
        {
            for(idx=0; idx<length; ++idx)       // fill in 1's for the data
            {
                data[idx] = 1;
            }                
            return;
        }
    }

    for(idx=0; idx<length; ++idx)               // receive data
    {
        data[idx] = get_char(uart);
    }
    
    IFS0bits.U1RXIF = 0;

}   // end of receive_motor_packet





void initiate_trigger(void)
{
    TRIG1_PIN = 0 ^ t1_polarity;
    TRIG2_PIN = 0 ^ t2_polarity;
    
    // reset the counter
    TMR2 = 0;  
    
    while(TMR2 < trigger_interval)
    {
        t1_out = ((TMR2 >= t1_int_offset) && (TMR2 <= t1_int_length))^ t1_polarity;
        t2_out = ((TMR2 >= t2_int_offset) && (TMR2 <= t2_int_length))^ t2_polarity;
        
        TRIG1_PIN = t1_out;
        TRIG2_PIN = t2_out;   
    }
    
    TRIG1_PIN = 0 ^ t1_polarity;
    TRIG2_PIN = 0 ^ t2_polarity;
    
}   // end of initiate_trigger

