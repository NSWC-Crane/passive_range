/**********************************************************
Micron Memory Module - N25Q - Serial NOR Flash Memory
Source File.

Writen by: David Emerson
	   19 April 2013

Written For: PIC32MX695F512

***********************************************************/

#include <xc.h>
#include <plib.h>
//#include <sys/attribs.h>
#include "../include/md.h"
#include "../include/config.h"
#include "../include/N25Q.h"

/*
 * Function: write_SPI(unsigned char value)
 *
 * Arguments: value - value to write to the buffer
 *
 * Return Value: value read from the buffer
 *
 * Description: routine to write and read value from the SPI buffer
 */
unsigned char write_SPI(unsigned char value)
{
    int temp = SPI3BUF;

    while(SPI3STATbits.SPITBF == 1);      // wait for at least one byte in the buffer to open up
    //while(SPI3STATbits.SPITBE == 0);    // wait for Tx Buffer to be empty
    SPI3BUF = value;                    // enable writing to the memory chip
    //while(SPI3STATbits.SPIRBE == 0);      // wait for the RX buffer to contain data
    while(SPI3STATbits.SPIRBF == 0);      // wait for the RX buffer to contain data

    temp = SPI3BUF;
    return temp;                        // return the received character

} // end of write_SPI


/*
 * Function: WREN(void)
 *
 * Arguments: None
 *
 * Return Value: None
 *
 * Description: Send write enable command to memory chip
 */
void Write_Enable(void)
{
    MEM_CS = 0;                         // enable chip select pin for data transfer
    write_SPI(WRITE_ENABLE);            // enable writing to the memory chip
    MEM_CS = 1;                         // disable chip select pin
} // end of Write_Enable


/*
 * Function: Enter_4B_Mode(void)
 *
 * Arguments: N/A
 *
 * Return Value: N/A
 *
 * Description: Send command to enter 4-Byte addressing mode
 */
void Enter_4B_Mode(void)
{
    Write_Enable();

    MEM_CS = 0;                         // enable chip select pin for data transfer
    write_SPI(ENTER_4B_MODE);           // enter 4-byte address mode
    MEM_CS = 1;                         // disable chip select pin
} // end of Enter_4B_Mode


/*
 * Function: unsigned char read_SR(void)
 *
 * Arguments: pointer to status register variable
 *
 * Return Value: unsigned char status register value
 *
 * Description: Read the status register on mem chip and return the result
 */
unsigned char read_SR(void)
{
    unsigned char temp;
    int i;
//    struct mem_data SR_result;
//    
//    SR_result.length = 1;
//    SR_result.data_stream = status_reg;
    
    MEM_CS = 0;                         // enable chip select pin for data transfer
//    for(i=0; i<100; i++)
//    {
//        Nop();
//    }
    write_SPI(READ_STATUS_REG);         // send read status register command

    temp = write_SPI(0);

    MEM_CS = 1;                         // disable chip select pin
    return temp;

} // end of read_SR


/*
 * Function: read_NV_CR(unsigned char *NV_result)
 *
 * Arguments: pointer to non-volatile config register results
 *
 * Return Value: pointer to non-volatile config register results
 *
 * Description: Read the status register on mem chip and return the result
 */
unsigned short read_NV_CR(void)
{
    short temp, temp1, i;

    MEM_CS = 0;                         // enable chip select pin for data transfer

    write_SPI(READ_NV_CONFIG_REG);      // send read status register command

    temp = write_SPI(129);
    Nop();
    temp1 = write_SPI(129);
    temp = (temp<<8) + temp1;

    MEM_CS = 1;                         // disable chip select pin

    return temp;
    
} // end of read_NV_CR


/*
 * Function: void read_Register(unsigned char reg_command, int numofRXbytes, unsigned char *RX_results)
 *
 * Arguments:
 * 1. cc_code: command code to access register
 * 2. numofRXbytes: number of data bytes to receive
 *
 * Return Value: 
 * 1. *RX_results: pointer to the received results
 *
 * Description: Read the status register for the given command and 
 * return the results
 */
void read_Register(unsigned char cc_code, int numofRXbytes, unsigned char RX_results[])
{
    unsigned char i=0;

    MEM_CS = 0;                         // enable chip select pin for data transfer

    write_SPI(cc_code);                 // send read status register command

    for(i=0;i < numofRXbytes; i++)
    {
        RX_results[i] = write_SPI(0);
    }

    MEM_CS = 1;                         // disable chip select pin

} // end of read_Register


