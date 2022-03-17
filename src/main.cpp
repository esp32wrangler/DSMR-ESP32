#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiClientSecure.h>
#include "decoder.h"
#include "utils.h"
#include "secrets.h"
#include <esp_task_wdt.h>

#include <TFT_eSPI.h>                 // Include the graphics library (this includes the sprite functions)



TFT_eSPI    tft = TFT_eSPI();  


#define GMT_OFFSET_SEC 3600

const char * MQTT_TOPIC ("p1meter");
const char * MQTT_DEBUG_TOPIC ("debug");
#define WDT_TIMEOUT 30

TMeterValues g_meter_values;




// openssl s_client -showcerts -connect 36ba4dc3dbfc416b8cc633a9210e4cce.s1.eu.hivemq.cloud:8883 -> root CA in chain (last item)

extern const uint8_t rootca_crt_bundle_start[] asm("_binary_data_cert_x509_crt_bundle_bin_start");

/*const char* rootCA_cert PROGMEM = "-----BEGIN CERTIFICATE-----\n" \
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAwTzELMAkGA1UE\n" \
"BhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2VhcmNoIEdyb3VwMRUwEwYDVQQD\n" \
"EwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQG\n" \
"EwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMT\n" \
"DElTUkcgUm9vdCBYMTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54r\n" \
"Vygch77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+0TM8ukj1\n" \
"3Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6UA5/TR5d8mUgjU+g4rk8K\n" \
"b4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sWT8KOEUt+zwvo/7V3LvSye0rgTBIlDHCN\n" \
"Aymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyHB5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ\n" \
"4Q7e2RCOFvu396j3x+UCB5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf\n" \
"1b0SHzUvKBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWnOlFu\n" \
"hjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTnjh8BCNAw1FtxNrQH\n" \
"usEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbwqHyGO0aoSCqI3Haadr8faqU9GY/r\n" \
"OPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CIrU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4G\n" \
"A1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY\n" \
"9umbbjANBgkqhkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n" \
"ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ3BebYhtF8GaV\n" \
"0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KKNFtY2PwByVS5uCbMiogziUwt\n" \
"hDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJw\n" \
"TdwJx4nLCgdNbOhdjsnvzqvHu7UrTkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nx\n" \
"e5AW0wdeRlN8NwdCjNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZA\n" \
"JzVcoyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq4RgqsahD\n" \
"YVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPAmRGunUHBcnWEvgJBQl9n\n" \
"JEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57demyPxgcYxn/eR44/KJ4EBs+lVDR3veyJ\n" \
"m+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n" \
"-----END CERTIFICATE-----\n";*/

WiFiClientSecure socketClient;
PubSubClient mqttClient (socketClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (500)
char msg[MSG_BUFFER_SIZE];
int value = 0;

bool g_screenState = false;



#define PIN_SERIAL_RX 13
#define PIN_SERIAL_TX 17

void setDateTime() {
  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "hu.pool.ntp.org", "time.google.com");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  int attempts = 0;
  while (now < 8 * 3600 * 2) {
    if (attempts > 60)
    {
      ESP.restart();
    }
    delay(1000);
    Serial.print(".");
    now = time(nullptr);
    attempts++;
  }
  Serial.println();

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("%s", asctime(&timeinfo));
}

void send_debug(const char * msg);

int first_connect = true;

void reconnect() {
  int attempts = 0;
  // Loop until we’re reconnected
  while (!socketClient.connected()) {
    String clientId = "basicPubSub";
    // Attempt to connect
    // Insert your password
    //if (mqttClient.connect(clientId.c_str(), "test1", HIVEMQ_PWD)) {
    if (mqttClient.connect(clientId.c_str())) {
      if (first_connect)
      {
        first_connect = false;
        send_debug("mqtt first connect");
      }
      else
      {
        send_debug("mqtt reconnected");
      }
    } else {
      Serial.print("failed, rc = ");
      Serial.print(mqttClient.state());
      Serial.println(" restart");
      if (attempts++ == 10)
      {
       ESP.restart();
      }
      esp_task_wdt_reset();
      delay(3000);

    }
  }
}



