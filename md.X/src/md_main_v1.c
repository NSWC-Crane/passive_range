/* 
 * File:   md_main_v1.c
 * Author: David Emerson
 *
 * Created on February 01, 2020
 */

// Configuration Bits
#pragma config CP = OFF, BWP = OFF, PWP = OFF, ICESEL = ICS_PGx2, DEBUG = OFF
#pragma config FWDTEN = OFF, WDTPS = PS4096, FCKSM = CSDCMD, FPBDIV = DIV_1, OSCIOFNC = OFF, POSCMOD = HS, IESO = OFF, FSOSCEN = ON, FNOSC = PRIPLL
#pragma config FPLLODIV = DIV_1, UPLLEN = OFF, UPLLIDIV = DIV_1, FPLLMUL = MUL_20, FPLLIDIV = DIV_2
#pragma config FVBUSONIO = OFF, FUSBIDIO = OFF, FMIIEN = ON, FSRSSEL = PRIORITY_7

#include <xc.h>
#include <plib.h>
#include <sys/attribs.h>
#include <string.h>
#include "../include/md.h"
#include "../include/config.h"
#include <cp0defs.h>

/****************************Local Definitions*********************************/

/*************************Declare Global Variables*****************************/
volatile unsigned short tick_time;
volatile unsigned char time[3] = {0,0,0};
volatile unsigned char date[3] = {0,0,0};

//unsigned int global_count=0;
unsigned char direction = 0;

unsigned char rx_data[PKT_SIZE] = {0, 0, 0};
unsigned char data_ready = 0;
const unsigned char firmware[2] = {0,50};
const unsigned char serial_num[1] = {2};

int focus_step_count = 0;
int zoom_step_count = 0;

const int max_focus_steps = 25000;
const int max_zoom_steps = 10000;
const int min_steps = 0;

/***************************Interrupt Definitions******************************/

/*****************************Timer 2 Interrupt********************************/
void __ISR(_TIMER_2_VECTOR, IPL6) Timer2Handler(void)
{
    //count1 = _CP0_GET_COUNT();
    TMR2 = 0;
//    while (IFS1bits.AD1IF == 0);        // wait for conversion to be done
//    AD1[count] = max_ADC1;
//    AD2[count] = max_ADC2;
//    AD3[count] = max_ADC3;
//    AD4[count] = max_ADC4;
//    IFS1bits.AD1IF = 0;                 // reset interrupt flag

//    max_ADC1 = 0;                       // reset max for ADC channels
//    max_ADC2 = 0;
//    max_ADC3 = 0;
//    max_ADC4 = 0;
//
//    delay_count++;
//    count++;
    switch(direction)
    {
        case 0:
            break;
        case 1:
            break;
    }

    mT2ClearIntFlag();
    //count2 = _CP0_GET_COUNT();
}   // end of Timer 2 interrupt

/*****************************Timer 3 Interrupt********************************/
void __ISR(12, IPL7AUTO) Timer3Handler(void)
{
    
    TMR3 = 0;
//    while (IFS1bits.AD1IF == 0);        // wait for conversion to be done
    
//    if(ADC1BUF0>max_ADC1)               // check max values
//        max_ADC1 = ADC1BUF0;
//    if(ADC1BUF1>max_ADC2)
//        max_ADC2 = ADC1BUF1;
//    if(ADC1BUF2>max_ADC3)
//        max_ADC3 = ADC1BUF2;
//    if(ADC1BUF3>max_ADC4)
//        max_ADC4 = ADC1BUF3;

//    IFS1bits.AD1IF = 0;                 // reset interrupt flag

    mT3ClearIntFlag();
    
}   // end of Timer 3 interrupt


void __ISR(32,IPL4) UART_Rx(void)
{
    char tmp_data = 0;

    tmp_data = get_U2_char();
    if(tmp_data == '$')
    {
        data_ready = 1;
        rx_data[0] = get_U2_char();
        rx_data[1] = get_U2_char();
        //rx_data[2] = get_U2_char();
    }

    mU2RXClearIntFlag();
}   // end of UART2 interrupt

