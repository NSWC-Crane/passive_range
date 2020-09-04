#ifndef MOTOR_DRIVER_CLASS_H
#define MOTOR_DRIVER_CLASS_H

// C/C++ Includes
#include <cstdint>
#include <string>
#include <vector>

#include "ftd2xx_functions.h"
#include "data_packet.h"
#include "dynamixel_packet_v2.h"

//#define MAX_DATA_LENGTH		16
//#define MAX_PACKET_LENGTH	(MAX_DATA_LENGTH+2)

//constexpr auto CMD_MOTOR_ENABLE = 0x10;       /* Enable/Disable motors */

// motor control
constexpr uint8_t FOCUS_MOTOR_ID = (uint8_t)10;         /* The ID for the focus motor */
constexpr uint8_t ZOOM_MOTOR_ID = (uint8_t)20;          /* The ID for the zoom motor */
constexpr uint8_t BROADCAST_ID = (uint8_t)254;

constexpr uint8_t MOTOR_CTRL = (uint8_t)0x20;           /* Initiate motor control */

constexpr uint8_t GET_MOTOR_STEP = (uint8_t)0x21;       /* get the step count for a given motor ID */

//// focus motor control
//constexpr auto CMD_FOCUS_CTRL = 0x21;       /* Focus motor control */
//constexpr auto ABS_FOCUS_CTRL = 0x22;       /* Absolute focus motor control */
//constexpr auto GET_FOC_MOT_STEP = 0x23;     /* get the focus motor step count */
//constexpr auto SET_FOC_MOT_SPD = 0x24;      /* set focus motor speed */
//constexpr auto GET_FOC_MOT_SPD = 0x25;      /* get focus motor speed */
//
//// zoom motor control
//constexpr auto CMD_ZOOM_CTRL = 0x31;          /* Zoom motor control */
//constexpr auto ABS_ZOOM_CTRL = 0x32;          /* Absolute zoom motor control */
//constexpr auto GET_ZM_MOT_STEP = 0x33;        /* get the zoom motor step count */
//constexpr auto SET_ZM_MOT_SPD = 0x34;         /* set zoom motor speed */
//constexpr auto GET_ZM_MOT_SPD = 0x35;         /* get zoom motor speed */

constexpr uint8_t MD_FIRM_READ = (uint8_t)0x51;           /* Read firmware version return command */
constexpr uint8_t MD_SER_NUM_READ = (uint8_t)0x52;        /* Read serial number return command */
constexpr uint8_t MD_CONNECT = (uint8_t)0x53;             /* Check for data connection to motor controller */

constexpr auto ENABLE_MOTOR = 1;                 /* Enable the motors */
constexpr auto DISABLE_MOTOR = 0;                /* Disable the motors */
//#define MOTOR_CW         0x80000000           /* turn the motor clockwise */
//#define MOTOR_CCW        0x00000000           /* turn the motor coutner-clockwise */

const uint8_t motor_packet_size = (uint8_t)6;

extern const int max_focus_steps = 40575;
extern const int max_zoom_steps = 4628;

//extern const int min_pw = 1000;
//extern const int max_pw = 30000;

