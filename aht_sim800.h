#ifndef _AHT_SIM800_H__
#define _AHT_SIM800_H__

#include <Arduino.h>
#ifndef ESP32
#include <SoftwareSerial.h>
#endif

#include "ultils.h"
#include "aht_gsm.h"

class AHT_SIM800: public AHT_GSM 
{
    public:
        AHT_SIM800(): AHT_GSM() {};
        AHT_SIM800(HardwareSerial *uart): AHT_GSM(uart) {};
        #ifndef ESP32
        AHT_SIM800(SoftwareSerial *uart): AHT_GSM(uart) {};
        #endif

        AHT_SIM800(const AHT_GSM *gsm);
        
        void    hello();

        // IMEI
        bool    getIMEI         (char* imei, int len);
        bool    getSimIMEI      (char* simImei, int len);
        
        // GPS
        bool    getLocation     (char* lat, char* lng);
        bool    getCellId       (char* cellId, char* lac);

        // SMS
        char    numSMS          (byte status);
        char    readSMS         (uint8_t index);
        char    deleteAllSMS    ();
        char    sendSMS         (const char* phone, const char* content);
        
        void    printPhone      (const char* phone);
        void    printSMS        (const char* sms);
        void    printlnSMS      (const char* sms);

        // CALL
        char    call            (const char* phone);
        char    handup          ();
        char    phoneActiveSTT  ();

        // NETWORK
        char    attackGPRS      (const char* domain, const char* uname, const char* pword);
        char    dettachGPRS     ();
        char    connectTCP      (const char* server, int port);
        char    disconnectTCP   ();
        char    requestGet      (const char* server, const char* path, int port, char* body, int len);
        
};

#endif
