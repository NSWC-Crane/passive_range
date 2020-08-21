#ifndef DYNAMIXEL_PROTOCOL1_PACKET_H
#define DYNAMIXEL_PROTOCOL1_PACKET_H

// C/C++ Includes
#include <cstdint>
#include <string>
#include <vector>


enum instruction{ PING = 0x01, READ = 0x02, WRITE = 0x03, WRITE_REG = 0x04, ACTION = 0x05, FACTORY_RESET = 0x06,
                REBOOT = 0x08, CLEAR = 0x10, STATUS = 0x55, SYNC_READ = 0x82, SYNC_WRITE = 0x83, BULK_READ = 0x92, BULK_WRITE = 0x93};




#endif  // DYNAMIXEL_PROTOCOL1_PACKET_H