/* Function: unsigned char write_Register(unsigned char cc_code, int numofTXbytes, unsigned char data[])
 *
 * Arguments:
 * 1. cc_code: command code to access register
 * 2. numofTXbytes: number of data bytes to transmit
 * 3. data[]: array of data bytes to transmit
 *
 * Return Value:
 * 1. status: value of Status Flag Register
 * 2. *RX_results: pointer to the received results
 *
 * Description: Generic write status register
 */
unsigned char write_Register(const unsigned char cc_code, int numofTXbytes, unsigned char data[])
{
    unsigned char status, i=0;
//    struct mem_data write_REG;

    Write_Enable();                 // write enable

    MEM_CS = 0;                     // enable chip select pin for data transfer
    write_SPI(cc_code);             // send read status register command

    for(i=0; i<numofTXbytes; i++)   //while(i < numofTXbytes)
    {
        write_SPI(data[i]);
    }
    MEM_CS = 1;                     // disable chip select pin

    do
    {
        TMR1 = 0;
        while(TMR1 < 17);
        MEM_CS = 0;
        write_SPI(READ_FLAG_STATUS_REG);
        status = write_SPI(0);
        MEM_CS = 1;
    }while((status&0x80)==0);

    return status;

} // end of write_Register


/* Function: unsigned char read_Mem(unsigned char cc_code, unsigned char mem_Address[])
 *
 * Arguments:
 * 1. cc_code: command code to access register
 * 2. mem_Address[]: 4-Byte array representing the memory address to read
 *
 * Return Value:
 * 1. flash_Data: 1 byte of data on flash memory
 *
 * Description: Read the memory location at mem_address and return the value
 */
unsigned char read_Mem(unsigned char cc_code, unsigned int mem_Address)
{
    unsigned char flash_Data;

    write_SPI(cc_code);                     // send read memory location code

    // send memory address to read
    write_SPI((mem_Address>>24)&0x00FF);
    write_SPI((mem_Address>>16)&0x00FF);
    write_SPI((mem_Address>>8)&0x00FF);
    write_SPI(mem_Address&0x00FF);

    flash_Data = write_SPI(0);              // get signle byte of data

    return flash_Data;

} // end of read_Mem

/* Function: unsigned char write_ADC_Mem(unsigned int mem_Address, short ADC1, short ADC2, short ADC3, short ADC4)
 *
 * Arguments:
 * 1. mem_Address: 4-Byte integer representing the starting memory address to write to
 * 2. ADC1: 16-bit value of the ADC conversion to write to memory
 * 3. ADC2: 16-bit value of the ADC conversion to write to memory
 * 4. ADC3: 16-bit value of the ADC conversion to write to memory
 * 5. ADC4: 16-bit value of the ADC conversion to write to memory
 *
 * Return Value:
 * 1. status: value of Status Flag Register
 *
 * Description: Function to write the ADC value to the memory location give by
 * mem_Address
 */
unsigned char write_ADC_Mem(unsigned int mem_Address, short ADC1, short ADC2, short ADC3, short ADC4)
{
    unsigned char status;
//    struct mem_data read_MEM_result;

    Write_Enable();                           // enable chip writing

    MEM_CS = 0;                               // enable chip select pin for data transfer
    write_SPI(PAGE_PROG);                     // send read memory location code

    // send memory address to write
    write_SPI((mem_Address>>24)&0x00FF);
    write_SPI((mem_Address>>16)&0x00FF);
    write_SPI((mem_Address>>8)&0x00FF);
    write_SPI(mem_Address&0x00FF);

    // Write Date
    write_SPI(date[0]);             // day
    write_SPI(date[1]);             // month
    write_SPI(date[2]);             // year

    // Write Time
    write_SPI(time[0]);             // hour
    write_SPI(time[1]);             // minute
    write_SPI(time[2]);             // second

    // Write ADC values
    write_SPI((ADC1>>8)&0x00FF);              // write the upper 8 bits to memory
    write_SPI(ADC1&0x00FF);                   // write the lower 8 bits to memory
    write_SPI((ADC2>>8)&0x00FF);              // write the upper 8 bits to memory
    write_SPI(ADC2&0x00FF);                   // write the lower 8 bits to memory    
    write_SPI((ADC3>>8)&0x00FF);              // write the upper 8 bits to memory
    write_SPI(ADC3&0x00FF);                   // write the lower 8 bits to memory   
    write_SPI((ADC4>>8)&0x00FF);              // write the upper 8 bits to memory
    write_SPI(ADC4&0x00FF);                   // write the lower 8 bits to memory

    MEM_CS = 1;                               // disable chip select pin

    do
    {
        TMR1 = 0;
        while(TMR1 < 20);
        MEM_CS = 0;
        write_SPI(READ_FLAG_STATUS_REG);
        status = write_SPI(0);
        MEM_CS = 1;
    }while((status&0x80)==0);

    return status;
    
} // end of write_ADC_Mem


