#ifndef PIC32_UART_CTRL_H_
#define PIC32_UART_CTRL_H_

#include "../include/dynamixel_protocol_v2.h"

//-----------------------------------------------------------------------------
void send_char(unsigned char data, unsigned char uart);

unsigned char get_char(unsigned char uart);

void send_packet(unsigned char uart, unsigned char command, unsigned short length, unsigned char data[]);

void flush_uart(unsigned char uart);

#endif  // PIC32_UART_CTRL_H_
