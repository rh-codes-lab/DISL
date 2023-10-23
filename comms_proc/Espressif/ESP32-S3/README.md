ESP32 FPGA
=============

This is a project intended to provide the ESP32 component of an FPGA JTAG interface. 


Hardware
--------

```bash
 ---------------------     JTAG     --------      USB    ---------   
| Arty|CMOD A7-35T TAP |  <---->   | FT2232H |   <--->  | ESP32S3 |  
 ---------------------              --------             ---------  
```
 
Wiring
------


The wiring tested for this is:

|ESP32S3 GPIO| Peripheral Pin|
|------------|---------------|
|6           |SDIO CLK       |
|7           |SDIO MOSI      |
|4           |SDIO MISO      |
|5           |SDIO CS        |
|8           |OLED I2C SCL   |
|9           |OLED I2C SDA   |

The code is developed and tested so far with the ESP32 ESP-IDF v5.1.1 and ESP32S3 devkit.  The board can be
powered via the USB connection which can be used for the UART and therefore the console.  

The FPGA board requires a power source greater than the USBH port of the ESP32S3 provides. Therefore, a Y splitter USB cable is necessary wherein the power portion of the Y is connected to a powered USB hub, PC, etc. while the data portion of the Y is connected to the USBH port of the ESP32S3. The base of the splitter is then plugged into the USB port of the FPGA board. 
 
Functionality
-------------

Functionality implemented so far:

1.  If the device has not been connected before
    1. Scan the network for available access points
    2. Bring up a HTML server which allows to select a network and provide a password, if necessary
2.  If the device was connected before retrieve the previous connection data from NVS
3.  Connect to the selected network
4.  If not successful, start again in 1.i
5.  Store the connection information in NVS
6.  Look for mDNS service `_mqtt._tcp` and try to connect to server at the specified port
7.  Periodically send an update to the topic /BOARDNAME/heartbeat which is a JSON string
8.  Listen to the topic /BOARDNAME/command. The expected data is a JSON string with a command key-value pair
9.  Establish external SD card filesystem
10. Establish OLED display of ssid/pw when in initial AP mode and heartbeat info when connected to MQTT
11. Establish USB Host interface for connecting FPGA JTAG

`BOARDNAME` has the form `ESP32-XXXXXXXXXXXX` where `XXXXXXXXXXXX` is the hexadecimal representation of the MAC
address of the board.


Requirement
-----------

A WiFi network is needed.  Only WPA2 networks have been tested so far.

On the network a mDNS service `_mqtt._tcp` has to be announced.  On Fedora systems use

     avahi-publish -s HOSTNAME _mqtt._tcp 1883

where `HOSTNAME` is the name of the machin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e with the MQTT server.  Adjust the port number,
if necessary.  This could be done permanently (see below).

On the specified host, run a MQTT broker.  On Fedora systems install the `mosquitto` package and then just

     /usr/sbin/mosquitto

Make sure the firewall allows outside connections to the MQTT (1883) port.

Make sure your mosquitto.conf has:

```
listener 1883
allow_anonymous true
```

in it, otherwise mDNS will tell your ESP32 a hostname for a MQTT broker that doesn't permit outside anonymous connections.

To monitor the topics run on the same or a different machine `mosquitto_sub`:

     mosquitto_sub -h HOSTNAME -t '#'

Replace `HOSTNAME` with the name of the machine running the MQTT broker.


### Use

When using Firefox to connect to the device to select a WiFi network it is necessary
(as of version 83) to enable an option.  In the browser, open a tab and use the URL

    about:config

Then in the search box type `dialog`.  This will show a number of entry, among them one
named

    dom.dialog_element.enabled

This variable needs to be set to `true`.  If this is not already the case double-click
on the `false` in this row to change the value.


### Automatic mDNS

Instead of starting mosquitto directly and separately announcing the service one can install the
`mosquitto.service` file as `/etc/avahi/services/mosquitto.service`.  With this avahi takes care of
announcing the service.


### Automatic MQTT

The mosquitto broker can also be started as a service:

     # systemctl start mosquitto

To enable the availability across the next reboot the service can be enabled:

     # systemctl enable mosquitto


Preparation
-----------

The toolchain and basic runtime (RTOS and some drivers) come from the ESP-IDF which can be installed from its github
repository:

     $ cd "$DEVELHOME"
     $ git clone -b v4.4.4 --recursive https://github.com/espressif/esp-idf.git
     $ cd esp-idf
     $ ./install.sh <TARGET>

The `DEVELHOME` environment variable is used here just for visualization.

If support for more than the base version if ESP32 is wanted replace the last line with

     $ ./install.sh esp32,esp32s2,esp32s3,esp32c3