void setup() {
  pinMode(35, INPUT);
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX, true, 0);
  Serial2.setRxBufferSize(1024);
  WiFi.begin(ssid, password);

  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setTextSize(1);

  digitalWrite(TFT_BL, g_screenState);

  int attempts = 0;
  tft.println("Connecting to Wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
    if (attempts++ == 60)
    {
      ESP.restart();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  tft.println("Setting Date...");

  setDateTime ();
 
  randomSeed(esp_random());

  socketClient.setCertificate(p1meter_cert);
  socketClient.setPrivateKey(p1meter_private_key);
  socketClient.setCACert(aws_cert);

  //espClient.setCACert(rootCA_cert);
  //espClient.setCACert((const char *)rootca_pem_crt_start);

  //Serial.printf("CA address %x %s", &rootca_pem_crt_start, rootca_pem_crt_start);

  //socketClient.setCACertBundle(rootca_crt_bundle_start);

  //Serial.printf("CA address %x", &rootCA_cert);
  //Serial.printf("CA address %x %s", &rootca_crt_start, rootca_crt_start);
  //espClient.setInsecure();
  /*SPIFFS.begin();
  File ca = SPIFFS.open("/certs.ar", "r");
  if(!ca) {
    Serial.println("FIle not Found!");
    return;  
  }
   */

  mqttClient.setServer(mqtt_server, 8883);
  mqttClient.setBufferSize(1500);
  
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch*/
}

void send_mqtt_message (const char* topic, const char * message)
{
    if (!mqttClient.publish(topic, message))
    {
      Serial.printf("Failed to publish mqtt message %s %s\n", topic, message);
    }
}

void send_debug(const char * msg)
{
  String jsonmsg("{\"message\": \"");
  jsonmsg.concat(msg);
  jsonmsg.concat("\"}");
  send_mqtt_message(MQTT_DEBUG_TOPIC, jsonmsg.c_str());
}

std::vector<std::pair<const char*, int>> date_map = 
{
  {"date_yr", 0},
  {"date_mo", 2},
  {"date_day", 4},
  {"time_h", 6},
  {"time_m", 8},
  {"time_s", 10}
};

void unpack_date()
{
  for (auto meter_value : g_meter_values)
  {
    if (strcmp(meter_value.first, "date") == 0) {
      char buf[] = "..";
      for (auto elem : date_map)
      {
        buf[0] = meter_value.second[elem.second];
        buf[1] = meter_value.second[elem.second+1];
        g_meter_values.push_back({elem.first, buf});
      }
      g_meter_values.push_back({"date_dst", meter_value.second[12] == 'S'?"1":"0"});
    }
  }
}

void send_meter_values()
{
  static char buf [1500];
  char * ptr = buf + sprintf(buf, "{\n");
  bool first = true;
  for (auto meter_value : g_meter_values)
  {
    if (!first)
    {
      ptr += sprintf (ptr, ",\n");
    }
    else
    {
      first = false;
    }

    const char * nzptr = meter_value.second.c_str();

    do
    {
      if (*nzptr != '0')
      {
        break;
      } 
      else
      {
        char next = *(nzptr+1);
        if (next == '.' || next == 0)
        {
          break;
        }
      }
    } while(*++nzptr);

    if (isNumeric(nzptr))
    {
      ptr += sprintf (ptr, "  \"%s\": %s", meter_value.first, nzptr);
    }
    else
    {
      ptr += sprintf (ptr, "  \"%s\": \"%s\"", meter_value.first, nzptr);
    }

  }
  sprintf(ptr, "\n}");
  send_mqtt_message(MQTT_TOPIC, buf);
}


void testlayout()
{
  g_meter_values.clear();
    g_meter_values.push_back({"date", "220114103300W"});

  g_meter_values.push_back({"presentConsumption_kWh", "000000.111"});
  g_meter_values.push_back({"presentReturn_kWh", "202020.222"});
  g_meter_values.push_back({"frequency_Hz", "49.85"});
  g_meter_values.push_back({"L1current_A", "000"});
  g_meter_values.push_back({"L1voltage_V", "231.23"});
  g_meter_values.push_back({"L1phase", "0.198"});
  g_meter_values.push_back({"L2current_A", "002"});
  g_meter_values.push_back({"L2voltage_V", "232.34"});
  g_meter_values.push_back({"L2phase", "0.287"});
  g_meter_values.push_back({"L3current_A", "003"});
  g_meter_values.push_back({"L3voltage_V", "233.45"});
  g_meter_values.push_back({"L3phase","0.345"});
  update_screen(g_meter_values, tft);
  unpack_date();
  send_meter_values();
  send_debug("testlayout has been done");
  delay(100000);
}

void screenButtonLoop()
{
  static int prevstate = 0;
  int read = digitalRead(35);
  if (prevstate != read)
  {
    prevstate = read;
    if (read == 0)
    {
      g_screenState = !g_screenState;
      digitalWrite(TFT_BL, g_screenState);
    }
  }
}



void loop() {

  #define MAX_LINE_LEN 1050
  static char linebuffer[MAX_LINE_LEN];
  static unsigned long lastHeartBeat = 0;
  static unsigned int heartBeatChar = 0;

  screenButtonLoop();

  esp_task_wdt_reset();

  if (g_screenState)
  {
    if (millis()-lastHeartBeat > 1000)
    {
      lastHeartBeat = millis();
      tft.setCursor(125, 0);
      tft.printf("%c", "/-\\|"[heartBeatChar++%4]);
    }
  }

  if (!mqttClient.connected()) {
    tft.setCursor(0, 200);
    tft.print("Reconnecting...");
    reconnect();
    tft.setCursor(0, 200);
    tft.print("Reconnected     ");
  }

  mqttClient.loop();

//  testlayout();


  if (Serial2.available())
  {
      size_t len = Serial2.readBytesUntil('\n', linebuffer, MAX_LINE_LEN-2);
      
      if (len > 0 && linebuffer[0] != 0)
      {
        linebuffer[len++] = '\n';
        linebuffer[len] = 0;
//        send_debug(linebuffer);
        if (decode_telegram(linebuffer, len, g_meter_values))
        {
          unpack_date();
          send_meter_values();
          if (g_screenState)
          {
            update_screen(g_meter_values, tft);
          }
          g_meter_values.clear();
          tft.setCursor(115, 0);
          tft.printf("%c", '*');
        }
      }
  }
  
}