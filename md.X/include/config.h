/* 
 * File:   config.h
 * Author: David Emerson
 *
 * Created on March 26, 2013, 8:53 AM
 */

#ifndef CONFIG_H
#define	CONFIG_H

/***********************Declare Global Variables*******************************/
extern volatile unsigned short tick_time;

/****************************Function Definitions******************************/
void init_ADC(void);
void init_Timers(void);
void init_Clock(void);
void init_RTCC(void);
void init_UART(void);
//void init_CN(void);
void init_SPI3(void);
void init_ETH(void);
void init_Comparator(void);
void init_PreCache(void);

#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* CONFIG_H */


