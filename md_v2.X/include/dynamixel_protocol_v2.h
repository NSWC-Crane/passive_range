#ifndef DYNAMIXEL_PROTOCOL_V2_
#define	DYNAMIXEL_PROTOCOL_V2_

#define MAX_PACKET_SIZE 192;



enum instruction{ 
    PING = 0x01, 
    READ = 0x02, 
    WRITE = 0x03, 
    WRITE_REG = 0x04, 
    ACTION = 0x05, 
    FACTORY_RESET = 0x06,
    REBOOT = 0x08, 
    CLEAR = 0x10, 
    STATUS = 0x55, 
    SYNC_READ = 0x82, 
    SYNC_WRITE = 0x83, 
    BULK_READ = 0x92, 
    BULK_WRITE = 0x93
    };
                
enum error_bit{ 
    ERRBIT_VOLTAGE=1, 
    ERRBIT_ANGLE=2, 
    ERRBIT_OVERHEAT=4, 
    ERRBIT_RANGE=8, 
    ERRBIT_CHECKSUM=16, 
    ERRBIT_OVERLOAD=32, 
    ERRBIT_INSTRUCTION=64
    };


enum results{ 
    TXSUCCESS=0, 
    RXSUCCESS=1, 
    TXFAIL=2, 
    RXFAIL=3,
    TXERROR=4,
    RXWAITING=5,
    RXTIMEOUT=6,
    RXCORRUPT=7
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

void build_packet(unsigned char id, unsigned short length, unsigned char instruction, unsigned char *params, data_packet packet );

int send_packet(data_packet packet);

unsigned short make_uint16(unsigned char lower_byte, unsigned char upper_byte);

void split_uint16(unsigned short data, unsigned char *lower_byte, unsigned char *upper_byte);





#endif  // DYNAMIXEL_PROTOCOL_V2_
