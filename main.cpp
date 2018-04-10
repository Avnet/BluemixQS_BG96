/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: Apache-2.0
*/

/* =====================================================================
   Copyright © 2018, Avnet (R)

   Contributors:
     * James M Flynn, www.em.avnet.com 
 
   Licensed under the Apache License, Version 2.0 (the "License"); 
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, 
   software distributed under the License is distributed on an 
   "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
   either express or implied. See the License for the specific 
   language governing permissions and limitations under the License.

    @file          main.cpp
    @version       1.0
    @date          April 1, 2018

======================================================================== */

#include <cctype>
#include <string>

#include "mbed.h"

#include "BG96Interface.h"
#include "XNucleoIKS01A2.h"

#include "MQTTClient.h"
#include "MQTTFormat.h"
#include "MQTT.h"

// connect options for MQTT broker
#define CLIENT            "quickstart"
#define URL               CLIENT ".messaging.internetofthings.ibmcloud.com"
#define CONFIGTYPE        "d:" CLIENT ":MicroZed:%s"

#define PORT              1883                           // MQTT broker port number
#define PUBLISH_TOPIC     "iot-2/evt/status/fmt/json"    // MQTT topic

#define APP_VERSION       "1.0"
#define CTOF(x)           ((x)*1.8+32)

/* initialize the expansion board */
static XNucleoIKS01A2 *mems_expansion_board = XNucleoIKS01A2::instance(D14, D15, D4, D5);

/* initialize the expansion board sensors */
static HTS221Sensor  *hum_temp   = mems_expansion_board->ht_sensor;

int main() {
    const char* topic    = PUBLISH_TOPIC;
    const char* hostname = URL;
    char        clientID[100], buf[100];
    string      st, uniqueID;
    int         loopCnt, rc, good=0, port=PORT;
    float       temp, hum;

    printf("\r\n");
    printf("     ****\r\n");
    printf("    **  **     Bluemix Quick Start example, version %s\r\n", APP_VERSION);
    printf("   **    **    Demo of the RSR1157 NbIOT BG96 expansion board\r\n");
    printf("  ** ==== **   by AVNET\r\n");
    printf("                   \r\n");
    printf("\r\n");
    printf("This demonstration program posts Temp and Humidity data to the IBM\r\n");
    printf("Quickstart web-site. \n\n");
    printf("\r\n");

    MQTT_USE(BG96Interface);
    MQTTct   net;
    MQTTnet& eth = net.getEth();

    MQTT::Client<MQTTct, Countdown> client = MQTT::Client<MQTTct, Countdown>(net);
    
    st = eth.get_mac_address();
    uniqueID="IoT-";
    uniqueID += st[0];
    uniqueID += st[1];
    uniqueID += st[3];
    uniqueID += st[4];
    uniqueID += st[6];
    uniqueID += st[7];
    uniqueID += st[9];
    uniqueID += st[10];
    uniqueID += "-2018";
    
    sprintf(clientID, CONFIGTYPE, uniqueID.c_str());

    printf("------------------------------------------------------------------------\r\n");
    printf("Firmware Revision:   %s\r\n\r\n",FIRMWARE_REV(&eth));
    printf("Network info:\r\n");    
    printf("IP address:          %s\r\n", eth.get_ip_address());
    printf("MAC address:         %s\r\n", eth.get_mac_address());
    printf("Your <uniqueID> is:  %s\r\n", uniqueID.c_str());
    printf("\r\nTo observe, go to 'https://quickstart.internetofthings.ibmcloud.com/'\r\n");
    printf("and enter '%s' as your device ID.  Data will then be displayed\r\n",uniqueID.c_str());
    printf("as it is received. \r\n");
    printf("------------------------------------------------------------------------\r\n");

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       

    while( !good ) {
        rc = 1;
        loopCnt=0;
        while( rc && loopCnt < 3) {   // connect to TCP socket and check return code
            printf("\r\n\r\n(%d) Attempting TCP connect to %s:%d:  ", loopCnt++, hostname, port);
            rc = net.connect((char*)hostname, port);
            if( rc ) {
                printf("Failed (%d)!\r\n",rc);
                wait(5);
                }
            else{
                printf("Success!\r\n");
                rc = 0;
                }
            }

        if( loopCnt == 3 ) {
            printf("Failed to connect to '%s'\n",hostname);
            abort();
            }
        
        data.willFlag         = 0;  
        data.MQTTVersion      = 3;
        data.clientID.cstring = clientID;
        data.keepAliveInterval= 10;
        data.cleansession     = 1;
    

        rc = 1;

        loopCnt = 0;
        while( !client.isConnected() && rc && loopCnt < 3) {
            printf("(%d) Attempting MQTT connect to %s:%d: ", loopCnt++, hostname, port);
            rc = client.connect(data);
            if( rc ) {
                printf("Failed (%d)!\r\n",rc);
                wait(5);
                }
            else
                printf("Success!\n\n");

            }
        if( loopCnt == 3 ) {
            printf("Failed to connect to MQTT service\n");
            abort();
            }
        

        good = 1;
        }
    
    MQTT::Message message;
    message.qos      = MQTT::QOS0;
    message.retained = false;
    message.dup      = false;
    message.payload  = (void*)buf;
    
    hum_temp->enable();

    loopCnt=0;

    while(true) {
 	      hum_temp->get_temperature(&temp);
        hum_temp->get_humidity(&hum);


        printf("Loop #%d\n", ++loopCnt);
        memset(buf,0x00,sizeof(buf));
        sprintf(buf, "{\"d\" : {\"humd\" : \"%2.1f\" }}", hum);
        printf("Publishing MQTT message '%s'\n", (char*)message.payload);
        message.payloadlen = strlen(buf);
        rc = client.publish(topic, message);
        if( rc ) 
            printf("Publish request failed! (%d)\r\n",rc);

        memset(buf,0x00,sizeof(buf));
        sprintf(buf, "{\"d\" : {\"temp\" : \"%5.1f\" }}", CTOF(temp));
        printf("Publishing MQTT message '%s'\n", (char*)message.payload);
        message.payloadlen = strlen(buf);
        rc = client.publish(topic, message);
        if( rc ) 
            printf("Publish request failed! (%d)\r\n",rc);

        client.yield(6000);
        }
}
