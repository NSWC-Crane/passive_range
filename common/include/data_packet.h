#ifndef DATA_PACKET_STRUCT_H
#define DATA_PACKET_STRUCT_H

// C/C++ Includes
#include <cstdint>
#include <string>
#include <vector>


//-----------------------------------------------------------------------------
typedef struct data_packet
{
    uint8_t start;
    uint8_t command;
    uint8_t byte_count;
    std::vector<uint8_t> data;

    data_packet() : start('$'), command(0), byte_count(0)
    {
        data.clear();
    }

    data_packet(uint8_t com) : start('$'), command(com), byte_count(0)
    {

    }

    data_packet(uint8_t com, uint8_t bc, std::vector<uint8_t> d) : start('$'), command(com), byte_count(bc)
    {
        data = d;
    }

    data_packet(uint8_t com, uint32_t d) : start('$'), command(com), byte_count(4)
    {

        data.clear();
        data.push_back((d >> 24) & 0x00FF);
        data.push_back((d >> 16) & 0x00FF);
        data.push_back((d >> 8) & 0x00FF);
        data.push_back(d & 0x00FF);
    }

    data_packet(std::vector<uint8_t> rx_data)
    {
        start = '$';
        command = rx_data[0];
        byte_count = rx_data[1];
        try {
            for (size_t idx = 0; idx < byte_count; ++idx)
            {
                data.push_back(rx_data[idx + 2]);
            }
        }
        catch (std::exception e)
        {
            std::cout << "Error converting std::vector<uint8_t> rx_data to data_packet" << std::endl;
            std::cout << e.what() << std::endl;
        }
    }

    std::vector<uint8_t> to_vector()
    {
        std::vector<uint8_t> packet_data;

        packet_data.push_back(start);
        packet_data.push_back(command);
        packet_data.push_back(byte_count);

        for (uint32_t idx = 0; idx < data.size(); ++idx)
            packet_data.push_back(data[idx]);

        return std::move(packet_data);
    }

} data_packet;

#endif  // DATA_PACKET_STRUCT_H
