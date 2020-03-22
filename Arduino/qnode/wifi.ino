#include "WiFi.h"
#include "PubSubClient.h"

#define TOPIC_PREFIX "/oneM2M/req"

bool req_wifi_process_f = false;
void RequestWifiProcess() {
  req_wifi_process_f = true;
}

char MAC_char[20];

char topic[256];
char topic_sub[256];

WiFiClient espClient;
PubSubClient client(espClient);

enum {
  WIFI_DISCONNECTED,
  WIFI_AP_CONNECTED,
  WIFI_SERVER_CONNECTED
};

uint8_t wifi_status;
uint8_t GetWifiStatus() {
  return wifi_status;
}

void callback(char* topic, byte* payload, unsigned int length) {
  char *msg = (char *)malloc(length + 4);
  if (!msg)
    return;
  memcpy(msg, "rm=", 3);
  memcpy(msg+3, payload, length);
  msg[length+3] = 0;
  Serial.println(msg);
  free(msg);
}

String getMacAddress() {
  uint64_t chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
  sprintf(MAC_char, "%04X",(uint16_t)(chipid>>32)); //print High 2 bytes
  sprintf(MAC_char+4, "%08X",(uint32_t)chipid);       //print Low 4bytes.
  return String(MAC_char);
}

void setupWifi() {
  getMacAddress();
  sprintf(topic, "%s/%s", TOPIC_PREFIX, MAC_char);
  sprintf(topic_sub, "%s/sub", topic);
  Serial.print("topic: ");
  Serial.println(topic);
  
  WiFi.begin(GetPreferenceAP(), GetPreferencePW());
  printf("AP length is %d\n", strlen(GetPreferenceAP()));

  Serial.printf("WiFi Connecting to %s with %s\n", GetPreferenceAP(), GetPreferencePW());
}

int wifi_count;
char wifi_msg[256];
long lastChecked = 0;
void loopWifi() {
  if (req_wifi_process_f) {
    req_wifi_process_f = false;
    
    long now = millis ();
    long diff = now - lastChecked;
    if (WiFi.status() != WL_CONNECTED) {
      wifi_status = WIFI_DISCONNECTED;
      
      if (diff > 3000) {
        lastChecked = now;
  
        Serial.println("Connecting to WiFi router");
        WiFi.begin(GetPreferenceAP(), GetPreferencePW());
      }
    }
    else {
      if (!client.connected()) {
        wifi_status = WIFI_AP_CONNECTED;
        
        if (diff > 3000) {
          client.setServer(GetPreferenceIP(), 1883);
          client.setCallback(callback);
          Serial.println("Connecting to MQTT broker");
          if (client.connect(MAC_char)) {
            client.subscribe(topic_sub);
          }
          lastChecked = millis();
        }
      }
      else
      {
        wifi_status = WIFI_SERVER_CONNECTED;
        
//        printf("MQTT Connected\n");
//        wifi_count++;
//        sprintf(wifi_msg, "{CURQA:-0.1234, COUNT:%d, HELLO THIS IS TEST NOW RUNNING}", wifi_count);

  //      sprintf(wifi_msg, "{\"EX\":%s,\"VX\":%s,\"CURQA\":\"%s\"}",TITAN_EX,TITAN_VX,TITAN_CURQA);
//        client.publish(topic, wifi_msg);
  //      Serial.print(topic);
  //      Serial.print(" with ");
  //      Serial.println(wifi_msg);
      }
    }
    client.loop();
  }
}