/* Function: unsigned char init_write_ADC_Trig(unsigned int mem_Address,
 * unsigned short tick, unsigned short trig_delay)
 *
 * Arguments:
 * 1. mem_Address: 4-Byte integer representing the starting memory address to
 * write to
 * 2. write_tick: value of the tick counter + delay
 * 3. trigger_tick: value of the tick counter when trigger occured
 *
 * Return Value:
 * 1. status: value of Status Flag Register
 *
 * Description: Function to write the intial information for a trigger event to
 * the memory location give by mem_Address
 */
unsigned char init_write_ADC_Trig(unsigned int mem_Address, unsigned short write_tick, unsigned short trigger_tick)
{
    unsigned char status;

    Write_Enable();                           // enable chip writing

    MEM_CS = 0;                               // enable chip select pin for data transfer
    write_SPI(PAGE_PROG);                     // send read memory location code

    // send memory address to write
    write_SPI((mem_Address>>24)&0x00FF);
    write_SPI((mem_Address>>16)&0x00FF);
    write_SPI((mem_Address>>8)&0x00FF);
    write_SPI(mem_Address&0x00FF);

    // Write the first 8 bytes
    // Write Date
    write_SPI(date[0]);
    write_SPI(date[1]);
    write_SPI(date[2]);

    // Write Time
    write_SPI(time[0]);
    write_SPI(time[1]);
    write_SPI(time[2]);
    
    // Write trigger event timing values
    // Write tick_sync value
    write_SPI((tick_sync>>8)&0x00FF);
    write_SPI(tick_sync&0x00FF);
   
    // Write value of the counter when trigger occured
    write_SPI((trigger_tick>>8)&0x00FF);
    write_SPI(trigger_tick&0x00FF);

    // Write trigger delay
    write_SPI((delay>>8)&0x00FF);
    write_SPI(delay&0x00FF);

    // Write write_tick #
    write_SPI((write_tick>>8)&0x00FF);
    write_SPI(write_tick&0x00FF);

    // Write tick_time value
    write_SPI((tick_time>>8)&0x00FF);
    write_SPI(tick_time&0x00FF);

    MEM_CS = 1;                               // disable chip select pin

    Nop();

    do
    {
        TMR1 = 0;
        while(TMR1 < 30);
        MEM_CS = 0;
        write_SPI(READ_FLAG_STATUS_REG);
        status = write_SPI(0);
        MEM_CS = 1;
    }while((status&0x80)==0);

    return status;

} // end of init_write_ADC_Trig


/* Function: unsigned char write_ADC_Trig(unsigned int mem_Address, short ADC1, short ADC2, short ADC3, short ADC4)
 *
 * Arguments:
 * 1. mem_Address: 4-Byte integer representing the starting memory address to write to
 * 2. ADC1: 16-bit value of the ADC conversion to write to memory
 * 3. ADC2: 16-bit value of the ADC conversion to write to memory
 * 4. ADC3: 16-bit value of the ADC conversion to write to memory
 * 5. ADC4: 16-bit value of the ADC conversion to write to memory
 *
 * Return Value:
 * 1. status: value of Status Flag Register
 *
 * Description: Function to write the ADC values to the memory location give by
 * mem_Address
 */
