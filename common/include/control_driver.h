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
constexpr uint8_t CONFIG_T1 = (uint8_t)0x11;            /* Configure Trigger 1 parameters */
constexpr uint8_t CONFIG_T2 = (uint8_t)0x12;            /* Configure Trigger 2 parameters */
constexpr uint8_t TRIG_CONFIG = (uint8_t)0x15;          /* Get the trigger config for each channel */

// initiate triggers
constexpr uint8_t TRIG_ALL = (uint8_t)0x20;             /* initiate trigger sequence */
constexpr uint8_t TRIG_CH1 = (uint8_t)0x21;             /* pulse channel 1 */
constexpr uint8_t TRIG_CH2 = (uint8_t)0x22;             /* pulse channel 2 */

// motor control commands
constexpr uint8_t MOTOR_CTRL_PING = (uint8_t)0x30;      /* Ping the motors to get their info */
constexpr uint8_t MOTOR_CTRL_WR = (uint8_t)0x31;        /* Initiate motor write command */
constexpr uint8_t MOTOR_CTRL_RD1 = (uint8_t)0x32;       /* Send read 1 byte command to motor */
constexpr uint8_t MOTOR_CTRL_RD2 = (uint8_t)0x33;       /* Send read 2 byte command to motor */
constexpr uint8_t MOTOR_CTRL_RD4 = (uint8_t)0x34;       /* Send read 4 byte command to motor */

// firmware read info
constexpr uint8_t DRIVER_FIRM_READ = (uint8_t)0x51;         /* Read firmware version return command */
constexpr uint8_t DRIVER_SER_NUM_READ = (uint8_t)0x52;      /* Read serial number return command */
constexpr uint8_t DRIVER_CONNECT = (uint8_t)0x53;           /* Check for data connection to motor controller */

// motor specific parameters
constexpr uint8_t FOCUS_MOTOR_ID = (uint8_t)10;         /* The ID for the focus motor */
constexpr uint8_t ZOOM_MOTOR_ID = (uint8_t)20;          /* The ID for the zoom motor */
constexpr uint8_t BROADCAST_ID = (uint8_t)254;

constexpr auto ENABLE_MOTOR = 1;                        /* Enable the motors */
constexpr auto DISABLE_MOTOR = 0;                       /* Disable the motors */

const uint8_t packet_size = (uint8_t)2;
const uint8_t read_sp_size = (uint8_t)(15 + packet_size);
const uint8_t ping_sp_size = (uint8_t)(14 + packet_size);
const uint8_t write_sp_size = (uint8_t)(11 + packet_size);

extern const int max_focus_steps = 40575;
extern const int max_zoom_steps = 4628;

const uint16_t max_offset = 50000;
const uint16_t max_length = 50000;


//extern const int min_pw = 1000;
//extern const int max_pw = 30000;

//-----------------------------------------------------------------------------
typedef struct controller_info
{
    uint8_t serial_number;
    uint8_t firmware[2];

    controller_info() = default;

    controller_info(std::vector<uint8_t> data)
    {
        if ((data[0] == 1) && (data.size() >= 4))
        {
            serial_number = data[1];
            firmware[0] = data[2];
            firmware[1] = data[3];
        }
    }

} controller_info;

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

    trigger_info(uint8_t n_, uint8_t *data) : num(n_)
    {
        polarity = data[0];

        offset = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
        length = (data[5] << 24) | (data[6] << 16) | (data[7] << 8) | data[8];
    }

    inline friend std::ostream& operator<< (
        std::ostream& out,
        const trigger_info& item
        )
    {
        //out << "Trigger Information: " << std::endl;
        out << "  Number:         " << (uint32_t)item.num << std::endl;
        out << "  Polarity:       " << (uint32_t)item.polarity << std::endl;
        out << "  Offset (us):    " << (uint32_t)item.offset << std::endl;
        out << "  Length (us):    " << (uint32_t)item.length << std::endl;
        return out;
    }

} trigger_info;

//-----------------------------------------------------------------------------
class controller
{

public:

