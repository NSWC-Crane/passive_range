/* 
 * File:   config.h
 * Author: David Emerson
 *
 * Created on March 26, 2013, 8:53 AM
 */

#ifndef PIC32MX695F_CONFIG_H_
#define	PIC32MX695F_CONFIG_H_

/***********************Declare Global Variables*******************************/

/****************************Function Definitions******************************/
void init_CLOCK(void);
void init_ADC(void);
void init_TMR1(void);
void init_TMR2(void);
void init_TMR3(void);
void init_RTCC(void);
void init_UART1(void);
void init_UART2(void);
//void init_SPI3(void);
void init_ETH(void);
void init_COMP1(void);
void init_PRECACHE(void);

#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif

#endif	// PIC32MX695F_CONFIG_H_ 