unsigned char write_ADC_Trig(unsigned int mem_Address, short ADC1, short ADC2, short ADC3, short ADC4)
{
    unsigned char status;

    Write_Enable();                           // enable chip writing

    MEM_CS = 0;                               // enable chip select pin for data transfer
    write_SPI(PAGE_PROG);                     // send read memory location code

    // send memory address to write
    write_SPI((mem_Address>>24)&0x00FF);
    write_SPI((mem_Address>>16)&0x00FF);
    write_SPI((mem_Address>>8)&0x00FF);
    write_SPI(mem_Address&0x00FF);

    // Write ADC values
    write_SPI((ADC1>>8)&0x00FF);              // write the upper 8 bits to memory
    write_SPI(ADC1&0x00FF);                   // write the lower 8 bits to memory
    write_SPI((ADC2>>8)&0x00FF);              // write the upper 8 bits to memory
    write_SPI(ADC2&0x00FF);                   // write the lower 8 bits to memory
    write_SPI((ADC3>>8)&0x00FF);              // write the upper 8 bits to memory
    write_SPI(ADC3&0x00FF);                   // write the lower 8 bits to memory
    write_SPI((ADC4>>8)&0x00FF);              // write the upper 8 bits to memory
    write_SPI(ADC4&0x00FF);                   // write the lower 8 bits to memory

    MEM_CS = 1;                               // disable chip select pin

    Nop();

    do
    {
        TMR1 = 0;
        while(TMR1 < 25);
        MEM_CS = 0;
        write_SPI(READ_FLAG_STATUS_REG);
        status = write_SPI(0);
        MEM_CS = 1;
    }while((status&0x80)==0);

    return status;

} // end of write_ADC_Trig


/* Function: void write_Mem(unsigned char cc_code, unsigned char mem_Address[], unsigned char data[], int length)
 *
 * Arguments:
 * 1. cc_code: command code to access register
 * 2. mem_Address[]: 4-Byte array representing the memory address to read
 * 3. data: Array of 1-byte of data to write to memory
 * 4. length: number of elements in data
 *
 * Return Value:
 * 1. status
 *
 * Description: Function to write data to the memory location give by mem_Address
 */
void write_Mem(unsigned char cc_code, unsigned int mem_Address, unsigned char data[], int length)
{
    int i;
    unsigned char status=0;
//    struct mem_data read_MEM_result;

    Write_Enable();                         // enable chip writing

    MEM_CS = 0;                             // enable chip select pin for data transfer
    write_SPI(cc_code);                     // send read memory location code

    // send memory address to write
    write_SPI((mem_Address>>24)&0x00FF);
    write_SPI((mem_Address>>16)&0x00FF);
    write_SPI((mem_Address>>8)&0x00FF);
    write_SPI(mem_Address&0x00FF);

    for(i=0; i<length; i++)
    {
        write_SPI(data[i]);
    }

    MEM_CS = 1;                             // disable chip select pin

    Nop();

    do
    {
        TMR1 = 0;
        while(TMR1 < 20);
        MEM_CS = 0;
        write_SPI(READ_FLAG_STATUS_REG);
        status = write_SPI(0);
        MEM_CS = 1;
    }while((status&0x80)==0);

} // end of write_Mem


/* Function: unsigned char subsector_Erase(unsigned int mem_Address)
 *
 * Arguments:
 * 1. mem_Address: Integer number representing the memory address to start the erase procedure
 *
 * Return Value:
 * 1. status: Value of the Status Flag Register
 *
 * Description: Function to erase a subsector of data (4096B) given by mem_Address
 */
unsigned char subsector_Erase(unsigned int mem_Address)
{
    unsigned char status;

    Write_Enable();                     // enable chip writing

    MEM_CS = 0;                         // enable chip select pin for data transfer
    write_SPI(SUBSECTOR_ERASE);                 // send read memory location code

    // send memory address to write
    write_SPI((mem_Address>>24)&0x00FF);
    write_SPI((mem_Address>>16)&0x00FF);
    write_SPI((mem_Address>>8)&0x00FF);
    write_SPI(mem_Address&0x00FF);

    MEM_CS = 1;                         // disable chip select pin

    Nop();

    do
    {
        TMR1 = 0;
        while(TMR1 < 12500);
        MEM_CS = 0;
        write_SPI(READ_FLAG_STATUS_REG);
        status = write_SPI(0);
        MEM_CS = 1;
    }while((status&0x80)==0);
    
    return status;

} // end of subsector_Erase


/* Function: unsigned char sector_Erase(unsigned int mem_Address)
 *
 * Arguments:
 * 1. mem_Address: Integer number representing the memory address to start the erase procedure
 *
 * Return Value:
 * 1. status: Value of the Status Flag Register
 *
 * Description: Function to erase a sector of data (64kB) given by mem_Address
 */
