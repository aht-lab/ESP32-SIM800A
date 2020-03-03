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

/* ======================================== */
/* WIFI */
/* ======================================== */
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define SSID "AHT LAB"
#define PASS "0941732379"

WiFiClient client;
IPAddress dns(8,8,8,8);

// INET
#define APN             "internet.wind"
#define USER            ""
#define PASS            ""
#define HTTP            "http://"
#define HOST            "ibsmanage.com"
#define PATTERN_ACTIVE  "/Active?IDImeiSim=%s&IDImeiDevice=%s&gen=%s&Device=%s&IDPartner=%d&t=%d"
#define BODY_LEN        128
char body[BODY_LEN];

// Params
//char LAT[10], LNG[10];
//char CELL_ID[6], LAC[6];
char PATH_ACTIVE[120];
char IMEI_GSM[20], IMEI_SIM[20];
char GEN[] = "2G", DEVICE_TYPE[] = "ESP32SIM800A", ID_PARTNER[] = "IDPartner";

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
            
        gsm->getIMEI(IMEI_GSM, 20);
        Serial.print("IMEI GSM: ");
        Serial.println(IMEI_GSM);

        gsm->getSimIMEI(IMEI_SIM, 20);
        Serial.print("IMEI SIM: ");
        Serial.println(IMEI_SIM);
    }
}

void loop() 
{
    if(!gsmOk)
    {
        Serial.println("can't detect gsm module");
        delay(5000);
        return;
    }
    
    requestActive();
    delay(20000);
}

void requestActive()
{
    sprintf(PATH_ACTIVE, PATTERN_ACTIVE, IMEI_SIM, IMEI_GSM, GEN, DEVICE_TYPE, ID_PARTNER);
    Serial.println(PATH_ACTIVE);

    bool requestStatus = getRequest(HOST, PATH_ACTIVE);
    if(requestStatus)
    {
        Serial.println("Request thanh cong");
        Serial.print("body: ");
        Serial.println(body);
    }
    Serial.println();
}

bool getRequest(const char* host, const char* path)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WL_NOT_CONNECTED: GPRS Request");
        return GSM_GetRequest(host, path);
    }
    else
    {
        Serial.println("WL_CONNECTED: WIFI Request");
        return WiFi_GetRequest(host, path);
    }
}


bool GSM_GetRequest(const char* host, const char* path)
{
    int gsmAttackNetworkTime = 0;
    while(!gsmNet && gsmAttackNetworkTime++ < 5)
    {
        gsmNet = gsm->attackGPRS("internet.wind", "", "");
    }
    if(!gsmNet) return false;
    
    return gsm->requestGet(host, path, 80, body, BODY_LEN);
}

bool WiFi_GetRequest(const char* host, const char* path) 
{
    HTTPClient http;
    http.begin(String(HTTP) + String(host) + String(path));    
    delay(100);
    int httpResponseCode = http.GET();
    delay(1000);
    String payload = http.getString();
    Serial.print("response: ");
    Serial.println(payload);
    
    Serial.print("Http return code: ");
    Serial.println(httpResponseCode);  

    if(httpResponseCode == 200) 
    {
        Serial.println(F("wifi request success"));
        payload.toCharArray(body, BODY_LEN);
        Serial.println(F("Disconnect server"));
        
        http.end(); 
        return true;
    }
    else
    {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
    return false;
}
