
#include <string>

#include "mbed.h"
#include "HTS221.h"
#include "EthernetInterface.h"
#include "WNC14A2AInterface.h"

#include "MQTTClient.h"
#include "MQTTFormat.h"
#include "MQTT.h"

#define CTOF(x)  ((x)*1.8+32)

typedef enum color {off, red, green, blue} color_t;

DigitalOut redLED(LED_RED);
DigitalOut greenLED(LED_GREEN);
DigitalOut blueLED(LED_BLUE);
InterruptIn switch2(SW2);
InterruptIn switch3(SW3);


// connect options for MQTT broker
#define CLIENT      "quickstart"
#define URL         CLIENT ".messaging.internetofthings.ibmcloud.com"
#define CONFIGTYPE  "d:" CLIENT ":MicroZed:%s"

#define PORT        1883                           // MQTT broker port number
#define CLIENTID    "96430312d8f7"                 // use MAC address without colons
#define USERNAME ""                                    // not required for demo app
#define PASSWORD ""                                    // not required for demo app
#define PUBLISH_TOPIC "iot-2/evt/status/fmt/json"              // MQTT topic
#define SUBSCRIBTOPIC "iot-2/cmd/+/fmt/+"

#define HTS221_STR  "  Temp  is: %0.2f F \n\r  Humid is: %02d %%\n\r"

Queue<uint32_t, 6> messageQ;

struct rcvd_errs{
    int err;
    const char *er;
    };
    
rcvd_errs response[] = {
    200, "200 OK - Request has succeeded.",
    201, "201 Created - Request has been fulfilled and a new resource created.",
    202, "202 Accepted - The request has been accepted for processing, but the processing will be completed asynchronously",
    204, "204 No Content - The server has fulfilled the request but does not need to return an entity-body.",
    400, "400 Bad Request - Bad request (e.g. sending an array when the API is expecting a hash.)",
    401, "401 Unauthorized - No valid API key provided.",
    403, "403 Forbidden - You tried to access a disabled device, or your API key is not allowed to access that resource, etc.",
    404, "404 Not Found - The requested item could not be found.",
    405, "405 Method Not Allowed - The HTTP method specified is not allowed.",
    415, "415 Unsupported Media Type - The requested media type is currently not supported.",
    422, "422 Unprocessable Entity - Can result from sending invalid fields.",
    429, "429 Too Many Requests - The user has sent too many requests in a given period of time.",
    500, "500 Server errors - Something went wrong in the M2X server",
    502, "502 Server errors - Something went wrong in the M2X server",
    503, "503 Server errors - Something went wrong in the M2X server",
    504, "504 Server errors - Something went wrong in the M2X server",
    };
#define RCMAX   sizeof(response)/sizeof(rcvd_errs)

const char * response_str(int rc) {
    static const char *unkown = "Unknown error code...";
    unsigned int i=0;
    while( response[i].err != rc && i < RCMAX)
        i++;
    return ((i<RCMAX)? response[i].er : unkown);
}

// LED color control function
void controlLED(color_t led_color) {
    switch(led_color) {
        case red :
            greenLED = blueLED = 1;          
            redLED = 0.7;
            break;
        case green :
            redLED = blueLED = 1;
            greenLED = 0.7;
            break;
        case blue :
            redLED = greenLED = 1;
            blueLED = 0.7;
            break;
        case off :
            redLED = greenLED = blueLED = 1;
            break;
    }
}
    
// Switch 2 interrupt handler
void sw2_ISR(void) {
    messageQ.put((uint32_t*)22);
}

// Switch3 interrupt handler
void sw3_ISR(void) {
    messageQ.put((uint32_t*)33);
}
 
// MQTT message arrived callback function
void messageArrived(MQTT::MessageData& md) {
    MQTT::Message &message = md.message;
    printf("Receiving MQTT message:  %.*s\r\n", message.payloadlen, (char*)message.payload);
    
    if (message.payloadlen == 3) {
        if (strncmp((char*)message.payload, "red", 3) == 0)
            controlLED(red);
        
        else if(strncmp((char*)message.payload, "grn", 3) == 0)
            controlLED(green);
        
        else if(strncmp((char*)message.payload, "blu", 3) == 0)
            controlLED(blue);
        
        else if(strncmp((char*)message.payload, "off", 3) == 0)
            controlLED(off);
    }        
}