unsigned char sector_Erase(unsigned int mem_Address)
{
    unsigned char status;

    Write_Enable();                     // enable chip writing

    MEM_CS = 0;                         // enable chip select pin for data transfer
    write_SPI(SECTOR_ERASE);                 // send read memory location code

    // send memory address to erase
    write_SPI((mem_Address>>24)&0x00FF);
    write_SPI((mem_Address>>16)&0x00FF);
    write_SPI((mem_Address>>8)&0x00FF);
    write_SPI(mem_Address&0x00FF);

    MEM_CS = 1;                         // disable chip select pin

    Nop();

    do
    {
        TMR1 = 0;
        while(TMR1 < 12500);
        MEM_CS = 0;
        write_SPI(READ_FLAG_STATUS_REG);
        status = write_SPI(0);
        MEM_CS = 1;
    }while((status&0x80)==0);

    return status;

} // end of sector_Erase


/* Function: unsigned char die_Erase(unsigned int mem_Address)
 *
 * Arguments:
 * 1. mem_Address: Integer number representing the memory address to start the erase procedure
 *
 * Return Value:
 * 1. status: Value of the Status Flag Register
 *
 * Description: Function to erase a die (256 Mb) given by mem_Address
 */
unsigned char die_Erase(unsigned int mem_Address)
{
    unsigned char status;

    Write_Enable();                     // enable chip writing

    MEM_CS = 0;                         // enable chip select pin for data transfer
    write_SPI(DIE_ERASE);                 // send read memory location code

    // send memory address to write
    write_SPI((mem_Address>>24)&0x00FF);
    write_SPI((mem_Address>>16)&0x00FF);
    write_SPI((mem_Address>>8)&0x00FF);
    write_SPI(mem_Address&0x00FF);

    MEM_CS = 1;                         // disable chip select pin

    Nop();

    do
    {
        TMR1 = 0;
        while(TMR1 < 65000);
        MEM_CS = 0;
        write_SPI(READ_FLAG_STATUS_REG);
        status = write_SPI(0);
        MEM_CS = 1;
    }while((status&0x80)==0);

    return status;

} // end of die_Erase


/* Function: unsigned char XIP_Reset(void)
 *
 * Arguments: N/A
 *
 * Return Value:
 * 1. status: returns 3bits in byte form of the XIP status in the NV config register
 *
 * Description: Function to reset XIP mode
 */
unsigned char XIP_Reset(void)
{
    unsigned char i, status = 0;
    TMR1 = 0;
    MEM_CS = 1;
    MEM_CLK = 0;
    MEM_HOLD = 0;
    MEM_SDO = 0;

    for(i=0; i<110; i++)
    {

        switch (i)
        {
            case 7:
            case 17:
            case 31:
            case 49:
            case 75:
            case 109:
                MEM_CS = 1;
                MEM_HOLD = 0;
                MEM_SDO = 0;
                break;

            default:
                MEM_CS = 0;
                MEM_HOLD = 1;
                MEM_SDO = 1;
                break;
        }

        while(TMR1 > 100)
        {
            MEM_CLK = 1;
            TMR1 = 0;
        }
        while(TMR1 < 100)
        {
            MEM_CLK = 0;
            TMR1 = 0;
        }
    } // end for loop

    MEM_HOLD = 0;
    MEM_SDO = 0;
    MEM_CS = 1;

    TMR1 = 0;
    while(TMR1 < 2000);

    MEM_CS = 0;
    write_SPI(READ_NV_CONFIG_REG);
    status = write_SPI(0);              // bits 9:11 have required info to determine if reset worked
    write_SPI(0);                       // read and ignore second part of NV config register
    MEM_CS = 1;

    return (status & 0b00001110);

} // end XIP_Reset

/* Function: unsigned char Protocol_Reset(void)
 *
 * Arguments: N/A
 *
 * Return Value:
 * 1. status: returns 3 bits in byte form of the SPI mode in the NV config register
 *
 * Description: Function to reset SPI Protocol mode
 */
unsigned char Protocol_Reset(void)
{
    unsigned char status=0;

    MEM_HOLD = 0;
    MEM_SDO = 0;
    MEM_CS = 0;
    MEM_HOLD = 1;

    write_SPI(0xFF);
    MEM_SDO = 0;
    MEM_HOLD = 0;
    MEM_CS = 1;
    write_SPI(0x00);

    TMR1 = 0;
    while(TMR1 < 2000);

    MEM_CS = 0;
    write_SPI(READ_NV_CONFIG_REG);
    write_SPI(0);                   // read and ignore first part of NV config register
    status = write_SPI(0);          // bits 0, 2:3 have required info to determine if reset worked
    MEM_CS = 1;

    return (status & 0b00001101);

} // end Protocol_Reset


