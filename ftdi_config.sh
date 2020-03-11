#!/bin/bash

if [ "$(id -u)" = "0" ]
then
    echo
    echo "This script will add the ftdi_sio drivers to the blacklist so that the FTDI D2XX";
    echo "drivers will work with the FTDI USB to serial port devices.";
    echo 
    echo
else
    echo
    echo "This script needs to be run as root.";
    echo "eg.";
    echo "sudo ftdi_config.sh";
    echo
    exit 0
fi

BlacklistFile="/etc/modprobe.d/blacklist_ftdi.conf";
echo
echo "Writing the blacklist file.";

# write the blacklist file
echo "blacklist ftdi_sio" 1>$BlacklistFile


# create a udev rule that allows a normal user to acces the usb
UDEV_FILE="/etc/udev/rules.d/99-ftdi.rules"
echo "SUBSYSTEMS==\"usb\", ATTRS{idVendor}==\"0403\", ATTRS{idProduct}==\"6001\", GROUP=\"users\", MODE=\"0666\"" > ${UDEV_FILE}


echo
echo "Configuration complete. A reboot may be required on some systems for changes to take effect";
echo

exit 0
