#ifndef DYNAMIXEL_PROTOCOL_V2_H_
#define	DYNAMIXEL_PROTOCOL_V2_H_

#define MAX_PACKET_SIZE 192

#define ID              4
#define LENGTH          5
#define INSTRUCTION     7
#define PARAMETER       8

#define ERRBIT          4

//-----------------------------------------------------------------------------
enum instruction{ 
    DYN_PING = 0x01, 
    DYN_READ = 0x02, 
    DYN_WRITE = 0x03, 
    DYN_WRITE_REG = 0x04, 
    DYN_ACTION = 0x05, 
    DYN_FACTORY_RESET = 0x06,
    DYN_REBOOT = 0x08, 
    DYN_CLEAR = 0x10, 
    DYN_STATUS = 0x55, 
    DYN_SYNC_READ = 0x82, 
    DYN_SYNC_WRITE = 0x83, 
    DYN_BULK_READ = 0x92, 
    DYN_BULK_WRITE = 0x93
    };
  
//-----------------------------------------------------------------------------
enum control_value {
    ADD_MODEL = 0,
    ADD_OPERATING_MODE = 11,
    ADD_TORQUE_ENABLE = 64,
    ADD_LED = 65,
    ADD_POSITION_D_GAIN = 80,
    ADD_POSITION_I_GAIN = 82,
    ADD_POSITION_P_GAIN = 86,
    ADD_GOAL_POSITION = 116,
    ADD_MOVING = 122,
    ADD_PRESENT_POSITION = 132
    };
    
//-----------------------------------------------------------------------------
enum error_bit{ 
    DYN_ERRBIT_VOLTAGE=1, 
    DYN_ERRBIT_ANGLE=2, 
    DYN_ERRBIT_OVERHEAT=4, 
    DYN_ERRBIT_RANGE=8, 
    DYN_ERRBIT_CHECKSUM=16, 
    DYN_ERRBIT_OVERLOAD=32, 
    ERRBIT_INSTRUCTION=64
    };

//-----------------------------------------------------------------------------
enum results{ 
    DYN_TXSUCCESS=0, 
    DYN_RXSUCCESS=1, 
    DYN_TXFAIL=2, 
    DYN_RXFAIL=3,
    DYN_TXERROR=4,
    DYN_RXWAITING=5,
    DYN_RXTIMEOUT=6,
    DYN_RXCORRUPT=7
    };
    
    
//-----------------------------------------------------------------------------
/*
Dynamixel Protocol V2 Instruction Packet
Header1, Header2, Header3, Reserved, Packet_ID, Length1, Length2, Instruction, Param,  Param, Param,  CRC1,  CRC2
0xFF,    0xFF,    0xFD,    0x00,     ID,        Len_L,   Len_H,   Instruction, Param1, ...,   ParamN, CRC_L, CRC_H

Dynamixel Protocol V2 Response Packet

*/
//-----------------------------------------------------------------------------
typedef struct data_packet
{
       
    unsigned char data[MAX_PACKET_SIZE];
    
} data_packet;


data_packet initialize_packet(void);

void build_packet(unsigned char id, unsigned short param_length, unsigned char instruction, unsigned char params[], unsigned char data[]);

unsigned short make_uint16(unsigned char lower_byte, unsigned char upper_byte);

void split_uint16(unsigned short data, unsigned char *lower_byte, unsigned char *upper_byte);

unsigned short calculate_crc(unsigned short data_size, unsigned char *data);



#endif  // DYNAMIXEL_PROTOCOL_V2_H_
