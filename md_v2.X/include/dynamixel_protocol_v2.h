#ifndef DYNAMIXEL_PROTOCOL_V2_
#define	DYNAMIXEL_PROTOCOL_V2_


//-----------------------------------------------------------------------------
/*
Dynamixel Protocol V2 Instruction Packet
Header1, Header2, Header3, Reserved, Packet_ID, Length1, Length2, Instruction, Param,  Param, Param,  CRC1,  CRC2
0xFF,    0xFF,    0xFD,    0x00,     ID,        Len_L,   Len_H,   Instruction, Param1, ...,   ParamN, CRC_L, CRC_H

Dynamixel Protocol V2 Response Packet

*/

//-----------------------------------------------------------------------------
typedef struct data_packet











#endif  // DYNAMIXEL_PROTOCOL_V2_
