#include "aht_sim800.h"

AHT_SIM800::AHT_SIM800(const AHT_GSM *gsm) 
{
    Serial.println("hello");
}

void AHT_SIM800::hello() 
{
    DB_Println("AHT_SIM800");
}

bool AHT_SIM800::getIMEI(char* imei, int len) 
{
    if(sendAndCheckReply("AT+CGSN", "OK", 2000, 1, 2) == AT_REPLY_FOUND)
    {
        int i = 0;
        while(i < len && _buffer[i] && _buffer[i] != '\r' && _buffer[i] != '\n')
        {
            imei[i] = _buffer[i];
            imei[++i] = '\0';
        }
    }
}

bool AHT_SIM800::getSimIMEI(char* simImei, int len)
{
    if(sendAndCheckReply("AT+CIMI", "OK", 2000, 1, 2) == AT_REPLY_FOUND)
    {
        int i = 0;
        while(i < len && _buffer[i] && _buffer[i] != '\r' && _buffer[i] != '\n')
        {
            simImei[i] = _buffer[i];
            simImei[++i] = '\0';
        }
    }
}

bool AHT_SIM800::getLocation(char* lat, char* lng) 
{
    sendAndReadResponse("AT+SAPBR=1,1", 5000);
    if (!sendAndCheckReply("AT+CIPGSMLOC=1,1", "OK", 5000, 1, 2)) 
    {
        return false;
    }

    bool success = Read_VARS("+CIPGSMLOC: *,%s,%s,", _buffer, lat, lng);
    DB_Println("lat: " + String(lat));
    DB_Println("lng: " + String(lng));

    return success && (atof(lat) != 0 && atof(lng) != 0);
}

bool AHT_SIM800::getCellId(char* cellId, char* lac)
{
    bool success;
    success = sendAndCheckReply("AT+CREG=2", "OK", 2000);
    success = sendAndCheckReply("AT+CREG?", "OK", 2000, 1, 2);
    if (!success) 
    {
        return false;
    }
    
    success = Read_VARS("+CREG: *,*,\"%s\",\"%s\"", _buffer, lac, cellId);
    DB_Println("lac: " + String(lac));
    DB_Println("cellId: " + String(cellId));
    
    return success;
}

char AHT_SIM800::numSMS(byte status) 
{
    int smsNum = 0;
    char cmdAT[25];
    
    switch(status)
    {
        case SMS_STT_ALL:
            sprintf(cmdAT, "AT+CMGL=\"%s\"", "ALL");
            break;
    }
    
    if(sendAndCheckReply("AT+CMGF=1", "OK", 1000, 2) != AT_REPLY_FOUND)
    {
        return -1;
    }

    if (sendAndCheckReply("AT+CSCS=\"UCS2\"", "OK", 1000, 2) != AT_REPLY_FOUND)
    {
        return -1;
    }
    
    _uart->println(cmdAT);
    byte segmentRetVal = 0;
    while(segmentRetVal != SEG_STOP)
    {
        segmentRetVal = readSegment("OK", 2000);
        if(segmentRetVal == SEG_END)
        {
            if(strstr(_buffer, "+CMGL:"))
            {
                char* p = strchr(_buffer, ':');
                if (p == NULL) continue;
                smsNum = atoi((p + 1));
                DB_Println("smsIndex: " + String(smsNum));
            }
        }
    }

    return smsNum;
}

char AHT_SIM800::readSMS(uint8_t index)
{
    char retVal = 0;
    char cmdAT[12];
    sprintf(cmdAT, "AT+CMGR=%d", index);
    
    if(sendAndCheckReply("AT+CMGF=1", "OK", 1000, 2) != AT_REPLY_FOUND)
    {
        return SMS_READ_FAIL;
    }

    if(sendAndCheckReply("AT+CSCS=\"UCS2\"", "OK", 1000,2) != AT_REPLY_FOUND)
    {
        return SMS_READ_FAIL;
    }
        
    if(sendAndCheckReply(cmdAT, "OK", 5000, 2) != AT_REPLY_FOUND)
    {
        return SMS_READ_FAIL;
    }
    
    char phoneUCS2[21], phone[11];
    char* p1 = strchr((char*)_buffer, ',');
    p1 = p1 + 2;
    char* p2 = strchr((char*)p1, ',');
    p2 = p2 - 1;
    *p2 = 0;
    strcpy(phoneUCS2, p1);
    convertUCS2(phoneUCS2, phone);

    DB_Print("\nphoneUCS2: ");
    DB_Println(phoneUCS2);
    DB_Print("\nPhone: ");
    DB_Println(phone);

    return SMS_READ_FAIL;
}

char AHT_SIM800::call(const char* phone)
{
    char AT[16];
    sprintf(AT, "ATD%s;", phone);
    return sendAndCheckReply(AT, "OK", 1000, 2) == AT_REPLY_FOUND;
}

char AHT_SIM800::handup()
{
    return sendAndCheckReply("ATH", "OK", 1000, 2) == AT_REPLY_FOUND;
}

char AHT_SIM800::phoneActiveSTT()
{
    return sendAndCheckReply("AT+CPAS", "+CPAS: 4", 1000, 1) == AT_REPLY_FOUND;
}

char AHT_SIM800::deleteAllSMS() 
{
    return sendAndCheckReply("AT+CMGD=1,4", "OK", 1000, 2) == AT_REPLY_FOUND;
}

