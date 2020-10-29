#include "ESPAsyncWebServer.h"

#define MODEL_TITAN
//#define MODEL_QNODE

#ifdef MODEL_TITAN
#define ENABLE_WS
#endif

//#define ENABLE_SDCARD
#define ENABLE_DEBUGGING

#define PREFS_MODE "MODE"
#define PREFS_AP "AP"
#define PREFS_PW "PW"
#define PREFS_IP "IP"

#define LED_BUILTIN 25
#define PIN_MODE 32
#define PIN_485EN 4

#define PIN_LED_B  13
#define PIN_LED_G  26
#define PIN_LED_R  27
#define LED_R 0
#define LED_G 1
#define LED_B 2

#define TITAN_ID_MAX 100

//@01:SVON
//#01:SVON=1
//@01:SVOFF
//#01:SVOFF=1
//@01:SVON
//#01:SVON=1
//@01:HSPD=350
//#01:HSPD=350
//@01:ACC=5000
//#01:ACC=5000
//@01:X=12345
//#01:X=1
//@01:X=-54321
//#01:X=1
//@01:JOGXP
//#01:JOGXP=1
//@01:STOPX
//#01:STOPX=1
//@01:JOGXN
//#01:JOGXN=1
//@01:STOPX
//#01:STOPX=1

const char* ssid = "KT_GiGA_2G_Wave2_F763";
const char* password =  "0ahebzh872";

AsyncWebServer server(80);
AsyncWebSocket ws("/titan");
AsyncWebSocketClient *wsClient = 0;
char ws_msg[256];


const int LED_FREQ = 5000;

int mode;
int lcd_init_count = 30;
bool system_ready = false;

bool req_titan_monitor_f;

bool recv_packet_f;
int recv_packet_count;

char WID[32] = "";

void LogD(const char *message) {
#ifdef ENABLE_DEBUGGING
  Serial.print(message);
#endif
}
void LogDln(const char *message) {
#ifdef ENABLE_DEBUGGING
  Serial.println(message);
#endif
}

int count;
char buff[256];
char titan2_buff[256];

int wifi_reset_count = 0;

int found_titanID = 0;
void SearchTitanID() {
//  Serial.println("SearchTitanID");
  delay(100);
  for (uint8_t i=0; i<TITAN_ID_MAX; i++) {
    sprintf(titan2_buff, "@%02d:FIRMVS\n", i);
    Serial2.print(titan2_buff);
    delay(10);
    serialEvent();
    if (found_titanID > 0) {
      break;
    }
  }
}

void GetTitanWID() {
  delay(100);
//  Serial.println("GetTitanWID");
  sprintf(titan2_buff, "@%02d:WID\n", found_titanID);
//  Serial.print(titan2_buff);
  Serial2.print(titan2_buff);
  delay(5);
  serialEvent();
}

void onWsEvent( AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len ) {
  wsClient = client;
  if(type == WS_EVT_CONNECT) {
    LogDln("Websocket client connection received");
  }
  else if(type == WS_EVT_DISCONNECT) {
    wsClient = 0;
    LogDln("Client disconnected");
  }
  else if(type == WS_EVT_DATA) {
//    Serial.println("Data received: ");
//    for(int i=0; i < len; i++) {
//      Serial.print(data[i]);
//      Serial.print("|");
//    }
//    Serial.println();

    memset(titan2_buff, 0, sizeof(titan2_buff));
    if (len > sizeof(titan2_buff))
      len = sizeof(titan2_buff);
    memcpy(titan2_buff, data, len);
    Send2Titan(titan2_buff);
    
//    count++;
//    sprintf(ws_msg, "Reply %d", count);
//    client->text(ws_msg);
  }
}

void setupWS(){
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    
    Serial.println("Connecting to WiFi..");

    wifi_reset_count++;
    if (wifi_reset_count > 10)
      ESP.restart();

    serialEvent();
  }

  Serial.println(WiFi.localIP());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.begin();
}

char tmp_msg[256];
void setup()
{
  delay(3000);

  Serial.begin(115200);
  Serial2.begin(115200);

  printf("\nStart\n");
  
  // led turned on/off from the iPhone app
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_485EN, OUTPUT);

  LoadPreference();
  printf("AP=%s\n", GetPreferenceAP());
  printf("PW=%s\n", GetPreferencePW());
  printf("IP=%s\n", GetPreferenceIP());
  printf("ID=%s\n", GetPreferenceID());

  SearchTitanID();
  GetTitanWID();

#ifdef ENABLE_SDCARD
  setupSdcard();
#endif

  setupWS();

  system_ready = true;
}

void Send2Titan(const char *msg) {
#ifdef ENABLE_DEBUGGING
  printf("Send2Titan %s\n", msg);
#endif
  digitalWrite(PIN_485EN, HIGH);
  Serial2.println(msg);
  Serial2.flush();
  digitalWrite(PIN_485EN, LOW);
}

void loop()
{
  delay(1);

  serialEvent();
}

