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

// configure the trigger parameters
constexpr uint8_t CONFIG_T1 = 0x11;           /* Configure Trigger 1 parameters */
constexpr uint8_t CONFIG_T2 = 0x12;           /* Configure Trigger 2 parameters */

// initiate triggers
constexpr uint8_t TRIG_ALL = 0x20;            /* initiate trigger sequence */
constexpr uint8_t TRIG_CH1 = 0x21;            /* pulse channel 1 */
constexpr uint8_t TRIG_CH2 = 0x22;            /* pulse channel 2 */

// motor control commands
constexpr uint8_t MOTOR_CTRL_PING = (uint8_t)0x30;
constexpr uint8_t MOTOR_CTRL_RD = (uint8_t)0x31;        /* Initiate motor control */
constexpr uint8_t MOTOR_CTRL_WR = (uint8_t)0x32;        /* Initiate motor control */

// firmware read info
constexpr uint8_t MD_FIRM_READ = (uint8_t)0x51;           /* Read firmware version return command */
constexpr uint8_t MD_SER_NUM_READ = (uint8_t)0x52;        /* Read serial number return command */
constexpr uint8_t MD_CONNECT = (uint8_t)0x53;             /* Check for data connection to motor controller */

// motor specific parameters
constexpr uint8_t FOCUS_MOTOR_ID = (uint8_t)10;         /* The ID for the focus motor */
constexpr uint8_t ZOOM_MOTOR_ID = (uint8_t)20;          /* The ID for the zoom motor */
constexpr uint8_t BROADCAST_ID = (uint8_t)254;

constexpr auto ENABLE_MOTOR = 1;                 /* Enable the motors */
constexpr auto DISABLE_MOTOR = 0;                /* Disable the motors */

const uint8_t packet_size = (uint8_t)2;
const uint8_t read_sp_size = (uint8_t)(15 + packet_size);
const uint8_t ping_sp_size = (uint8_t)(14 + packet_size);
const uint8_t write_sp_size = (uint8_t)(11 + packet_size);

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
        model = (data[SP_PARAMS_POS + 1] << 8) | data[SP_PARAMS_POS];
        firmware = data[SP_PARAMS_POS + 2];
    }

    inline friend std::ostream& operator<< (
        std::ostream& out,
        const motor_info& item
        )
    {
        out << "  ID:               " << (uint32_t)item.id << std::endl;
        out << "  Model Number:     " << (uint32_t)item.model << std::endl;
        out << "  Firmware Version: " << (uint32_t)item.firmware << std::endl;
        return out;
    }

} motor_info;



