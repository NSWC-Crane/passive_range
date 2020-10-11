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

#define BLUE_LED        LATDbits.LATD0          /* define RED LED */
#define GREEN_LED       LATCbits.LATC13         /* define GREEN LED */
#define RED_LED         LATCbits.LATC14         /* define BLUE LED */

#define DIR_485_PIN     LATDbits.LATD1          /* RS485 Direction pin */

#define TRIG1_PIN       LATBbits.LATB6           /* Trigger 1 control */
#define TRIG2_PIN       LATEbits.LATE5           /* Trigger 2 control */


/***********************Define Header Command Responses************************/

#define MOTOR_ENABLE    0x10                  /* Enable/Disable motors */

// configure the trigger parameters
#define CONFIG_T1       0x11                  /* Configure Trigger 1 parameters */
#define CONFIG_T2       0x12                  /* Configure Trigger 2 parameters */
#define TRIG_CONFIG     0x15                  /* Get the trigger config for each channel */

// initiate triggers
#define TRIG_INIT       0x20                  /* initiate trigger sequence */
#define TRIG_CH1        0x21                  /* pulse channel 1 */
#define TRIG_CH2        0x22                  /* pulse channel 2 */

// focus motor control
#define MOTOR_CTRL_PING 0x30                  /* Ping the motors to get their info */
#define MOTOR_CTRL_WR	0x31				  /* send write command to motor */
#define MOTOR_CTRL_RD1	0x32				  /* send read 1 byte command to motor */
#define MOTOR_CTRL_RD2	0x33				  /* send read 2 byte command to motor */
#define MOTOR_CTRL_RD4	0x34				  /* send read 4 byte command to motor */

// engineering functions
#define FIRM_READ       0x51                  /* Read firmware version return command */
#define SER_NUM_READ    0x52                  /* Read serial number return command */
#define CONNECT         0x53                  /* Check for data connection to motor controller */

#define U1              1
#define U2              2

#define PING_PACKET_LENGTH  14
#define WRITE_PACKET_LENGTH 11
#define READ1_PACKET_LENGTH 12
#define READ2_PACKET_LENGTH 13
#define READ4_PACKET_LENGTH 15

// motor specific parameters
#define FOCUS_MOTOR_ID      10              /* The ID for the focus motor */
#define ZOOM_MOTOR_ID       20              /* The ID for the zoom motor */
#define BROADCAST_ID        254             /* Broadcast ID */

// ----------------------------------------------------------------------------
// Function Definitions
// ----------------------------------------------------------------------------
void delay_ms(int count);

void send_motor_packet(unsigned char uart, unsigned short length, unsigned char* data);
void receive_motor_packet(unsigned char uart, unsigned short length, unsigned char* data);

void initiate_trigger(void);

extern unsigned short focus_position_pid[2][3] = {{0, 8, 800},
                                                  {0, 20, 800}
};

extern unsigned short zoom_position_pid[2][3] = {{0, 2, 800},
                                                 {0, 0, 800}
};


#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif

#endif	// MD_H

