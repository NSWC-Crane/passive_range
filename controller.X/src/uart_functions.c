#define _SUPPRESS_PLIB_WARNING 1

#include <xc.h>
#include <plib.h>


#include "../include/uart_functions.h"
#include "../include/dynamixel_protocol_v2.h"

//-----------------------------------------------------------------------------
/* Function: void send_char(unsigned char c)
 *
 * Arguments: 
 *  1. data: single byte of data
 *  2. uart: number of the uart to address
 *
 * Return Value: None
 *
 * Description: Function to send data out of UART
 */
void send_char(unsigned char data, unsigned char uart)
{
    switch(uart)
    {
        case 1:      
            while(U1STAbits.UTXBF);				// wait for at least one buffer slot to be open
            U1TXREG = data;
            break;
        case 2:
            while(U2STAbits.UTXBF);				// wait for at least one buffer slot to be open
            U2TXREG = data;
            break;
        default:
            break;
        
    }
}   // end of send_char


//-----------------------------------------------------------------------------
/* Function: unsigned char get_char(void)
 *
 * Arguments: None
 *  1. uart: number of the uart to address
 *
 * Return Value: 
 *  1. U2RXREG: single byte of data received from UART2
 *
 * Description: Function to read the UART2 receive register and make the value
 * available to the user
 */

unsigned char get_char(unsigned char uart)
{
    unsigned char temp = 0;
   
    switch(uart)
    {
        case 1:
            while(U1STAbits.URXDA == 0);        // wait for at least one byte in buffer

            temp = U1RXREG;

            if(U1STAbits.OERR == 1)             // check for overflow errors
            {
                U1STAbits.OERR = 0;             // clear overflow error
            }
            break;
            
        case 2:
            while(U2STAbits.URXDA == 0);        // wait for at least one byte in buffer

            temp = U2RXREG;

            if(U2STAbits.OERR == 1)             // check for overflow errors
            {
                U2STAbits.OERR = 0;             // clear overflow error
            }
            break;
            
        default:
            break;            
    }
    
    return temp;
    
}   // end of get_char



//-----------------------------------------------------------------------------
/* Function: void send_packet(unsigned char code)
 *
 * Arguments:
 * 1. command: command code to be sent
 * 2. length: length of the data to be sent
 * 3. *data: pointer to the data to be sent
 *
 * Return Value: None
 *
 * Description: Send message with command header, packet byte size, data
 */
void send_packet(unsigned char uart, unsigned char command, unsigned short length, unsigned char data[])
{
    unsigned short idx;
    
    send_char(command, uart);
    send_char(length, uart);

    for(idx=0; idx<length; ++idx)                 // send data and perform CRC calculations
    {
        send_char(data[idx], uart);
    }

}   // end of send_packet