//-----------------------------------------------------------------------------
typedef struct motor_driver_info
{
    uint8_t serial_number;
    uint8_t firmware[2];

    motor_driver_info() = default;

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

typedef struct motor_info
{
    uint8_t id;
    uint16_t model;
    uint8_t firmware;

    motor_info() = default;

    motor_info(std::vector<uint8_t> data)
    {
        id = data[ID_POS];
        model = (data[SP_DATA_POS + 1] << 8) | data[SP_DATA_POS];
        firmware = data[SP_DATA_POS + 2];
    }

    inline friend std::ostream& operator<< (
        std::ostream& out,
        const motor_info& item
        )
    {
        out << "Motor Information: " << std::endl;
        out << "  ID:               " << (uint32_t)item.id << std::endl;
        out << "  Model Number:     " << (uint32_t)item.model << std::endl;
        out << "  Firmware Version: " << (uint32_t)item.firmware << std::endl;
        return out;
    }

} motor_info;

//-----------------------------------------------------------------------------
class motor_driver
{

public:

    motor_driver_info md_info;
    data_packet tx;
    data_packet rx;

    motor_driver() = default;

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
    bool ping_motors(FT_HANDLE md_handle)
    {
        bool status = true;

        dynamixel_packet dyn_packet(FOCUS_MOTOR_ID, (uint16_t)3, DYN_PING);
        tx = data_packet(MOTOR_CTRL, (uint8_t)dyn_packet.data.size(), dyn_packet.data);
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 8, rx);

        motor_info focus_motor(rx.data);

        dyn_packet = dynamixel_packet(ZOOM_MOTOR_ID, (uint16_t)3, DYN_PING);
        tx = data_packet(MOTOR_CTRL, (uint8_t)dyn_packet.data.size(), dyn_packet.data);
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 8, rx);

        motor_info zoom_motor(rx.data);


        std::cout << focus_motor << std::endl;
        std::cout << zoom_motor << std::endl;

        return status;
    }

    //-----------------------------------------------------------------------------
    bool set_position(FT_HANDLE md_handle, uint8_t id, int32_t &step)
    {
        bool status = true;

        dynamixel_packet dyn_packet(id, (uint16_t)4, DYN_WRITE, ADD_TORQUE_ENABLE, { ENABLE_MOTOR });

        // enable the focus motor
        tx = data_packet(MOTOR_CTRL, (uint8_t)dyn_packet.data.size(), dyn_packet.data);
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 3, rx);

        // step the focus motor
        dyn_packet  = dynamixel_packet(id, (uint16_t)4, DYN_WRITE, ADD_GOAL_POSITION, split_uint32(step));
        tx = data_packet(MOTOR_CTRL, (uint8_t)dyn_packet.data.size(), dyn_packet.data);
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 7, rx);

        step = (rx.data[SP_DATA_POS] << 24) | (rx.data[SP_DATA_POS+1] << 16) | (rx.data[SP_DATA_POS+2] << 8) | (rx.data[SP_DATA_POS+3]);

        // disable the focus motor
        dyn_packet = dynamixel_packet(id, (uint16_t)4, DYN_WRITE, ADD_TORQUE_ENABLE, { DISABLE_MOTOR });
        tx = data_packet(MOTOR_CTRL, (uint8_t)dyn_packet.data.size(), dyn_packet.data);
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 3, rx);

        return status;

    }   // end of step_motor

    //-----------------------------------------------------------------------------
    bool get_position(FT_HANDLE md_handle, uint8_t id, int32_t& step)
    {
        bool status = true;
        dynamixel_packet dyn_packet(id, (uint16_t)5, DYN_WRITE, ADD_PRESENT_POSITION);

        tx = data_packet(MOTOR_CTRL, (uint8_t)dyn_packet.data.size(), dyn_packet.data);
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 11, rx);

        return status;
    }
/*
    bool step_zoom_motor(FT_HANDLE md_handle, int32_t& zoom_step, uint8_t mode)
    {
        bool status = true;
        dynamixel_packet dyn_packet(ZOOM_MOTOR_ID, (uint16_t)4, DYN_WRITE, ADD_TORQUE_ENABLE, { 1 });


        tx = data_packet(MOTOR_CTRL, 1, { ENABLE_MOTOR });
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 3, rx);

        tx = data_packet(MOTOR_CTRL, zoom_step);
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 6, rx);

        zoom_step = (rx.data[0] << 24) | (rx.data[1] << 16) | (rx.data[2] << 8) | (rx.data[3]);

        tx = data_packet(MOTOR_CTRL, 1, { DISABLE_MOTOR });
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, 3, rx);

        return status;

    }   // end of step_focus_motor
*/



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
