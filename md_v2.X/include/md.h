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
#define PKT_SIZE      192                      /* maximum number of bytes in a packet */

#define Blue_LED      LATFbits.LATF3          /* define Blue LED */
#define Green_LED     LATEbits.LATE4          /* define green LED */
#define ZM_STEP_PIN   PORTBbits.RB10          /* Motor control - zoom steps */
#define ZM_DIR_PIN    PORTBbits.RB8           /* Motor control - zoom direction */
#define FOC_STEP_PIN  PORTBbits.RB4           /* Motor control - focus steps */
#define FOC_DIR_PIN   PORTBbits.RB2           /* Motor control - focus direction */
#define CMD_SW_PIN    PORTBbits.RB15          /* Switch in Command Position */
#define RUN_SW_PIN    PORTBbits.RB14          /* Switch in Run Posistion */
#define MOT_EN_PIN    PORTGbits.RG6           /* Motor control - enable all */

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

