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

// focus motor control
#define ZERO_FOCUS		 0x20				  /* zero the focus motor */
#define FOCUS_CTRL       0x21                 /* Focus motor control */
#define ABS_FOCUS_CTRL   0x22                 /* Absolute focus motor control */
#define GET_FOC_MOT_STEP 0x23                 /* get the focus motor step count */  
#define SET_FOC_MOT_SPD  0x24				  /* set focus motor speed */
#define GET_FOC_MOT_SPD  0x25				  /* get focus motor speed */

// zoom motor control
#define ZERO_ZOOM		 0x30				  /* zero the zoom motor */
#define ZOOM_CTRL        0x31                 /* Zoom motor control */
#define ABS_ZOOM_CTRL    0x32                 /* Absolute zoom motor control */
#define GET_ZM_MOT_STEP  0x33                 /* get the zoom motor step count */
#define SET_ZM_MOT_SPD   0x34				  /* set zoom motor speed */
#define GET_ZM_MOT_SPD   0x35				  /* get zoom motor speed */


#define FIRM_READ       0x51                  /* Read firmware version return command */
#define SER_NUM_READ    0x52                  /* Read serial number return command */
#define CONNECT         0x53                  /* Check for data connection to motor controller */

#define TRIG_CTRL       0x61                  /* Camera trigger control */

#define U1              1
#define U2              2


// ----------------------------------------------------------------------------
// Function Definitions
// ----------------------------------------------------------------------------
void delay_ms(int count);

#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif

#endif	// MD_H