    controller_info ctrl_info;
    data_packet tx;
    data_packet rx;

    controller() = default;

    void set_driver_info(data_packet packet)
    {
        ctrl_info = controller_info(packet.data);
    }

    //-----------------------------------------------------------------------------
    bool send_packet(FT_HANDLE ctrl_handle, data_packet packet)
    {

        return send_data(ctrl_handle, packet.to_vector());

    }	// end of send_packet

    //-----------------------------------------------------------------------------
    bool receive_packet(FT_HANDLE ctrl_handle, uint32_t count, data_packet &packet)
    {

        std::vector<uint8_t> rx_data;

        bool status = receive_data(ctrl_handle, count, rx_data);
        
        if(status)
            packet = data_packet(rx_data);

        return status;

    }   // end of receive_packet

    //-----------------------------------------------------------------------------
    bool ping_motor(FT_HANDLE ctrl_handle, uint8_t id, motor_info &mi)
    {
        bool status = true;
        uint8_t mtr_error = 0;

        dynamixel_packet mtr_packet(id, DYN_PING);
        tx = data_packet(MOTOR_CTRL_PING, (uint8_t)mtr_packet.get_size(), mtr_packet.get_packet_array());
        send_packet(ctrl_handle, tx);
        status &= receive_packet(ctrl_handle, ping_sp_size, rx);

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
    bool enable_motor(FT_HANDLE ctrl_handle, uint8_t id, bool value)
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
        send_packet(ctrl_handle, tx);
        status &= receive_packet(ctrl_handle, write_sp_size, rx);
        status &= (rx.data[SP_ERROR_POS] == 0);

        if (status == false)
        {
            std::cout << "Error enabling motor id: " << (uint32_t)id << ".  Error: " << mtr_error_string[rx.data[SP_ERROR_POS]] << std::endl;
        }

        return status;

    }   // end of enable_motor

    //-----------------------------------------------------------------------------
    bool set_position(FT_HANDLE ctrl_handle, uint8_t id, int32_t &step)
    {
        bool status = true;

        dynamixel_packet mtr_packet(id, DYN_WRITE);

        // step the focus motor
        mtr_packet.add_params((uint16_t)ADD_GOAL_POSITION, (uint32_t)step);
        
        tx = data_packet(MOTOR_CTRL_WR, (uint8_t)mtr_packet.get_size(), mtr_packet.get_packet_array());
        send_packet(ctrl_handle, tx);
        status &= receive_packet(ctrl_handle, write_sp_size, rx);
        status &= (rx.data[SP_ERROR_POS] == 0);

        if (status == false)
        {
            std::cout << "Error setting position for motor id: " << (uint32_t)id << ".  Error: " << mtr_error_string[rx.data[SP_ERROR_POS]] << std::endl;
        }

        return status;

    }   // end of step_motor

