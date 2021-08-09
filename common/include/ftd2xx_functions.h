#ifndef _FTDI_FUNCTIONS_H
#define _FTDI_FUNCTIONS_H

#include <cstdint>
#include <string>
#include <iostream>
#include <iomanip>
#include <vector>

#include "ftd2xx.h"

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)

#elif defined(__linux__)

#endif

//-----------------------------------------------------------------------------
//structure storage for FTDI device details
typedef struct ftdiDeviceDetails 
{
    int32_t device_number = 0;
    uint32_t type = 0;
    uint32_t baud_rate = 0;
    std::string description;
    std::string serial_number;
    uint8_t bits = FT_BITS_8;
    uint8_t stop_bits = FT_STOP_BITS_2;
    uint8_t parity = FT_PARITY_NONE;

    ftdiDeviceDetails() = default;

} ftdiDeviceDetails;


//-----------------------------------------------------------------------------
inline uint32_t get_device_list(std::vector<ftdiDeviceDetails> &device)
{
    FT_HANDLE ftHandleTemp; 
    FT_DEVICE_LIST_INFO_NODE dev_info[32];
    uint32_t dev_number = 0, dev_found = 0;
    DWORD num_devices = 0;
    DWORD flags, type, id, loc_id;
    char serial_number[16] = { 0 };
    char description[64] = { 0 };
    ftdiDeviceDetails tmp_dev;
    
    // search for devices connected to the USB port
    FT_CreateDeviceInfoList(&num_devices);

    device.clear();

    if (num_devices > 0)
    {
        if (FT_GetDeviceInfoList(dev_info, &num_devices) == FT_OK)
        {

            for(uint32_t idx=0; idx<num_devices; ++idx)
            {
                if (FT_GetDeviceInfoDetail(idx, &flags, &type, &id, &loc_id, serial_number, description, &ftHandleTemp) == FT_OK)
                {
                    tmp_dev.device_number = idx;
                    tmp_dev.type = type;
                    tmp_dev.description = std::string(description);
                    tmp_dev.serial_number = std::string(serial_number);

                    device.push_back(tmp_dev);
                }
                else
                {
                    std::cout << "Error getting the FTDI device info!" << std::endl;
                }   

            }
        }
    }

    return num_devices;

}   // end of get_device_list


// ----------------------------------------------------------------------------------------
inline FT_HANDLE open_com_port(ftdiDeviceDetails &device, uint32_t read_timeout=10000, uint32_t write_timeout=1000)
{
    FT_HANDLE ftHandle = NULL;
    LONG comm_port_num;
    FT_STATUS status;
    uint8_t latency_timer = 2;
    
    status = FT_Open(device.device_number, &ftHandle);
    
    if (status == FT_OK)
    {

        if (FT_SetBaudRate(ftHandle, device.baud_rate) != FT_OK)
        {
            std::cout << "Error setting baud rate!" << std::endl;
            return ftHandle;
        }

        status = FT_SetDataCharacteristics(ftHandle, device.bits, device.stop_bits, device.parity);
        if(status != FT_OK)
        {
            std::cout << "Error setting FT_SetDataCharacteristics: FT_STATUS = " << status << std::endl;
        }
        
        status = FT_SetTimeouts(ftHandle, read_timeout, write_timeout);
        if(status != FT_OK)
        {
            std::cout << "Error setting FT_SetTimeouts: FT_STATUS = " << status << std::endl;
        }
        
        status = FT_SetLatencyTimer(ftHandle, latency_timer);
        if(status != FT_OK)
        {
            std::cout << "Error setting FT_SetLatencyTimer: FT_STATUS = " << status << std::endl;
        }
        
#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)
        status = FT_GetComPortNumber(ftHandle, &comm_port_num);
        if (status == FT_OK)
        {
            if (comm_port_num == -1) // No COM port assigned
            {
                std::cout << "The device selected does not have a Comm Port assigned!" << std::endl << std::endl;
            }
            else
            {
                std::cout << "FTDI device " << device.description << " found on COM:" << std::setw(2) << std::setfill('0') << comm_port_num << std::endl << std::endl;
            }
        }
        else
        {
            std::cout << "Error getting FT_GetComPortNumber: FT_STATUS = " << status << std::endl;
        }
#endif

    }
    else
    {
        std::cout << "Error opening device: FT_STATUS = " << status << std::endl;
    }

    return ftHandle;
}	// end of open_com_port


//-----------------------------------------------------------------------------
inline FT_STATUS close_com_port(FT_HANDLE ftHandle)
{
    FT_STATUS status = FT_Close(ftHandle);
    return status;
}	// end of close_com_port


//-----------------------------------------------------------------------------
inline bool send_data(FT_HANDLE driver, std::vector<uint8_t> data)
{
    bool status = true;
    DWORD bytes_written;
    unsigned long ft_status;

    ft_status = FT_Write(driver, data.data(), (DWORD)data.size(), &bytes_written);
    if ((ft_status != FT_OK) || (bytes_written < (DWORD)data.size()))
    {
        status = false;
    }

    return status;

}	// end of send_packet

//-----------------------------------------------------------------------------
inline bool receive_data(FT_HANDLE driver, uint8_t& rx_data)
{
    bool status = true;
    DWORD read_count = 0;
    unsigned long ft_status;

    ft_status = FT_Read(driver, &rx_data, 1, &read_count);

    if (read_count < 1)
    {
        std::cout << "No data received from FTDI device!" << std::endl;
        status = false;
    }

    return status;

}   // end of receive_data

//-----------------------------------------------------------------------------
inline bool receive_data(FT_HANDLE driver, uint32_t count, std::vector<uint8_t> &rx_data)
{
    bool status = true;
    DWORD read_count = 0;
    unsigned long ft_status;

    rx_data.clear();
    rx_data.resize(count);

    ft_status = FT_Read(driver, &rx_data[0], count, &read_count);

    if(read_count < count)
    {
        std::cout << "No data received from FTDI device!" << std::endl;
        status = false;
    }

    return status;

}   // end of receive_data

// ----------------------------------------------------------------------------------------
inline bool flush_port(FT_HANDLE driver)
{
    bool status = true;
    unsigned long ft_status;

    ft_status = FT_Purge(driver, FT_PURGE_RX | FT_PURGE_TX);

    if (ft_status != FT_OK)
    {
        status = false;
    }

    return status;
}   // end of flush_port

// ----------------------------------------------------------------------------------------
inline std::ostream& operator<< (
    std::ostream& out,
    const ftdiDeviceDetails& item
    )
{
    using std::endl;
    out << "FTDI Device [" << item.device_number <<  "]: ";
    out << item.description;
    out << " (" << item.serial_number << ")";
    //out << " Baud Rate: " << item.baud_rate;
    out << std::endl;
    return out;
}

// ----------------------------------------------------------------------------------------	

#endif  // _FTDI_FUNCTIONS_H

