#ifndef MOTOR_DRIVER_CLASS_H
#define MOTOR_DRIVER_CLASS_H

// C/C++ Includes
#include <cstdint>
#include <string>
#include <vector>

#include "ftd2xx_functions.h"

//#define MAX_DATA_LENGTH		16
//#define MAX_PACKET_LENGTH	(MAX_DATA_LENGTH+2)

#define CMD_MOTOR_ENABLE    0x30                /* Enable/Disable motors */
#define CMD_FOCUS_CTRL      0x31                /* Focus motor control */
#define CMD_ZOOM_CTRL       0x32                /* Zoom motor control */
#define CMD_ALL_CTRL        0x33                /* Both motor control */
#define ZERO_FOCUS		    0x35                /* zero the focus motor */
#define ZERO_ZOOM		    0x36                /* zero the zoom motor */
#define ZERO_ALL            0x37                /* zero all motors */

#define SET_FOC_MOT_SPD     0x40                /* set focus motor speed */
#define GET_FOC_MOT_SPD     0x41                /* get focus motor speed */
#define SET_ZM_MOT_SPD      0x42                /* set zoom motor speed */
#define GET_ZM_MOT_SPD      0x43                /* get zoom motor speed */

#define FIRM_READ           0x51                /* Read firmware version return command */
#define SER_NUM_READ        0x52                /* Read serial number return command */
#define CONNECT             0x53                /* Check for data connection to motor controller */

//#define TRIG_CTRL           0x61                /* Camera trigger control */

#define ENABLE_MOTOR        0x00                /* Enable the motors */
#define DISABLE_MOTOR       0x01                /* Disable the motors */
#define MOTOR_CW            0x80000000          /* turn the motor clockwise */
#define MOTOR_CCW           0x00000000          /* turn the motor coutner-clockwise */

const uint8_t motor_packet_size = 6;

//-----------------------------------------------------------------------------
typedef struct motor_driver_info
{
    uint8_t serial_number;
    uint8_t firmware[2];

    motor_driver_info() {}

    motor_driver_info(std::vector<uint8_t> data)
    {
        if ((data[0] == 1) && (data.size() >= 4))
        {
            serial_number = data[1];
            firmware[0] = data[2];
            firmware[1] = data[3];
        }
    }

} motor_driver_info;

//-----------------------------------------------------------------------------
typedef struct data_packet
{
    uint8_t start;
    uint8_t command;
    uint8_t byte_count;
    std::vector<uint8_t> data;
    //uint16_t checksum;


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
            for (uint32_t idx = 0; idx < byte_count; ++idx)
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

//-----------------------------------------------------------------------------
class motor_driver
{

public:

    motor_driver_info md_info;
    data_packet tx;
    data_packet rx;

    motor_driver() {}

    void set_driver_info(data_packet packet)
    {
        md_info = motor_driver_info(packet.data);
    }

    //-----------------------------------------------------------------------------
    bool send_packet(FT_HANDLE driver, data_packet packet)
    {

        return send_data(driver, packet.to_vector());

    }	// end of send_packet

    //-----------------------------------------------------------------------------
    bool receive_packet(FT_HANDLE driver, uint32_t count, data_packet &packet)
    {

        std::vector<uint8_t> rx_data;

        bool status = receive_data(driver, count, rx_data);
        
        if(status)
            packet = data_packet(rx_data);

        return status;

    }   // end of receive_lens_packet


};   // end of class

//-----------------------------------------------------------------------------

inline std::ostream& operator<< (
    std::ostream& out,
    const motor_driver& item
    )
{
    using std::endl;
    out << "Motor Controller Information: " << std::endl;
    out << "  Serial Number:    " << (uint32_t)item.md_info.serial_number << std::endl;
    out << "  Firmware Version: " << (uint32_t)item.md_info.firmware[0] << "." << std::setfill('0') << std::setw(2) << (uint32_t)item.md_info.firmware[1] << std::endl;
    return out;
}


#endif  // MOTOR_DRIVER_CLASS_H