void AHT_SIM800::printPhone(const char* phone)
{
    _uart->print(F("AT+CMGS=\""));
    unsigned char i=0;
    while(phone[i])
    {
        _uart->print(F("00"));
        _uart->print(phone[i],HEX);
        i++;
    }
    _uart->println(F("\""));
}

void AHT_SIM800::printSMS(const char* data)
{
    unsigned char i=0;
    String buf="";
    while(data[i])
    {
        unsigned char c = data[i]&0xFF;
        if(c==0xE0)
        {
            _uart->print(F("0E"));
            DB_Print(F("OE"));
            i++;
            c = data[i];
            if(c == 0xB8)
            {
                i++;
                c = data[i]-0x80;
                if(c <= 0x0F)
                {
                  _uart->print(F("0"));
                  DB_Print(F("0"));        
                }
                buf = String(c,HEX);
                buf.toUpperCase();
                _uart->print(buf);
                DB_Print(buf); 
                }
            else
            {
                i++;
                c = data[i]-0x40;
                if(c <= 0x0F)
                {
                    _uart->print(F("0"));
                    DB_Print(F("0")); 
                }
                buf = String(c,HEX);
                buf.toUpperCase();
                _uart->print(buf);
                DB_Print(buf); 
            }     
        }
        else
        {
            _uart->print(F("00"));
            DB_Print(F("00")); 
            if(c == 0x0A)
            {
                _uart->print("0A");
                DB_Print(F("0A")); 
            }
            else if(c == 0x0D)
            {
                _uart->print("0D");
               DB_Print(F("0D")); 
            }
            else
            {
                buf = String(c,HEX);
                buf.toUpperCase();
                _uart->print(buf);
                DB_Print(buf); 
            }
        
        }
        i++;
    }
    DB_Print("\n");
}

void AHT_SIM800::printlnSMS(const char* data) {
    printSMS(data);
    printSMS("\r\n");
}

char AHT_SIM800::sendSMS(const char* phone, const char* content)
{
    if(sendAndCheckReply("AT+CMGF=1", "OK") != AT_REPLY_FOUND)
    {
        return 0;
    }
    if(sendAndCheckReply("AT+CSCS=\"UCS2\"", "OK", 1500) != AT_REPLY_FOUND)
    {
        return 0;
    }
    
    printPhone(phone);
    if(readUntil(1000, ">") == AT_REPLY_FOUND)
    {
        printlnSMS(content);
        _uart->write(0x1A);
        
        return readUntil(10000, "OK") == AT_REPLY_FOUND;
    }
    return 0;
}

char AHT_SIM800::attackGPRS(const char* domain, const char* user, const char* pass)
{
    sendAndReadResponse("AT+CIPSHUT", 2000);
    if(sendAndCheckReply("AT+CGATT=1", "OK", 3000, 3) != AT_REPLY_FOUND)
    {
        return 0;
    }

    print("AT+CSTT=\"");
    print(domain);
    print("\",\"");
    print(user);
    print("\",\"");
    print(pass);
    print("\"\r");

    if(readUntil(5000, "OK") != AT_REPLY_FOUND)
    {
        return 0;
    }

    if(sendAndCheckReply("AT+CIICR", "OK", 3000, 2) != AT_REPLY_FOUND)
    {
        return 0;
    }

    if(sendAndCheckReply("AT+CIFSR", "AT+CIFSR", 2000, 2) != AT_REPLY_FOUND)
    {
        return 0; 
    }
}

char AHT_SIM800::dettachGPRS()
{
    return sendAndCheckReply("AT+CGATT=0", "OK", 5000, 2) == AT_REPLY_FOUND;
}

char AHT_SIM800::connectTCP(const char* server, int port)
{
    print("AT+CIPSTART=\"TCP\",\"");
    print(server);
    print("\",\"");
    print(port);
    print("\"\r");

    if(readUntil(8000, "OK") != AT_REPLY_FOUND)
    {
        return 0;
    }
    println("AT+CIPSEND");
    return readUntil(5000, ">") == AT_REPLY_FOUND;
}

char AHT_SIM800::disconnectTCP()
{
    return sendAndCheckReply("AT+CIPCLOSE", "OK", 2000, 2) == AT_REPLY_FOUND;
}

char AHT_SIM800::requestGet(const char* server, const char* path, int port, char* body, int len)
{
    char end_c[2];
    end_c[0]=0x1a;
    end_c[1]='\0';
    
    if(!connectTCP(server, port))
    {
        return 0;
    }

    print("GET ");
    print(path);
    print(" HTTP/1.0\r\nHost: ");
    print(server);
    print("\r\n");
    print("User-Agent: ESP32");
    print("\r\n\r\n");
    print(end_c);
    print("\r");

    if(readUntil(6000, "SEND OK") == AT_REPLY_FOUND)
    {
        if(readResponse(6000, "200 OK"))
        {
            DB_Println("HTTP STATUS: 200");
            // process header and body
            int i = 0;
            const char* pbody = strstr(_buffer, "\r\n\r\n");
            if(pbody != nullptr)
            {
                pbody += 4;
            }
            while(*pbody && *pbody != '\r' && *pbody != '\n' && i < len)
            {
                body[i++] = *pbody++;
                body[i] = '\0';
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        disconnectTCP();
    }
    
    return 1;
}