or a reduced version. For our purposes the esp32s3 is our target so at least the following is needed:

     $ ./install.sh esp32s3
     

Before every development session (and the subsequent steps) the environment of the shell session needs to be initialized
for the IDF.

     $ . "$DEVELHOME/esp-idf/export.sh"

The SDK needs configuration as well.  The board settings need to match the available hardware.
The board is connected through USB.  The device for the communication needs to be specified.  It is
usually something like `/dev/ttyUSB1` or so.


To support secure HTTPS connections when accessing the device as an access
point a CA certification and key needs to be created.  This is not done
automatically to allow choosing the files.  With the `gen-ca.sh` script the
`ca` directory can be filled with the appropriate files.


Building
--------

When building the project for the first time or after cleaning up everything it's a good idea to set the target to esp32s3:

     $ idf.py set-target esp32s3

In addition, the configuration needs to be created if anything outside of what's in sdkconfig.defaults is needed.

     $ idf.py menuconfig

Note: If FreeRTOS timer stack errors occur, Increase Components --> FreeRTOS --> timer stack size to 4096 within the menuconfig step.

The appropriate configurations can be selected.  There is also a toplevel menu `ESP32 Connect` which allows
to specify the system-specific options.  

Running this command also creates the `build` subdirectory which is where the project should be built.

     $ idf.py build
     $ idf.py -p PORT_NAME flash monitor

This will build the binary, flash it to the device, and then start the serial console.

First Run
---------

To access services on the local network or the wider Internet the decice
needs to get access to the local WiFi network.  To safely enable the
connection the SSID and password of the WiFi network are **not** stored in
the source code or somehow else added to the binary.

Instead, if the program on the ESP32 cannot connect to a WiFi network
(either because it has never done that or because a previously used
network is not available anymore) it will create a WiFi access point (AP).

The SSID of the AP is `ESP32-XXXXXXXXXXXX` where `XXXXXXXXXXXX` is the MAC address
of the boards.  The password is randomly generated.  The details are shown on
a display (and in debug mode, on the serial console).

Connect to the IP address `192.168.4.1`, which is the ESP32 board access point, with
HTTPS.  Please keep in mind the note about the use of Firefox above.
The page that is displayed allows to select the WiFi network to use during
operation.  Selecting it will bring up a dialog to enter the password.

After successfully filling in the information the access point will stop
and the ESP32 device will try to connect to the selected WiFi network.
if successful it will connect to the advertizes MQTT service.

If the WiFi connection fails the AP will be brought up again and the
information can be entered anew.

Supported Peripherals
---------------------
- Adafruit OLED I2C SSD1306
- SD card over SPI
- Digilent ARTY or CMOD A7-35T board via USB and UART

Project Options
---------------

Running `idf.py menuconfig` from within the project's base directory brings up an interface for project configuration options. In the ESP Smartbadge options menu there are submenus for the peripherals listed above. GPIOs need to be assigned based on how your ESP32 board is wired up to your peripherals. 

MQTT Commands
-------------

Once a connection to the MQTT broker is established, commands can be sent over the topic /BOARDNAME/command. 
Responses are currently limited to the `idf.py monitor` output, however, these will be moved to MQTT once
the IoT application is fleshed out. 

The following commands are accepted, depending on project configuration options:

- {"command":"GetVersion"}
- {"command":"GetFileFromURL","url":"<URL OF FILE TO DOWNLOAD>","filename":"<LOCAL FILENAME>"}
- {"command":"ListSDCardFiles"}
- {"command":"RemoveFile","filename":"<LOCAL FILENAME>"}
- {"command":"JTAGProgramFPGA","filename":"<LOCAL FILENAME>"}
- {"command":"FlashSoftcore","filename":"<LOCAL FILENAME>"}
- {"command":"DisplayClear"}
- {"command":"DisplayHeartbeat","setting":True|False}
- {"command":"DisplayString","value":"<STRING TO DISPLAY>"}

The ESP32 has an SD card attached via its SPI bus. The mount point for its filesystem is /sdcard (or as defined in `idf.py menuconfig`) hence  in the commands above <LOCAL FILENAME> is expected to begin with the mount point e.g. /sdcard/MYFILE.BIN


Commands can be sent as follows:

`mosquitto_pub -h HOSTNAME -t '<TOPIC>' -m '<MESSAGE>'`

`HOSTNAME` is the name of the MQTT broker

`<TOPIC>` is of the form `/ESP32-<UUID>/command`

`<MESSAGE>` is per the above commands

Controller GUI
--------------

In the `controller_ui` directory is a Python tkinter script for simplifying sending commands to the ESP32 and getting status information.

Additional UI Tooling
------------------

Programs such as i[MQTT Explorer](http://mqtt-explorer.com) can be used as an alternative GUI for sending/receiving commands. 


