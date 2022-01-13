
#include "decoder.h"
#include "utils.h"
#include <Arduino.h>
#include <TFT_eSPI.h> 

std::map<String, const char*> g_p1_fields = {
    {"0-0:1.0.0", "date"},
    {"1-0:1.7.0", "presentConsumption_kWh"},
    {"1-0:2.7.0", "presentReturn_kWh"},
    {"1-0:5.7.0", "importedQ1_kvarh"},
    {"1-0:6.7.0", "importedQ2_kvarh"},
    {"1-0:7.7.0", "importedQ3_kvarh"},
    {"1-0:8.7.0", "importedQ4_kvarh"},
    {"1-0:1.8.0", "consumptionTotal_kWh"},
    {"1-0:1.8.1", "consumptionT1_kWh"},
    {"1-0:1.8.2", "consumptionT2_kWh"},
    {"1-0:1.8.3", "consumptionT3_kWh"},
    {"1-0:1.8.4", "consumptionT4_kWh"},
    {"1-0:2.8.0", "returnTotal_kWh"},
    {"1-0:2.8.1", "returnT1_kWh"},
    {"1-0:2.8.2", "returnT2_kWh"},
    {"1-0:2.8.3", "returnT3_kWh"},
    {"1-0:2.8.4", "returnT4_kWh"},
    {"1-0:3.8.0", "QconsumptionTotal_kvarh"},
    {"1-0:4.8.0", "QreturnTotal_kvarh"},
    {"1-0:5.8.0", "Q1consumptionTotal_kvarh"},
    {"1-0:6.8.0", "Q2consumptionTotal_kvarh"},
    {"1-0:7.8.0", "Q3consumptionTotal_kvarh"},
    {"1-0:8.8.0", "Q4consumptionTotal_kvarh"},
    {"1-0:15.8.0", "absoluteEnergyTotal_kWh"},
    {"1-0:13.7.0", "phase"},
    {"1-0:14.7.0", "frequency_Hz"},
    /*{"1-0:21.7.0", "L1power"},
    {"1-0:41.7.0", "L2power"},
    {"1-0:61.7.0", "L3power"},*/ //not supported :( 
    {"1-0:31.7.0", "L1current_A"},
    {"1-0:32.7.0", "L1voltage_V"},
    {"1-0:33.7.0", "L1phase"},
    {"1-0:51.7.0", "L2current_A"},
    {"1-0:52.7.0", "L2voltage_V"},
    {"1-0:53.7.0", "L2phase"},
    {"1-0:71.7.0", "L3current_A"},
    {"1-0:72.7.0", "L3voltage_V"},
    {"1-0:73.7.0", "L3phase"},
};

std::map<String, std::pair<int,int>> g_screen_layout = {
    {"presentConsumption_kWh", {0,0}},
    {"presentReturn_kWh", {0,20}},
    {"frequency_Hz", {95, 20}},
    {"L1power", {0, 40}},
    {"L2power", {0, 60}},
    {"L3power", {0, 80}},
    {"L1current_A", {108, 40}},
    {"L2current_A", {108, 60}},
    {"L3current_A", {108, 80}},
    {"L1voltage_V", {0, 40}},
    {"L2voltage_V", {0, 60}},
    {"L3voltage_V", {0, 80}},
    {"L1phase", {58, 40}},
    {"L2phase", {58, 60}},
    {"L3phase", {58, 80}},
};

void send_debug(const char * msg);


void update_screen(TMeterValues& metervalues, TFT_eSPI& tft)
{
    tft.fillScreen(TFT_BLACK);
    for (auto val: metervalues)
    {
        //Serial.printf("looking for %s\n", val.first);
        auto iter = g_screen_layout.find(val.first);
        if (iter != g_screen_layout.end())
        {
            tft.setCursor(iter->second.first, iter->second.second);
            //Serial.printf("putting %s at %d %d\n", val.first, iter->second.first, iter->second.second);
            tft.print(val.second.c_str());
            //tft.print("000000.000");
        }
    }
}

bool decode_telegram(const char * telegram, size_t len, TMeterValues& metervalues)
{
    static unsigned int currentCRC = 0;
    if (telegram[0] == '/')
    {
        // * Start found. Reset CRC calculation
        currentCRC = CRC16(0x0000,(unsigned char *) telegram, len);
        return false;
    }
    else if (telegram[0] == '!')
    {
        // * Add ! to crc calc
        currentCRC = CRC16(currentCRC,(unsigned char*)telegram, 1);

        char messageCRC[5];
        strncpy(messageCRC, telegram + 1, 4);

        messageCRC[4] = 0;   // * Thanks to HarmOtten (issue 5)
        bool validCRCFound = (strtol(messageCRC, NULL, 16) == currentCRC);


        if (validCRCFound)
        {
            return true;
        }
        else
        {
            send_debug("invalid crc return");
            metervalues.clear();
            return false;
        }
    }
    else
    {
        currentCRC = CRC16(currentCRC, (unsigned char*) telegram, len);
    }

    int pos = 0;
    int state = 0;
    String value;
    String key;
    while (pos < len)
    {
        switch (state) {
            case 0:  // finding OBIS
                if (telegram[pos] == '(')
                {
                    state = 1;
                    pos ++;
                    break;
                }
                if (!isOBIS(telegram[pos]))
                {
                    //send_debug("non-obis character, returning");
                    //send_debug(telegram+pos);
                    return false;
                }
                key += telegram[pos++];
                break;
            case 1: // in value
                if (telegram[pos] == ')' || telegram[pos] == '*')
                {
                    state = 2;
                    break;
                } 
                value += telegram[pos++];
                break;
            case 2:
                break;
        }
        if (state == 2)
        {
            break;
        }
    }
    if (key.length() && value.length())
    {
        //static char buf[1000];
        auto iter = g_p1_fields.find(key.c_str());
        if (iter != g_p1_fields.end())
        {
            metervalues.push_back({iter->second, value});
            //sprintf(buf, "added key: %s, value: %s", iter->second, value.c_str());
        }
        else
        {
            //sprintf(buf, "ignored obis: %s, value: %s", key.c_str(), value.c_str());
        }
        //send_debug(buf);
    }

    return false;
}

