
# Bluemix Quick Start Example program 

This example program runs the Mbed OS (version 5.x) on the hardware listed in the 'Hardware used' section. The 
example program reads the Temprature and Humidity sensors from the sensors expansion board then posts that data
to the IBM Watson IoT Platform dashboard.  The sensor readings are sent every 6 seconds continuously.  Instructions 
needed for accessing the dashboard are presented to the user when the program starts.

## Hardware used
 - X_NUCLEO_L476RG mcu board
 - X_NUCLEO_IKS01A2 sensors expansion board
 - AVNET RSR1157 NbIOT BG96 expansion board

**NOTE: This program tested on BG96 Rev:BG96MAR02A04M1G**

## Building

The example program can be build from either the command line or the on-line compiler (located at https://os.mbed.com), 
instructions for both method are below.  

### Building from the Command Line
1. Import the BluemixQS_BG96 project: **mbed import http://github.com/Avnet/BluemixQS_BG96**

2. Goto the BluemixQS_BG96 folder, Then edit mbed_settings.py and add the path to your compiler using GCC_ARM_PATH
 
3. Build the program by executing the command **'mbed compile -m K64F -t GCC_ARM'**

4. The executable program will be located at **BUILD/NUCLEO_L476RG/GCC_ARM/BluemixQS_BG96.bin**

5. Copy the executable program the Nucleo board that is connected to your PC, and once copied, press the reset button.

### Building using the On-Line compiler

1.  Ensure the NUCLEO-L476RG platform is selected.

2.  Right click the mouse over the "My Programs" folder.  You will be presented with a drop-down box, select
    "Import Program", and then "From URL".

3.  The source URL is **mbed import http://github.com/Avnet/BluemixQS_BG96**, then select "Import"

4.  The program will be built and placed into your 'Download' folder.

5.  Copy the executable program from your Download folder to the Nucleo board connected to you PC.

## Proram Execution

You can verify operation of the Bluemix Quick Start application via a connected terminal program (Putty, minicom, hyperterm, coolterm or some other similar program).  The connection parameters are 115200-N81. The program execution should resemble:

```

     ****
    **  **     Bluemix Quick Start example, version 1.0
   **    **    Demo of the RSR1157 NbIOT BG96 expansion board
  ** ==== **   by AVNET
                   

This demonstration program posts Temp and Humidity data to the IBM
Quickstart web-site. 


------------------------------------------------------------------------
Local network info:
IP address:         10.192.164.106
MAC address:        37:67:61:79:08:72:30
Your <uniqueID> is: IoT-37676179-2016

To observe, go to 'https://quickstart.internetofthings.ibmcloud.com/'
and enter 'IoT-37676179-2016' as your device ID.  Data will then be displayed
as it is received. 
------------------------------------------------------------------------


(0) Attempting TCP connect to quickstart.messaging.internetofthings.ibmcloud.com:1883:  Success!
(0) Attempting MQTT connect to quickstart.messaging.internetofthings.ibmcloud.com:1883: Success!
Loop #1
Publishing MQTT message '{"d" : {"humd" : "47.0" }}'
Publishing MQTT message '{"d" : {"temp" : " 55.9" }}'
Loop #2
Publishing MQTT message '{"d" : {"humd" : "46.9" }}'
Publishing MQTT message '{"d" : {"temp" : " 55.8" }}'
Loop #3
Publishing MQTT message '{"d" : {"humd" : "47.0" }}'
Publishing MQTT message '{"d" : {"temp" : " 55.8" }}'


```


**NOTE: elements such as IP, MAC, and UniqueID will be different from that shown above.