/*****************************Main Program*************************************/
void main(void)
{
    // Variable Declarations
    int idx = 0;
    int length = 0;
    //int int_temp = 0;
	int desired_steps = 0;
	int steps = 0;
	
    unsigned int uint_temp = 0;
    unsigned char dir;
	
    unsigned char running=0, command=0;
    unsigned char temp;//, read, erase_size[2] = {0,0};
    unsigned char packet_data[PKT_SIZE] = {0,0,0,0,0,0,0,0,0,0};

    if(RCON & 0x18)             // The WDT caused a wake from Sleep
    {
        asm volatile("eret");   // return from interrupt
    }

    DDPCONbits.JTAGEN=0;        // disable JTAG

    init_PreCache();            // setup wait states
    init_Clock();               // Setup and Initialize Main and Secondary Clock
    init_RTCC();                // Setup and Initialize RTCC
    init_ETH();                 // Setup Ethernet module
    init_Timers();              // Setup and Initialize Timer Modules
    init_Comparator();          // Setup Comparator Module
    
    init_UART();                // Setup and Initialize UART
    init_SPI3();                // Setup and Initialize SPI3 Module
    init_ADC();                 // Setup and initialize ADC Module

    // TRIS Configurations
    TRISB = 0x00FAEB;             // RB2, RB4, RB8 & RB10 set to outputs
    TRISE = 0x00FFE3;             // RE2, RE3 & RE4 => outputs
    TRISF = 0x00FFF7;             // RF3 => output
    TRISG = 0x00FFBF;             // RG6 => output
    
    // make sure that the motor is disabled on startup
    MOT_EN_PIN = 1;

    // configure interrupt sources
    mU2SetIntPriority(4);       // configure UART2 Interrupt priority 4
    mT2SetIntPriority(6);       // Configure Timer2 Interrupt priority to 6
    mT3SetIntPriority(7);       // Configure Timer3 Interrupt priority to 7
	
    mU2RXClearIntFlag();
    mT3ClearIntFlag();
    mT2ClearIntFlag();
    
    INTEnableSystemMultiVectoredInt();

    // Configure LEDs
    Green_LED = 0;              // turn off green LED
    Blue_LED = 0;               // turn off blue LED

    Green_LED = 1;
    Green_LED = 0;
    
    Blue_LED = 1;
    Blue_LED = 0;

    for(idx=0; idx>5; ++idx)              	// wait 250ms to begin
    {
        TMR1 = 0;
        while(TMR1 < 62500);        		// wait 50ms before starting
    }
        
/**********************************MAIN LOOP**********************************/
    while(1)
    {
        // This state is when the switch is to the right and the logger enters data logging mode
        while (RUN_SW_PIN == 1 && CMD_SW_PIN == 0)
        {
            if(running == 0)
            {
                for(idx=0; idx<2; ++idx)              // wait 100ms to begin
                {
                    TMR1 = 0;
                    while(TMR1 < 62500);        // wait 50ms before starting
                }

                mU2RXIntEnable(0);          // disable U2 Interrupt
                mT3IntEnable(1);            // enable T3 Interrupt
                mT2IntEnable(1);            // enable T2 Interrupt
                
                Blue_LED = 0;               // turn off blue LED
                running = 1;
                command = 0;
            }        
                
        } // end of RUN_SW while

        // This state is when the switch is set to the left and the logger enters command mode.
        while(CMD_SW_PIN == 1 && RUN_SW_PIN == 0)
        {
            
            if(command == 0)
            {
                temp = U2RXREG;
                mU2RXClearIntFlag();
                mU2RXIntEnable(1);          // enable UART Rx interrupt

                mT3IntEnable(0);            // enable T3 Interrupt
                mT2IntEnable(0);            // enable T2 Interrupt
                
                Green_LED = 0;              // turn off green LED
                Blue_LED = 1;               // turn on blue LED
                running = 0; 
                command = 1;
            }

            if(data_ready == 1)
            {
                mU2RXIntEnable(0);          // disable UART Rx interrupt
                
                // read in the remaining data bytes
                for(idx=0; idx<rx_data[1]; ++idx)
                {
                    rx_data[2+idx] = get_U2_char();
                }
                
                // This is the main command number
                switch(rx_data[0])
                {
                    
/*************************Motor Control Operations****************************/

                    // Enable Motors
                    case MOTOR_ENABLE:
                        length = 1;
                        
                        MOT_EN_PIN = rx_data[2];
                        packet_data[0] = MOT_EN_PIN;
                        send_packet(MOTOR_ENABLE, length, packet_data);
                        break;
                        
                    // focus motor control
                    case FOCUS_CTRL:
                        length = 4;
                        Green_LED = 1;
						
                        // run the motor control
                        steps = (rx_data[2]&0xFF)<<24 | (rx_data[3]<<16) | (rx_data[4]<<8) | (rx_data[5]);
						FOC_DIR_PIN = (rx_data[2] >> 7) & 0x01;
						
						// ensure that the steps do not go beyond the limits on the rings being turned
						desired_steps = focus_step_count + steps;
						
						if(desired_steps < min_steps)
						{
							steps = min_steps - focus_step_count;
						}
						else if(desired_steps > max_focus_steps)
						{
							steps = max_focus_steps - focus_step_count;
						}

						focus_step_count += steps;
						step_focus_motor(abs(steps));
                        
						packet_data[0] = (focus_step_count >> 24) & 0x00FF;
						packet_data[1] = (focus_step_count >> 16) & 0x00FF;
						packet_data[2] = (focus_step_count >> 8) & 0x00FF;
						packet_data[3] = (focus_step_count) & 0x00FF;

                        send_packet(FOCUS_CTRL, length, packet_data);
                        Green_LED = 0;
                        break;
                        
                    // zoom motor control
                    case ZOOM_CTRL:           
                        length = 4;
                        Green_LED = 1;
						
                        // run the motor control
						steps = (rx_data[2]&0xFF)<<24 | (rx_data[3]<<16) | (rx_data[4]<<8) | (rx_data[5]);
						ZM_DIR_PIN = (rx_data[2] >> 7) & 0x01;
                                
						// ensure that the steps do not go beyond the limits on the rings being turned
						desired_steps = zoom_step_count + steps;

						if(desired_steps < min_steps)
						{
							steps = min_steps - zoom_step_count;
						}
						else if(desired_steps > max_zoom_steps)
						{
							steps = max_zoom_steps - zoom_step_count;
						}

						zoom_step_count += steps;						
                        step_zoom_motor(abs(steps));
                        
						packet_data[0] = (zoom_step_count >> 24) & 0x00FF;
						packet_data[1] = (zoom_step_count >> 16) & 0x00FF;
						packet_data[2] = (zoom_step_count >> 8) & 0x00FF;
						packet_data[3] = (zoom_step_count) & 0x00FF;

                        send_packet(ZOOM_CTRL, length, packet_data);
                        Green_LED = 0;
                        break;

                    // both motor controls
                    case ALL_CTRL:
                        length = 8;
                        Green_LED = 1;
						
                        // run the motor control
                        steps = (rx_data[2]&0xFF)<<24 | (rx_data[3]<<16) | (rx_data[4]<<8) | (rx_data[5]);
						FOC_DIR_PIN = (rx_data[2] >> 7) & 0x01;
						
						// ensure that the steps do not go beyond the limits on the rings being turned
						desired_steps = focus_step_count + steps;
						
						if(desired_steps < min_steps)
						{
							steps = min_steps - focus_step_count;
						}
						else if(desired_steps > max_focus_steps)
						{
							steps = max_focus_steps - focus_step_count;
						}

						focus_step_count += steps;
						step_focus_motor(abs(steps));
						
                        // run the zoom motor control
                        steps = (rx_data[6]&0xFF)<<24 | (rx_data[7]<<16) | (rx_data[8]<<8) | (rx_data[9]);
                        ZM_DIR_PIN = (rx_data[6] >> 7) & 0x01;
                                
						// ensure that the steps do not go beyond the limits on the rings being turned
						desired_steps = zoom_step_count + steps;

						if(desired_steps < min_steps)
						{
							steps = min_steps - zoom_step_count;
						}
						else if(desired_steps > max_zoom_steps)
						{
							steps = max_zoom_steps - zoom_step_count;
						}

						zoom_step_count += steps;						
                        step_zoom_motor(abs(steps));
                        
						packet_data[0] = (focus_step_count >> 24) & 0x00FF;
						packet_data[1] = (focus_step_count >> 16) & 0x00FF;
						packet_data[2] = (focus_step_count >> 8) & 0x00FF;
						packet_data[3] = (focus_step_count) & 0x00FF;
						
						packet_data[4] = (zoom_step_count >> 24) & 0x00FF;
						packet_data[5] = (zoom_step_count >> 16) & 0x00FF;
						packet_data[6] = (zoom_step_count >> 8) & 0x00FF;
						packet_data[7] = (zoom_step_count) & 0x00FF;
						
                        send_packet(ALL_CTRL, length, packet_data);
                        Green_LED = 0;
                        break;
						
					case ZERO_FOCUS:
						length = 4;
						Green_LED = 1;
						
						FOC_DIR_PIN = 1;
						step_focus_motor(focus_step_count);
						focus_step_count = 0;
						
						packet_data[0] = (focus_step_count >> 24) & 0x00FF;
						packet_data[1] = (focus_step_count >> 16) & 0x00FF;
						packet_data[2] = (focus_step_count >> 8) & 0x00FF;
						packet_data[3] = (focus_step_count) & 0x00FF;
						
                        send_packet(ZERO_FOCUS, length, packet_data);
                        Green_LED = 0;
                        break;
						
					case ZERO_ZOOM:
						length = 4;
						Green_LED = 1;
						
						FOC_DIR_PIN = 1;
						step_focus_motor(zoom_step_count);
						zoom_step_count = 0;
						
						packet_data[0] = (zoom_step_count >> 24) & 0x00FF;
						packet_data[1] = (zoom_step_count >> 16) & 0x00FF;
						packet_data[2] = (zoom_step_count >> 8) & 0x00FF;
						packet_data[3] = (zoom_step_count) & 0x00FF;
						
                        send_packet(ZERO_ZOOM, length, packet_data);
                        Green_LED = 0;
                        break;						
                        
/***********************Camera Trigger Operations*****************************/
                        
                    case TRIG_CTRL:
                        
                        break;
                        
/*************************Engineering Operations******************************/
                        
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
                        
                    // send back a connected message
                    case CONNECT:
                        length = 4;
                        packet_data[0] = 1;
                        packet_data[1] = serial_num[0];
                        packet_data[2] = firmware[0];
                        packet_data[3] = firmware[1];
                        send_packet(CONNECT, length, packet_data);
                        break;  

                } // end of switch(code[1])

                data_ready = 0;
                mU2RXIntEnable(1);          // enable UART Rx Interrupt
                
            } // end if
             
        }// end of CMD_SW while

        // This state is when the switch is in the center position and the PIC should enter power
        // save mode.  Watchdog timer wakes PIC from sleep
        while((CMD_SW_PIN == 0 && RUN_SW_PIN == 0) || (CMD_SW_PIN == 1 && RUN_SW_PIN == 1))
        {
            WDTCONSET = 0x01;               // service the WDT
            WDTCONbits.ON = 0x01;           // enable WDT
            
            if((running == 1) || (command == 1))
            {
                mU2RXIntEnable(0);          // disable U2 Interrupt
                mT3IntEnable(0);            // disable T3 Interrupt
                mT2IntEnable(0);            // disable T2 Interrupt
                
                Green_LED = 0;              // turn off green LED
                Blue_LED = 0;               // turn off blue LED
                command = 0;
                running = 0;
            }       

            asm volatile("wait");
            WDTCONSET = 0x01;               // service the WDT
            WDTCONbits.ON = 0x00;           // disable WDT
        }
        
    } // end of main while loop

    //return 0;
} // End of Main


