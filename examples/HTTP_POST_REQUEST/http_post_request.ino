/* ======================================== */
/* WIFI */
/* ======================================== */
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char WIFI_SSID[] = "AHTLAB";
const char WIFI_PASS[] = "ahtlab.com";

WiFiClient client;
IPAddress dns(8,8,8,8);

/* ======================================== */
/* SIM800 */
/* ======================================== */
#include <aht_sim800.h>
#define MODEM_RX 16
#define MODEM_TX 17
#define uart Serial2

AHT_GSM *gsmMaster = new AHT_GSM(&uart);
AHT_GSM *gsm;
GSM_TYPE gsmType;

bool gsmOk = false;

// INET
#define APN             "internet.wind"
#define USER            ""
#define PASS            ""
#define HTTP            "https://"
#define HOST            "5ea8480235f3720016608d00.mockapi.io"
#define PATH            "/api/test"
#define RESPONSE_LEN        128
char response[RESPONSE_LEN];

// Flags
bool gsmNet = false;

void setup() 
{
    Serial.begin(115200);
    delay(1000);

    if(gsmMaster->begin()) 
    {
        gsmType = gsmMaster->detectGSM(&uart);
        unsigned long baudrate = gsmMaster->getBaudrate();
        free(gsmMaster);
        
        if(gsmType == SIM800) 
        {
            gsm = new AHT_SIM800(&uart);
            gsm->begin(baudrate);
            gsmOk = true;
        }
        gsmNet = gsm->attackGPRS("internet.wind", "", "");

        if (GSM_PostRequest(HOST, PATH, "ping=1&pong=1"))
        {
            Serial.println(F("---RESPONSE---"));
            Serial.println(response);
            Serial.println(F("--------------"));
        }
    }

    WiFi_Init();
}

void loop() 
{
    if(!gsmOk)
    {
        Serial.println("can't detect gsm module");
        delay(5000);
        return;
    }

    if(uart.available())
    {
        Serial.write(uart.read());
    }
    
    if(Serial.available())
    {
        char c = Serial.read();
        Serial.write(c);
        uart.write(c);
    }
}


bool GSM_PostRequest(const char* host, const char* path, const char* data)
{
    if(!gsmNet)
    {
        gsmNet = gsm->attackGPRS("internet.wind", "", "");
        if(!gsmNet) return false;
    }
    
    return gsm->requestPost(host, path, 80, data, response, RESPONSE_LEN);
}

void WiFi_Init()
{
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int timeConnect = 0;
    while (WiFi.status() != WL_CONNECTED && timeConnect++ < 20) 
    {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected");
        return;
    }
    Serial.println("WiFi connected");
    WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), dns);
}