int main() {
    int rc, txSel=0, good = 0;
    const char* topic = PUBLISH_TOPIC;
    char clientID[100], buf[100];
    string st, uniqueID;

    HTS221 hts221;

    printf("\r\n");
    printf("     ** **\r\n");
    printf("    **   **     Bluemix Quick Start example\r\n");
    printf("   **     **    using the AT&T IoT Starter Kit\r\n");
    printf("  ** ===== **   by AVNET\r\n");
    printf("                   \r\n");
    printf("\r\n");
    printf("This demonstration program posts Temp and Humidity data to the IBM\r\n");
    printf("Quickstart web-site. On the FRDM-K64F board:\n\n");
    printf("  -> SW2 toggles between Temp and Humidity\n");
    printf("  -> SW3 toggles enabled sending both Temp & Humity\n");
    printf("\r\n");

    rc = hts221.init();
    if ( rc  ) {
        printf("HTS221 Detected (0x%02X)\n\r",rc);
        printf(HTS221_STR, CTOF(hts221.readTemperature()), hts221.readHumidity()/10);
    }
    else {
        printf("HTS221 NOT DETECTED!\n\r");
    }

    // turn on LED  
    controlLED(red);
    
    // set SW2 and SW3 to generate interrupt on falling edge 
    switch2.fall(&sw2_ISR);
    switch3.fall(&sw3_ISR);
    

    // initialize ethernet interface and construct the MQTT client
#if   MBED_CONF_APP_MQTT_INTERFACE == MQTT_BG96
    printf("Connecting using the BG96\n\n");
#elif MBED_CONF_APP_MQTT_INTERFACE == MQTT_WNC14A2A
    printf("Connecting using the WNC14A2A\n\n");
#elif MBED_CONF_APP_MQTT_INTERFACE == MQTT_ETHERNET
    printf("Connecting using the ETHERNET\n\n");
#endif

    MQTTct net;
    MQTTnet& eth = net.getEth();
    MQTT::Client<MQTTct, Countdown> client = MQTT::Client<MQTTct, Countdown>(net);
    
    const char* hostname = URL;
    int port = PORT;
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
    uniqueID += "-2016";
    
    sprintf(clientID, CONFIGTYPE, uniqueID.c_str());

    printf("------------------------------------------------------------------------\r\n");
    printf("Local network info:\r\n");    
    printf("IP address:         %s\r\n", eth.get_ip_address());
    printf("MAC address:        %s\r\n", eth.get_mac_address());
    printf("Gateway address:    %s\r\n", eth.get_gateway());
    printf("Your <uniqueID> is: %s\r\n", uniqueID.c_str());
    printf("\r\nTo observe, go to 'https://quickstart.internetofthings.ibmcloud.com/'\r\n");
    printf("and enter '%s' as your device ID.  Data will then be displayed\r\n",uniqueID.c_str());
    printf("as it is received. \r\n");
    printf("------------------------------------------------------------------------\r\n");

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       

    int tries;
    
    while( !good ) {
        tries=0;
        rc = 1;
        while( rc && tries < 3) {   // connect to TCP socket and check return code
            printf("\r\n\r\n(%d) Attempting TCP connect to %s:%d:  ", tries++, hostname, port);
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
        if( tries < 3 )
            tries = 0;
        
        data.willFlag = 0;  
        data.MQTTVersion = 3;

        data.clientID.cstring = clientID;
        data.keepAliveInterval = 10;
        data.cleansession = 1;
    
        rc = 1;
        while( !client.isConnected() && rc && tries < 3) {
            printf("(%d) Attempting MQTT connect to %s:%d: ", tries++, hostname, port);
            rc = client.connect(data);
            if( rc ) {
                printf("Failed (%d)!\r\n",rc);
                wait(5);
                }
            else
                printf("Success!\r\n");
            }

        if( tries < 3 )
            tries = 0;
        good = 1;
        }
    
    MQTT::Message message;
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    
    while(true) {
        osEvent switchEvent = messageQ.get(100);
      
        if( switchEvent.status == osEventMessage && switchEvent.value.v == 22 ) { //switch between sending humidity & temp
            controlLED(green);
            txSel = (txSel&1)? txSel&2 : txSel|1;
            printf("Publishing %s messages.\n", (txSel&1)? "Humid":"Temp");
            }
          
        if( switchEvent.status == osEventMessage && switchEvent.value.v == 33) {  //user wants to run in Quickstart of Demo mode
            controlLED(blue);
            txSel = (txSel&2)? txSel&1 : txSel|2;
            if( txSel & 2 )
                printf("Publishing both readings.\n");
            }
                    
        if( txSel & 0x03 ) {
            memset(buf,0x00,sizeof(buf));
            rc = hts221.readHumidity();
            sprintf(buf, "{\"d\" : {\"humd\" : \"%2d.%d\" }}", rc/10, rc-((rc/10)*10));
            printf("Publishing MQTT message '%s'\n", (char*)message.payload);
            message.payloadlen = strlen(buf);
            rc = client.publish(topic, message);
            if( rc ) 
                printf("Publish request failed! (%d)\r\n",rc);
            }
        if( txSel & 0x02 || !txSel ) {
            memset(buf,0x00,sizeof(buf));
            sprintf(buf, "{\"d\" : {\"temp\" : \"%5.1f\" }}", CTOF(hts221.readTemperature()));
            printf("Publishing MQTT message '%s'\n", (char*)message.payload);
            message.payloadlen = strlen(buf);
            rc = client.publish(topic, message);
            if( rc ) 
                printf("Publish request failed! (%d)\r\n",rc);
            }
        client.yield(6000);
    }
}