//-----------------------------------------------------------------------------
/**********************************Functions**********************************/
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
    while(U2STAbits.UTXBF);				// wait for at least one buffer slot to be open
    U2TXREG = c;
}   // end of send_char


//-----------------------------------------------------------------------------
/* Function: unsigned char get_U2_char(void)
 *
 * Arguments: None
 *
 * Return Value: 
 * 1. U2RXREG: single byte of data received from UART2
 *
 * Description: Function to read the UART2 receive register and make the value
 * available to the user */
unsigned char get_U2_char(void)
{
    char temp;
    while(U2STAbits.URXDA == 0);        // wait for at least one byte in buffer

    temp = U2RXREG;

    if(U2STAbits.OERR == 1)             // check for overflow errors
    {
        U2STAbits.OERR = 0;             // clear overflow error
    }
    
    return temp;
}   // end of get_U2_char


//-----------------------------------------------------------------------------
/* Function: void BCD_Convert(short ADC_Value)
 *
 * Arguments:
 * 1. Value: Value to convert
 *
 * Return Value: None
 *
 * Description: Function to convert a number to a hex representation over serial port */
void BCD_Convert(short Value)
{
    short temp, i;
    char Convert_Array[4];

    temp = Value;
    Convert_Array[0] = temp / 1000;
    temp = temp % 1000;
    Convert_Array[1] = temp / 100;
    temp = temp % 100;
    Convert_Array[2] = temp / 10;
    temp = temp % 10;
    Convert_Array[3] = temp;

    for (i=0; i<4; i++)
    {
        send_char(Convert_Array[i] + 48);
    }
    
}   // end of BCD_Convert


