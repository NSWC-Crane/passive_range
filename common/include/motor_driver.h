#ifndef MOTOR_DRIVER_CLASS_H
#define MOTOR_DRIVER_CLASS_H

// C/C++ Includes
#include <cstdint>
#include <string>
#include <vector>

#include "ftd2xx_functions.h"
#include "data_packet.h"

//#define MAX_DATA_LENGTH		16
//#define MAX_PACKET_LENGTH	(MAX_DATA_LENGTH+2)

constexpr auto CMD_MOTOR_ENABLE = 0x10;       /* Enable/Disable motors */

// focus motor control
#define ZERO_FOCUS		 0x20				  /* zero the focus motor */
constexpr auto CMD_FOCUS_CTRL = 0x21;         /* Focus motor control */
constexpr auto ABS_FOCUS_CTRL = 0x22;         /* Absolute focus motor control */
constexpr auto GET_FOC_MOT_STEP = 0x23;       /* get the focus motor step count */
constexpr auto SET_FOC_MOT_SPD = 0x24;        /* set focus motor speed */
constexpr auto GET_FOC_MOT_SPD = 0x25;        /* get focus motor speed */

// zoom motor control
#define ZERO_ZOOM		 0x30				  /* zero the zoom motor */
constexpr auto CMD_ZOOM_CTRL = 0x31;          /* Zoom motor control */
constexpr auto ABS_ZOOM_CTRL = 0x32;          /* Absolute zoom motor control */
constexpr auto GET_ZM_MOT_STEP = 0x33;        /* get the zoom motor step count */
constexpr auto SET_ZM_MOT_SPD = 0x34;         /* set zoom motor speed */
constexpr auto GET_ZM_MOT_SPD = 0x35;         /* get zoom motor speed */

constexpr auto MD_FIRM_READ = 0x51;           /* Read firmware version return command */
constexpr auto MD_SER_NUM_READ = 0x52;        /* Read serial number return command */
constexpr auto MD_CONNECT = 0x53;             /* Check for data connection to motor controller */

//#define TRIG_CTRL        0x61                 /* Camera trigger control */

#define ENABLE_MOTOR     0x00                 /* Enable the motors */
#define DISABLE_MOTOR    0x01                 /* Disable the motors */
#define MOTOR_CW         0x80000000           /* turn the motor clockwise */
#define MOTOR_CCW        0x00000000           /* turn the motor coutner-clockwise */

const uint8_t motor_packet_size = 6;

extern const int max_focus_steps = 40575;
extern const int max_zoom_steps = 4628;

extern const int min_pw = 1000;
extern const int max_pw = 30000;

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
//typedef struct data_packet
//{
//    uint8_t start;
//    uint8_t command;
//    uint8_t byte_count;
//    std::vector<uint8_t> data;
//    //uint16_t checksum;
//
//
//    data_packet() : start('$'), command(0), byte_count(0)
//    {
//        data.clear();
//    }
//
//    data_packet(uint8_t com) : start('$'), command(com), byte_count(0)
//    {
//
//    }
//
//    data_packet(uint8_t com, uint8_t bc, std::vector<uint8_t> d) : start('$'), command(com), byte_count(bc)
//    {
//        data = d;
//    }
//
//    data_packet(uint8_t com, uint32_t d) : start('$'), command(com), byte_count(4)
//    {
//
//        data.clear();
//        data.push_back((d >> 24) & 0x00FF);
//        data.push_back((d >> 16) & 0x00FF);
//        data.push_back((d >> 8) & 0x00FF);
//        data.push_back(d & 0x00FF);
//    }
//
//    data_packet(std::vector<uint8_t> rx_data)
//    {
//        start = '$';
//        command = rx_data[0];
//        byte_count = rx_data[1];
//        try {
//            for (uint32_t idx = 0; idx < byte_count; ++idx)
//            {
//                data.push_back(rx_data[idx + 2]);
//            }
//        }
//        catch (std::exception e)
//        {
//            std::cout << "Error converting std::vector<uint8_t> rx_data to data_packet" << std::endl;
//            std::cout << e.what() << std::endl;
//        }
//    }
//
//    std::vector<uint8_t> to_vector()
//    {
//        std::vector<uint8_t> packet_data;
//
//        packet_data.push_back(start);
//        packet_data.push_back(command);
//        packet_data.push_back(byte_count);
//
//        for (uint32_t idx = 0; idx < data.size(); ++idx)
//            packet_data.push_back(data[idx]);
//
//        return std::move(packet_data);
//    }
//
//} data_packet;

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

    }   // end of receive_packet


    //-----------------------------------------------------------------------------
    bool step_focus_motor(FT_HANDLE md_handle, int32_t &focus_step, uint8_t mode = ABS_FOCUS_CTRL)
    {
        bool status = true;

        tx = data_packet(CMD_MOTOR_ENABLE, 1, { ENABLE_MOTOR });
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 3, rx);

        tx = data_packet(mode, focus_step);
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 6, rx);

        focus_step = (rx.data[0] << 24) | (rx.data[1] << 16) | (rx.data[2] << 8) | (rx.data[3]);

        tx = data_packet(CMD_MOTOR_ENABLE, 1, { DISABLE_MOTOR });
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 3, rx);

        return status;

    }   // end of step_focus_motor

    //-----------------------------------------------------------------------------
    bool step_zoom_motor(FT_HANDLE md_handle, int32_t& zoom_step, uint8_t mode = ABS_ZOOM_CTRL)
    {
        bool status = true;

        tx = data_packet(CMD_MOTOR_ENABLE, 1, { ENABLE_MOTOR });
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 3, rx);

        tx = data_packet(mode, zoom_step);
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 6, rx);

        zoom_step = (rx.data[0] << 24) | (rx.data[1] << 16) | (rx.data[2] << 8) | (rx.data[3]);

        tx = data_packet(CMD_MOTOR_ENABLE, 1, { DISABLE_MOTOR });
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 3, rx);

        return status;

    }   // end of step_focus_motor

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