uint8_t recv_cmd[256+1];
uint8_t cmd_index;
char recv_cmd2[256+1];
uint8_t cmd2_index;
bool u1_data_catch;
uint8_t u1_cmd[256+1];
uint8_t u1_resp_message[32];
void serialEvent () {
  while(Serial.available()) {
    recv_cmd[cmd_index] = (uint8_t)Serial.read();
    if (recv_cmd[cmd_index] == 0x0A || recv_cmd[cmd_index] == 0x0D) {
      recv_cmd[cmd_index] = 0x00;
      memcpy(u1_cmd, recv_cmd, cmd_index+1);

      char *p_token;
      char buf[32];
      char value[32];

      LogD("[U1]");
      LogD((const char *)recv_cmd);
      LogD("\n");
      u1_data_catch = false;

      if (recv_cmd[0] == '@') {
        if (!u1_data_catch) {
          Send2Titan((const char *)u1_cmd);
        }
      }

      if (!u1_data_catch) {
        parse((char *)u1_cmd);
      }
      
      cmd_index = 0;
      continue;
    }
    cmd_index++;
  }
  
  while(Serial2.available()) {
    recv_cmd2[cmd2_index] = (uint8_t)Serial2.read();
    if (recv_cmd2[cmd2_index] == 0x0A) {
      Serial.printf("[U2]%s", recv_cmd2);
      recv_cmd2[cmd2_index+1] = 0x00;
#ifdef ENABLE_SDCARD
      WriteMessage((char *)recv_cmd2);
#endif

#ifdef ENABLE_WS
      if (wsClient) {
        wsClient->text((const char *)recv_cmd2, strlen(recv_cmd2));
//        wsClient->close();
      }
#endif

      parseTitan((char *)recv_cmd2);

      recv_packet_f = true;

      memset(recv_cmd2, 0, sizeof(recv_cmd2));
      cmd2_index = 0;
      continue;
    }
    cmd2_index++;
  }
}

void parseTitan(char* cmd) {
  uint8_t start_index = 0;
  char buf[32];
  char value[32];
//#01:EX=2;VX=0;MST=0x0;CURQA=-0.0263982;CURQD=0;CURDD=0;DIN=0x0;DOUT=0x0
//   Serial.print("parsing: ");
//   Serial.println(cmd);
  
  char *p_token;
  char *c_token;
  p_token = strtok(cmd, ":");

  if (p_token) {
//    Serial.println("foundID");
//    Serial.println(p_token);
    found_titanID = atoi(p_token+1);
//    Serial.print("Found id ");
//    Serial.println(found_titanID);
    
    if (strncmp(p_token, "#", 1) == 0) {
      p_token = strtok(NULL, ":");
//      Serial.println(p_token);

      p_token = strtok(p_token, ";");
      while (p_token) {
        strcpy(buf, p_token);
//        Serial.println(buf);

        if (strncmp(buf, "WID=", 4) == 0) {
          if (strlen(WID) > 0) {
//            ESP.restart();
          }
          strcpy(WID, buf+4);
          for (uint8_t i=0; i < sizeof(WID); i++) {
            if (WID[i] == '\r' || WID[i] == '\n')
              WID[i] = '\0';
          }
          Serial.print("Get WID ");
          Serial.println(WID);
        }
        p_token = strtok(NULL, ";");
      }
    }
  }
}

char msg[256];
void parse(char* cmd) {
  LogD("parsing: ");
  LogDln(cmd);
 
  if (strlen(cmd)==0)
    return;
  
  char *p_token = (char *)strtok(cmd, "=");
  
  if (strncmp(p_token, "#", 1) == 0) {
    // Ignore comment
  }
  else if (strncmp(p_token,"mo",2)==0) {
    p_token = (char *)strtok(NULL, "=");
    if (p_token) {
      SavePreferenceMode(p_token);
    }
    sprintf(msg, "mo=%s\n", GetPreferenceMode());
    printf(msg);
//    SendSplit((uint8_t *)msg, strlen(msg), 20);
  }
  else if (strncmp(p_token,"ap",2)==0) {
    p_token = (char *)strtok(NULL, "=");
    if (p_token) {
      SavePreferenceAP(p_token);
    }
    sprintf(msg, "ap=%s\n", GetPreferenceAP());
    printf(msg);
//    SendSplit((uint8_t *)msg, strlen(msg), 20);
  }
  else if (strncmp(p_token,"pw",2)==0) {
    p_token = (char *)strtok(NULL, "=");
    if (p_token) {
      SavePreferencePW(p_token);
    }
    sprintf(msg, "pw=%s\n", GetPreferencePW());
    printf(msg);
//    SendSplit((uint8_t *)msg, strlen(msg), 20);
  }
  else if (strncmp(p_token,"ip",2)==0) {
    p_token = (char *)strtok(NULL, "=");
    if (p_token) {
      SavePreferenceIP(p_token);
      printf("Save ip to %s\n", p_token);
    }
    sprintf(msg, "ip=%s\n", GetPreferenceIP());
    printf(msg);
//    SendSplit((uint8_t *)msg, strlen(msg), 20);
  }
  else if (strncmp(p_token,"re",2)==0) {
    printf("Restart Requested\n");
    ESP.restart();
  }
  else {
    LogD("Unknown Command");
    LogDln(cmd);
//    Serial.printf("Unknown Command %s\n", cmd);
  }
}