//-----------------------------------------------------------------------------
/* Function: void bin2hex(unsigned char convert)
 *
 * Arguments:
 * 1. convert: binary 1-byte number to convert
 *
 * Return Value: None
 *
 * Description: convert a single byte to a 2 character number to send over serial
 * comms  */
void bin2hex(unsigned char convert)
{
    unsigned char i, temp;

    for(i=2; i>0; i--)
    {
        temp = (convert>>4*(i-1))&0x0F;

        if(temp <= 9)
            temp += 48;
        else if(temp == 10)
            temp = 'A';
        else if(temp == 11)
            temp = 'B';
        else if(temp == 12)
            temp = 'C';
        else if(temp == 13)
            temp = 'D';
        else if(temp == 14)
            temp = 'E';
        else if(temp == 15)
            temp = 'F';

        send_char(temp);
    }

} // end of bin2hex

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
 * Description: Send message with command header, packet byte size, data and Fletcher16 CRC */
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


//-----------------------------------------------------------------------------
/* Function: void send_packet(unsigned char code)
 *
 * Arguments:
 * 1. motor: pin connected to stepper motor driver step control
 * 2. N: number of steps to turn
 *
 * Return Value: None
 *
 * Description: Rotate motor N number of steps */
void step_focus_motor(unsigned int N)
{
    unsigned int idx = 0;
	    
    for(idx=0; idx<N; ++idx)
    {
        FOC_STEP_PIN = 1;
        //PORTBbits.RB4 = 1;
        TMR2 = 0;
        while(TMR2 < FOCUS_MOTOR_PW);
        
        FOC_STEP_PIN = 0;
        //PORTBbits.RB4 = 0;
        TMR2 = 0;
        while(TMR2 < FOCUS_MOTOR_PW);       
    }
   
}   // end of step_focus_motor

void step_zoom_motor(unsigned int N)
{
    unsigned int idx = 0;
    
    for(idx=0; idx<N; ++idx)
    {
        ZM_STEP_PIN = 1;
        //PORTBbits.RB10 = 1;
        TMR2 = 0;
        while(TMR2 < ZOOM_MOTOR_PW);
        
        ZM_STEP_PIN = 0;
        //PORTBbits.RB10 = 0;
        TMR2 = 0;
        while(TMR2 < ZOOM_MOTOR_PW);       
    }
   
}   // end of step_zoom_motor

