/* 
 * File:   DataLogger.h
 * Author: Owner
 *
 * Created on April 19, 2013, 9:02 PM
 */

#ifndef MD_H
#define	MD_H

/*****************************Define Constants*********************************/
//#define DELAY       304                     // 998.4 us delay
//#define MAX_SAMP    13822                   //11774 //14846  //12798  // number of samples to record
//#define CAL_SIZE    1024
//#define MAX_TRIG    1639                    // Max number of triggers
//#define MAX_DATA    3141632
#define PKT_SIZE      16                      // maximum number of bytes in a packet
//#define MOTOR_PW      3985                    // number of TMR2 ticks for 50us
//#define MOTOR_PW      7970                    // number of TMR2 ticks for 100us
#define FOCUS_MOTOR_PW     1200                    // number of TMR2 ticks for 100us
#define ZOOM_MOTOR_PW      3200                    // number of TMR2 ticks for 100us


#define Blue_LED      LATFbits.LATF3          // define Blue LED
#define Green_LED     LATEbits.LATE4          // define green LED
//#define MEM_CS      LATDbits.LATD5          // Chip select for memory chip
//#define MEM_HOLD    LATDbits.LATD4          // Memory hold for memory chip
//#define MEM_SDO     LATDbits.LATD3          // SDO Pin for SPI3
//#define MEM_SDI     LATDbits.LATD2          // SDI Pin for SPI3
//#define MEM_CLK     LATDbits.LATD1          // CLock Pin for SPI3
#define ZM_STEP_PIN   PORTBbits.RB10          // Motor control - zoom steps
#define ZM_DIR_PIN    PORTBbits.RB8           // Motor control - zoom direction
#define FOC_STEP_PIN  PORTBbits.RB4           // Motor control - focus steps
#define FOC_DIR_PIN   PORTBbits.RB2           // Motor control - focus direction
#define CMD_SW_PIN    PORTBbits.RB15          // Switch in Command Position
#define RUN_SW_PIN    PORTBbits.RB14          // Switch in Run Posistion
#define MOT_EN_PIN    PORTGbits.RG6           // Motor control - enable all
//#define CN2         PORTGbits.RG7           // Change Notification Channel 2

/***********************Define Header Command Responses************************/

#define MOTOR_ENABLE    0x30                // Enable/Disable motors
#define FOCUS_CTRL      0x31                // Focus motor control
#define ZOOM_CTRL       0x32                // Zoom motor control
#define ALL_CTRL        0x33                // Both motor control

#define ZERO_FOCUS		0x35				// zero the focus motor 
#define ZERO_ZOOM		0x36				// zero the zoom motor

#define TRIG_CTRL       0x41                // Camera trigger control

#define FIRM_READ       0x51                // Read firmware version return command
#define SER_NUM_READ    0x52                // Read serial number return command
#define CONNECT         0x53                // Check for data connection to motor controller



/****************************Function Definitions******************************/
void send_char(unsigned char c);
unsigned char get_U2_char(void);
//void BCD_Convert(short Value);
void bin2hex(unsigned char convert);
//unsigned short get_delay(void);
//void send_delay(void);
//void Status_Error(unsigned char code);
void send_packet(unsigned char command, unsigned short length, unsigned char data[]);
//unsigned int mem_Scan(unsigned int start_add, unsigned int stop_add, unsigned short jump);
//void read_Cal(char cal[][CAL_SIZE], unsigned char channel);
//void write_Cal(char cal[][CAL_SIZE], unsigned char channel);
//char getCal(void);
void step_focus_motor(unsigned int N);
void step_zoom_motor(unsigned int N);

#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif

#endif	// MD_H

