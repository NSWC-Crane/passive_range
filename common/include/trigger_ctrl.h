#ifndef TRIGGER_CONTROL_CLASS_H
#define TRIGGER_CONTROL_CLASS_H

// C/C++ Includes
#include <cstdint>
#include <string>
#include <vector>

#include "ftd2xx_functions.h"
#include "data_packet.h"


/***********************Define Header Command Responses************************/
#define CONNECT         0x01                  /* Check for data connection to motor controller */
#define FIRM_READ       0x02                  /* Read firmware version return command */
#define SER_NUM_READ    0x03                  /* Read serial number return command */

// configure the trigger parameters
#define CONFIG_T1       0x11                  /* Configure Trigger 1 parameters */
#define CONFIG_T2       0x12                  /* Configure Trigger 2 parameters */
#define CONFIG_T3       0x13                  /* Configure Trigger 3 parameters */

// initiate triggers
#define TRIG_INIT       0x20                  /* initiate trigger sequence */
#define TRIG_CH1        0x21                  /* pulse channel 1 */ 
#define TRIG_CH2        0x22                  /* pulse channel 2 */

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


#endif  // TRIGGER_CONTROL_CLASS_H
