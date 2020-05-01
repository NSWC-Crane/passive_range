#ifndef TRIGGER_CONTROL_CLASS_H
#define TRIGGER_CONTROL_CLASS_H

// C/C++ Includes
#include <cstdint>
#include <string>
#include <vector>

#include "ftd2xx_functions.h"
#include "data_packet.h"

const uint16_t max_offset = 25000;
const uint16_t max_length = 25000;

/***********************Define Header Command Responses************************/
#define TC_CONNECT         0x01                  /* Check for data connection to motor controller */
#define TC_FIRM_READ       0x02                  /* Read firmware version return command */
#define TC_SER_NUM_READ    0x03                  /* Read serial number return command */

// configure the trigger parameters
constexpr uint8_t CONFIG_T1 = 0x11;           /* Configure Trigger 1 parameters */
constexpr uint8_t CONFIG_T2 = 0x12;           /* Configure Trigger 2 parameters */
//constexpr uint8_t CONFIG_T3 = 0x13             /* Configure Trigger 3 parameters */

// initiate triggers
constexpr uint8_t TRIG_ALL = 0x20;            /* initiate trigger sequence */
constexpr uint8_t TRIG_CH1 = 0x21;            /* pulse channel 1 */
constexpr uint8_t TRIG_CH2 = 0x22;            /* pulse channel 2 */

//-----------------------------------------------------------------------------
typedef struct trigger_ctrl_info
{
    uint8_t serial_number = 0;
    uint8_t firmware[2] = { 0, 0 };

    trigger_ctrl_info() {}

    trigger_ctrl_info(std::vector<uint8_t> data)
    {
        if ((data[0] == 1) && (data.size() >= 4))
        {
            serial_number = data[1];
            firmware[0] = data[2];
            firmware[1] = data[3];
        }
    }

} trigger_ctrl_info;


//-----------------------------------------------------------------------------
class trigger_ctrl
{

public:

    trigger_ctrl_info info;
    data_packet tx;
    data_packet rx;

    trigger_ctrl() {}

    void set_trigger_info(data_packet packet)
    {
        info = trigger_ctrl_info(packet.data);
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
    
private:



};   // end of class

//-----------------------------------------------------------------------------
inline std::ostream& operator<< (
    std::ostream& out,
    const trigger_ctrl& item
    )
{
    using std::endl;
    out << "Trigger Controller Information: " << std::endl;
    out << "  Serial Number:    " << (uint32_t)item.info.serial_number << std::endl;
    out << "  Firmware Version: " << (uint32_t)item.info.firmware[0] << "." << std::setfill('0') << std::setw(2) << (uint32_t)item.info.firmware[1] << std::endl;
    return out;
}

//-----------------------------------------------------------------------------
bool config_channel(FT_HANDLE driver_handle, 
    std::string input,
    uint8_t channel,
    trigger_ctrl &tc
)
{
    bool status = false;
    std::vector<uint8_t> data(5);
    std::vector<std::string> params;

    try
    {
        parse_csv_line(input, params);

        data[0] = std::stoi(params[0]) & 0x01;

        uint16_t offset = min((uint16_t)(std::stoi(params[1]) & 0xFFFF), max_offset);
        data[1] = (offset >> 8) & 0xFF;
        data[2] = offset & 0xFF;

        uint16_t length = min((uint16_t)(std::stoi(params[2]) & 0xFFFF), max_length) + offset;
        data[3] = (length >> 8) & 0xFF;
        data[4] = length & 0xFF;

        tc.tx = data_packet(channel, (uint8_t)data.size(), data);
        tc.send_packet(driver_handle, tc.tx);
        status = tc.receive_packet(driver_handle, 3, tc.rx);
    }
    catch (std::exception e)
    {
        std::cout << "Error getting configuration information: " << e.what() << std::endl;
    }

    return status;

}   // end of config_channel

//-----------------------------------------------------------------------------
bool trigger(FT_HANDLE driver_handle,
    uint8_t channel,
    trigger_ctrl& tc
)
{
    tc.tx = data_packet(channel);
    tc.send_packet(driver_handle, tc.tx);
    return tc.receive_packet(driver_handle, 3, tc.rx);
}   // end of trigger


#endif  // TRIGGER_CONTROL_CLASS_H
