/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef TRIGGER_H
#define	TRIGGER_H

#include <xc.h> // include processor files - each processor file is guarded.  

/*****************************Define Constants*********************************/
#define PKT_SIZE 8             /* maximum packet size in bytes */

#define TRIG1           LATAbits.LATA2        /* define trigger 1 pin */
#define TRIG2           LATAbits.LATA4        /* define trigger 2 pin */
#define TRIG3           LATAbits.LATA5        /* define trigger 3 pin */

/***********************Define Header Command Responses************************/
#define CONNECT         0x01                  /* Check for data connection to motor controller */
#define FIRM_READ       0x02                  /* Read firmware version return command */
#define SER_NUM_READ    0x03                  /* Read serial number return command */

// configure the trigger parameters
#define CONFIG_T1       0x11                  /* Configure Trigger 1 parameters */
#define CONFIG_T2       0x12                  /* Configure Trigger 2 parameters */
#define CONFIG_T3       0x13                  /* Configure Trigger 3 parameters */

// initiate triggers
#define TRIG_INIT       0x21                  /* initiate trigger */



//------------------------------------------------------------------------------
/* Function: void send_char(unsigned char c)
 *
 * Arguments: 
 *  1. c: single byte of data
 *
 * Return Value: None
 *
 * Description: Function to send data out of UART2 to the user */
void send_char(unsigned char c);

//------------------------------------------------------------------------------
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
void send_packet(unsigned char command, unsigned short length, unsigned char data[]);

void initiate_trigger(void);


#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

    // TODO If C++ is being used, regular C code needs function names to have C 
    // linkage so the functions can be used by the c code. 

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* TRIGGER_H */