    //-----------------------------------------------------------------------------
    bool get_position(FT_HANDLE ctrl_handle, uint8_t id, int32_t& step)
    {
        bool status = true;
        dynamixel_packet mtr_packet(id, DYN_READ);
        mtr_packet.add_params((uint16_t)ADD_PRESENT_POSITION, (uint16_t)4);

        tx = data_packet(MOTOR_CTRL_RD4, (uint8_t)mtr_packet.get_size(), mtr_packet.get_packet_array());
        send_packet(ctrl_handle, tx);
        status &= receive_packet(ctrl_handle, read_sp_size, rx);
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
    bool motor_moving(FT_HANDLE ctrl_handle, uint8_t id)
    {
        bool status = true;
        
        dynamixel_packet mtr_packet(id, DYN_READ);
        mtr_packet.add_params((uint16_t)ADD_MOVING, (uint16_t)1);

        tx = data_packet(MOTOR_CTRL_RD1, (uint8_t)mtr_packet.get_size(), mtr_packet.get_packet_array());

        send_packet(ctrl_handle, tx);
        status &= receive_packet(ctrl_handle, 14, rx);
        status &= (rx.data[SP_ERROR_POS] == 0);

        if (status == false)
        {
            std::cout << "Error getting move status for motor id: " << (uint32_t)id << ".  Error: " << mtr_error_string[rx.data[SP_ERROR_POS]] << std::endl;
        }

        return status;

    }   // end of motor_moving

    //-----------------------------------------------------------------------------
    bool reset_motor(FT_HANDLE ctrl_handle, uint8_t id)
    {
        bool status = true;

        dynamixel_packet mtr_packet(id, DYN_REBOOT);
        tx = data_packet(MOTOR_CTRL_RD4, (uint8_t)mtr_packet.get_size(), mtr_packet.get_packet_array());
        
        send_packet(ctrl_handle, tx);
        status &= receive_packet(ctrl_handle, read_sp_size, rx);
        status &= (rx.data[SP_ERROR_POS] == 0);

        if(status == false)
        {
            std::cout << "Error reseting motor id: " << (uint32_t)id << ".  Error: " << mtr_error_string[rx.data[SP_ERROR_POS]] << std::endl;
        }

        return status;

    }   // end of reset_motor

    //-----------------------------------------------------------------------------
    bool config_channel(FT_HANDLE ctrl_handle,
        uint8_t channel,
        std::string input
    )
    {
        bool status = false;
        std::vector<uint8_t> data(9);
        std::vector<std::string> params;

        try
        {
            parse_csv_line(input, params);

            // get the trigger line polarity
            data[0] = std::stoi(params[0]) & 0x01;

            // get the trigger offset
            uint32_t offset = min((uint32_t)(std::stoi(params[1]) & 0x0000FFFF), max_offset);
            data[1] = (offset >> 24) & 0xFF;
            data[2] = (offset >> 16) & 0xFF;
            data[3] = (offset >> 8) & 0xFF;
            data[4] = offset & 0xFF;

            // get the trigger length
            uint32_t length = min((uint32_t)(std::stoi(params[2]) & 0x0000FFFF), max_length);// +offset;
            data[5] = (length >> 24) & 0xFF;
            data[6] = (length >> 16) & 0xFF;
            data[7] = (length >> 8) & 0xFF;
            data[8] = length & 0xFF;

            tx = data_packet(channel, (uint8_t)data.size(), data);
            send_packet(ctrl_handle, tx);
            status = receive_packet(ctrl_handle, 3, rx);
        }
        catch (std::exception e)
        {
            std::cout << "Error getting configuration information: " << e.what() << std::endl;
        }

        return status;

    }   // end of config_channel

    //-----------------------------------------------------------------------------
    bool trigger(FT_HANDLE ctrl_handle, uint8_t channel)
    {
        bool status = true;

        tx = data_packet(channel);
        send_packet(ctrl_handle, tx);
        status = receive_packet(ctrl_handle, 3, rx);
        
        return status;
    }   // end of trigger


    bool get_trigger_info(FT_HANDLE ctrl_handle, trigger_info &t1_info, trigger_info &t2_info)
    {
        bool status = true;
        //trigger_info t1_info;
        //trigger_info t2_info;

        tx = data_packet(TRIG_CONFIG);

        // send the request packet and get response back
        send_packet(ctrl_handle, tx);
        status = receive_packet(ctrl_handle, 20, rx);

        if (status == true)
        {
            t1_info = trigger_info(1, rx.data.data());
            t2_info = trigger_info(2, rx.data.data() + 9);
        }

        return status;
    }


};   // end of class

//-----------------------------------------------------------------------------

inline std::ostream& operator<< (
    std::ostream& out,
    const controller& item
    )
{
    using std::endl;
    out << "Motor Controller Information: " << std::endl;
    out << "  Serial Number:    " << (uint32_t)item.ctrl_info.serial_number << std::endl;
    out << "  Firmware Version: " << (uint32_t)item.ctrl_info.firmware[0] << "." << std::setfill('0') << std::setw(2) << (uint32_t)item.ctrl_info.firmware[1] << std::endl;
    return out;
}


#endif  // MOTOR_DRIVER_CLASS_H
