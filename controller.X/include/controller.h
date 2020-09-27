/* 
 * File:   md.h
 * Author: David Emerson
 *
 * Created on February 01, 2020
 */

#ifndef MD_H
#define	MD_H

// ----------------------------------------------------------------------------
// Define Constants
// ----------------------------------------------------------------------------
#define PKT_SIZE        192                     /* maximum number of bytes in a packet */

#define RED_LED         LATDbits.LATD0          /* define RED LED */
#define GREEN_LED       LATCbits.LATC13         /* define GREEN LED */
#define BLUE_LED        LATCbits.LATC14         /* define BLUE LED */

#define DIR_485_PIN     LATDbits.LATD1          /* RS485 Direction pin */

#define TRIG1_PIN       LATBbits.LATB6           /* Trigger 1 control */
#define TRIG2_PIN       LATEbits.LATE5           /* Trigger 2 control */


/***********************Define Header Command Responses************************/

#define MOTOR_ENABLE    0x10                  /* Enable/Disable motors */

// configure the trigger parameters
#define CONFIG_T1       0x11                  /* Configure Trigger 1 parameters */
#define CONFIG_T2       0x12                  /* Configure Trigger 2 parameters */

// initiate triggers
#define TRIG_INIT       0x20                  /* initiate trigger sequence */
#define TRIG_CH1        0x21                  /* pulse channel 1 */
#define TRIG_CH2        0x22                  /* pulse channel 2 */

// focus motor control
#define MOTOR_CTRL_PING 0x30
#define MOTOR_CTRL_WR	0x31				  /* send write command to motor */
#define MOTOR_CTRL_RD1	0x32				  /* send read command to motor */
#define MOTOR_CTRL_RD2	0x33				  /* send read command to motor */
#define MOTOR_CTRL_RD4	0x34				  /* send read command to motor */

// engineering functions
#define FIRM_READ       0x51                  /* Read firmware version return command */
#define SER_NUM_READ    0x52                  /* Read serial number return command */
#define CONNECT         0x53                  /* Check for data connection to motor controller */

#define U1              1
#define U2              2

#define PING_PACKET_LENGTH  14
#define WRITE_PACKET_LENGTH 11
#define READ1_PACKET_LENGTH  12
#define READ2_PACKET_LENGTH  13
#define READ4_PACKET_LENGTH  15

// ----------------------------------------------------------------------------
// Function Definitions
// ----------------------------------------------------------------------------
void delay_ms(int count);

void send_motor_packet(unsigned char uart, unsigned short length, unsigned char* data);
void receive_motor_packet(unsigned char uart, unsigned short length, unsigned char* data);

void initiate_trigger(void);


#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif

#endif	// MD_H

