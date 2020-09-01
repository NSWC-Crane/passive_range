#ifndef DYNAMIXEL_PROTOCOL1_PACKET_H
#define DYNAMIXEL_PROTOCOL1_PACKET_H

// C/C++ Includes
#include <cstdint>
#include <string>
#include <vector>


enum instruction {
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

enum control_value {
    ADD_MODEL = 0,
    ADD_OPERATING_MODE = 11,
    ADD_TORQUE_ENABLE = 64,
    ADD_LED = 65,
    ADD_GOAL_POSITION = 116,
    ADD_PRESENT_POSITION = 132
    
    };

constexpr auto ID_POS = 0;
constexpr auto LENGTH_POS = 1;
constexpr auto INSTR_POS = 3;
constexpr auto IP_ADDRESS_POS = 4;
constexpr auto SP_ERROR_POS = 4;
constexpr auto IP_DATA_POS = 6;
constexpr auto SP_DATA_POS = 5;


inline std::vector<uint8_t> split_uint16(unsigned short value)
{
    std::vector<uint8_t> data(2);

    data[0] = (uint8_t)(value & 0x00FF);
    data[1]= (uint8_t)((value >> 8) & 0x00FF);
    return data;
}

inline std::vector<uint8_t> split_uint32(uint32_t value)
{
    std::vector<uint8_t> data(4);

    data[0] = (uint8_t)(value & 0x00FF);
    data[1] = (uint8_t)((value >> 8) & 0x00FF);
    data[2] = (uint8_t)((value >> 16) & 0x00FF);
    data[3] = (uint8_t)((value >> 24) & 0x00FF);
    return data;
}

class dynamixel_packet
{
public:
    
    std::vector<uint8_t> data;
    
    dynamixel_packet() = default;
    
    dynamixel_packet(uint8_t id, uint16_t length, uint8_t instruction)
    {
        data.clear();
        add_uint8(id);
        add_uint16(length);
        add_uint8(instruction);
    }

    //dynamixel_packet(uint8_t id, uint16_t length, uint8_t instruction, std::vector<uint8_t> d)
    //{
    //    data.clear();
    //    add_uint8(id);
    //    add_uint16(length);
    //    add_uint8(instruction);
    //    std::copy(d.begin(), d.end(), std::back_inserter(data));
    //}

    dynamixel_packet(uint8_t id, uint16_t length, uint8_t instruction, uint16_t address, std::vector<uint8_t> d)
    {
        data.clear();
        add_uint8(id);
        add_uint16(length);
        add_uint8(instruction);
        add_uint16(address);
        std::copy(d.begin(), d.end(), std::back_inserter(data));
    }
    

    void write_address(uint16_t value)
    {
        data[IP_DATA_POS] = (uint8_t)(value & 0x00FF);
        data[IP_DATA_POS+1] = (uint8_t)((value >> 8) & 0x00FF);
    }

    void write_length(uint16_t value)
    {
        data[LENGTH_POS] = (uint8_t)(value & 0x00FF);
        data[LENGTH_POS + 1] = (uint8_t)((value >> 8) & 0x00FF);
    }


    void write_instruction(uint8_t value)
    {
        data[INSTR_POS] = (uint8_t)(value & 0x00FF);
    }


    void add_uint8(uint8_t d)
    {
        data.push_back(d);
    }
    
    void add_uint16(uint16_t d)
    {
        data.push_back((uint8_t)(d&0x00FF));
        data.push_back((uint8_t)((d>>8)&0x00FF));
    }
    
    void add_uint32(uint32_t d)
    {
        data.push_back((uint8_t)(d&0x00FF));
        data.push_back((uint8_t)((d>>8)&0x00FF));
        data.push_back((uint8_t)((d>>16)&0x00FF));
        data.push_back((uint8_t)((d>>24)&0x00FF));
    }
    
    //void init(uint8_t id, uint16_t length, uint8_t instruction)
    //{
    //    data.clear();
    //    add_uint8(id);
    //    add_uint16(length);
    //    add_uint8(instruction);
    //}
    
    
private:
    
    
    
};


#endif  // DYNAMIXEL_PROTOCOL1_PACKET_H