typedef struct trigger_info
{
    uint8_t num;
    uint8_t polarity;
    uint32_t offset;
    uint32_t length;

    trigger_info() = default;

    trigger_info(uint8_t n_, uint8_t p_, uint32_t o_, uint32_t l_) : num(n_), polarity(p_), offset(o_), length(l_) {}

    trigger_info(uint8_t n_, std::vector<uint8_t> data) : num(n_)
    {
        polarity = data[0];
    }

    inline friend std::ostream& operator<< (
        std::ostream& out,
        const trigger_info& item
        )
    {
        out << "Trigger Information: " << std::endl;
        out << "  ID:               " << (uint32_t)item.num << std::endl;
        out << "  Polarity:         " << (uint32_t)item.polarity << std::endl;
        out << "  Offset:           " << (uint32_t)item.offset << std::endl;
        out << "  Length:           " << (uint32_t)item.length << std::endl;
        return out;
    }

} trigger_info;

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
    bool ping_motor(FT_HANDLE md_handle, uint8_t id, motor_info &mi)
    {
        bool status = true;
        uint8_t mtr_error = 0;

        dynamixel_packet mtr_packet(id, DYN_PING);
        tx = data_packet(MOTOR_CTRL_PING, (uint8_t)mtr_packet.get_size(), mtr_packet.get_packet_array());
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, ping_sp_size, rx);

        if (status == true)
        {
            //mtr_error = rx.data[SP_ERROR_POS];

            if (rx.data[SP_ERROR_POS] == 0)
            {
                mi = motor_info(rx.data);
            }
            else
            {
                status = false;
            }
        }

        return status;

    }   // end of ping_motor

    //-----------------------------------------------------------------------------
    bool enable_motor(FT_HANDLE md_handle, uint8_t id, bool value)
    {
        bool status = true;

        dynamixel_packet mtr_packet(id, DYN_WRITE);

        if (value)
        {
            mtr_packet.add_params((uint16_t)ADD_TORQUE_ENABLE, (uint8_t)ENABLE_MOTOR);
        }
        else
        {
            mtr_packet.add_params((uint16_t)ADD_TORQUE_ENABLE, (uint8_t)DISABLE_MOTOR);
        }

        // send the enable/disable motor packet
        tx = data_packet(MOTOR_CTRL_WR, (uint8_t)mtr_packet.get_size(), mtr_packet.get_packet_array());
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, write_sp_size, rx);
        status &= (rx.data[SP_ERROR_POS] == 0);

        if (status == false)
        {
            std::cout << "Error enabling motor id: " << (uint32_t)id << ".  Error: " << mtr_error_string[rx.data[SP_ERROR_POS]] << std::endl;
        }

        return status;

    }   // end of enable_motor

    //-----------------------------------------------------------------------------
    bool set_position(FT_HANDLE md_handle, uint8_t id, int32_t &step)
    {
        bool status = true;

        dynamixel_packet mtr_packet(id, DYN_WRITE);

        // step the focus motor
        mtr_packet.add_params((uint16_t)ADD_GOAL_POSITION, (uint32_t)step);
        
        tx = data_packet(MOTOR_CTRL_WR, (uint8_t)mtr_packet.get_size(), mtr_packet.get_packet_array());
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, write_sp_size, rx);
        status &= (rx.data[SP_ERROR_POS] == 0);

        if (status == false)
        {
            std::cout << "Error setting position for motor id: " << (uint32_t)id << ".  Error: " << mtr_error_string[rx.data[SP_ERROR_POS]] << std::endl;
        }

        return status;

    }   // end of step_motor

    //-----------------------------------------------------------------------------
    bool get_position(FT_HANDLE md_handle, uint8_t id, int32_t& step)
    {
        bool status = true;
        dynamixel_packet mtr_packet(id, DYN_READ);
        mtr_packet.add_params((uint16_t)ADD_PRESENT_POSITION, (uint16_t)4);

        tx = data_packet(MOTOR_CTRL_RD, (uint8_t)mtr_packet.get_size(), mtr_packet.get_packet_array());
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, read_sp_size, rx);
        //uint8_t mtr_error = rx.data[SP_ERROR_POS];

        if ((status == true) && (rx.data[SP_ERROR_POS] == 0))
        {
            step = (rx.data[SP_PARAMS_POS + 3] << 24) | (rx.data[SP_PARAMS_POS + 2] << 16) | (rx.data[SP_PARAMS_POS + 1] << 8) | (rx.data[SP_PARAMS_POS]);
        }
        else
        {
            std::cout << "Error getting position for motor id: " <<  (uint32_t)id << ".  Error: " << mtr_error_string[rx.data[SP_ERROR_POS]] << std::endl;
        }
        return status;
    }   // end of get_position

    //-----------------------------------------------------------------------------
    bool reset_motor(FT_HANDLE md_handle, uint8_t id)
    {
        bool status = true;

        dynamixel_packet mtr_packet(id, DYN_REBOOT);
        tx = data_packet(MOTOR_CTRL_RD, (uint8_t)mtr_packet.get_size(), mtr_packet.get_packet_array());
        
        send_packet(md_handle, tx);
        status &= receive_packet(md_handle, write_sp_size, rx);
        status &= (rx.data[SP_ERROR_POS] == 0);

        if(status == false)
        {
            std::cout << "Error reseting motor id: " << (uint32_t)id << ".  Error: " << mtr_error_string[rx.data[SP_ERROR_POS]] << std::endl;
        }

        return status;

    }   // end of reset_motor

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
