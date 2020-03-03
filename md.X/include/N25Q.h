/*******************************************************************************
Micron Memory Module - N25Q - Serial NOR Flash Memory
Hearder File.

Writen by: David Emerson
	   25 March 2013

Written For: PIC32MX695F512

*******************************************************************************/

#ifndef N25Q_H
#define	N25Q_H

/******************************Command Definitions*****************************/
#define RESET_ENABLE		0x66
#define RESET_MEMORY		0x99
#define READ_ID			0x9E
#define READ_DISC_PARAM		0x5A
#define READ			0x03
#define FAST_READ		0x0B
#define FAST_READ_DTR		0x0D
#define READ_4B                 0x13
#define FAST_READ_4B            0x0C
#define WRITE_ENABLE		0x06
#define WRITE_DISABLE		0x04
#define READ_STATUS_REG		0x05
#define WRITE_STATUS_REG	0x01
#define READ_LOCK_REG		0xE8 
#define WRITE_LOCK_REG		0xE5
#define READ_FLAG_STATUS_REG	0x70
#define CLEAR_FLAG_STATUS_REG	0x50
#define READ_NV_CONFIG_REG	0xB5
#define WRITE_NV_CONFIG_REG	0xB1
#define READ_VOL_CONFIG_REG	0x85
#define WRITE_VOL_CONFIG_REG	0x81
#define READ_EN_VOL_CONFIG_REG	0x65
#define WRITE_EN_VOL_CONFIG_REG	0x61
#define READ_EX_ADD_REG		0xC8
#define WRITE_EX_ADD_REG	0xC5
#define PAGE_PROG		0x02
#define SUBSECTOR_ERASE		0x20
#define SECTOR_ERASE		0xD8
#define DIE_ERASE               0xC4
#define READ_OTP_ARRAY		0x4B
#define PROG_OTP_ARRAY		0x42
#define ENTER_4B_MODE		0xB7
#define EXIT_4B_MODE		0xE9

/*****************************Memory Constants*********************************/
#define DIE1                    0x00000000
#define DIE2                    0x02000000
#define DIE3                    0x04000000
#define DIE4                    0x06000000

#define SUBSECTOR_SIZE          0x1000
#define SECTOR_SIZE             0x10000
#define DIE_SIZE                0x02000000

/***********************Declare Global Variables*******************************/
extern volatile short AD1[MAX_SAMP];
extern volatile short AD2[MAX_SAMP];
extern volatile short AD3[MAX_SAMP];
extern volatile short AD4[MAX_SAMP];
extern volatile char CAL1[CAL_SIZE];
extern volatile char CAL2[CAL_SIZE];
extern volatile char CAL3[CAL_SIZE];
extern volatile char CAL4[CAL_SIZE];
extern volatile unsigned short tick_sync, tick_time, delay;
extern volatile unsigned char time[3];
extern volatile unsigned char date[3];

// data structure definitions
//struct mem_data
//{
//    int length;                     // number of bytes in data_stream
//    unsigned char *data_stream;     // pointer to the data
//};


/***************************Function Definitions*******************************/
unsigned char write_SPI(unsigned char value);
void Write_Enable(void);
void Enter_4B_Mode(void);
unsigned char read_SR(void);
unsigned short read_NV_CR(void);
void read_Register(unsigned char cc_code, int numofRXbytes, unsigned char RX_results[]);
unsigned char write_Register(const unsigned char cc_code, int numofTXbytes, unsigned char data[]);
unsigned char read_Mem(unsigned char cc_code, unsigned int mem_Address);
unsigned char write_ADC_Mem(unsigned int mem_Address, short ADC1, short ADC2, short ADC3, short ADC4);
unsigned char init_write_ADC_Trig(unsigned int mem_Address, unsigned short write_tick, unsigned short trigger_tick);
unsigned char write_ADC_Trig(unsigned int mem_Address, short ADC1, short ADC2, short ADC3, short ADC4);

void write_Mem(unsigned char cc_code, unsigned int mem_Address, unsigned char data[], int length);
unsigned char subsector_Erase(unsigned int mem_Address);
unsigned char sector_Erase(unsigned int mem_Address);
unsigned char die_Erase(unsigned int mem_Address);
unsigned char XIP_Reset(void);
unsigned char Protocol_Reset(void);


#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif

#endif  // end of define


