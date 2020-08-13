#include "led_indicator.h"

#define MODEL_TITAN
//#define MODEL_QNODE

#ifdef MODEL_QNODE
#define ENABLE_WIFI
#define ENABLE_DISPLAY
#endif

//#define ENABLE_SDCARD
//#define ENABLE_DEBUGGING

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

enum {
  LED_NONE,
  LED_BLE_RUNNING,
  LED_WIFI_RUNNING,
  LED_BLE_DISCONNECT,
  LED_WIFI_SERVER_DISCONNECTED,
  LED_WIFI_AP_DISCONNECTED,
};

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



LedIndicator ledIndicator;

const int LED_FREQ = 5000;

int mode;
int lcd_init_count = 30;
bool system_ready = false;

bool req_report_ble_f;
bool req_report_wifi_f;
bool req_titan_monitor_f;

int test_led_count;

bool recv_packet_f;
int recv_packet_count;

float curqa;
float curqd;
float curdd;
int ex;
int vx;
int mst;
int din;
int dout;

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
// ----- Timer Setup -----//
hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
int timer_count_100ms;

void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);

  timer_count_100ms++;

  if (lcd_init_count) {
    lcd_init_count--;
  }

  if(recv_packet_f) {
    recv_packet_f = false;
    recv_packet_count++;
  }

  if ((timer_count_100ms%2)==0) {
    digitalWrite(LED_BUILTIN, ledIndicator.process());
  }

  if ((timer_count_100ms%5)==0) {
    RequestWifiProcess();
  }

  if ((timer_count_100ms%10)==0) {
//    Serial.println(GetDeviceName());
    test_led_count++;
    req_report_wifi_f = true;
  }

  if ((timer_count_100ms%100)==0) {
    timer_count_100ms = 0;
  }

  req_report_ble_f = true;
  req_titan_monitor_f = true;

  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}
// ----- Timer Setup -----//

char default_name[32];
char qnode_name[32];
const char *GetDefaultName() {
  uint64_t chipid = ESP.getEfuseMac();
#ifdef MODEL_TITAN
  sprintf(default_name, "TITAN_%04X", (uint16_t)chipid);
#elif MODE_QNODE
  sprintf(default_name, "QNODE_%04X", (uint16_t)chipid);
#endif
  return default_name;
}
const char *GetRFID() {
  if (strlen(GetPreferenceID()) > 0)
    return GetPreferenceID();
  else
    return GetDefaultName();
}
const char *GetDeviceName() {
  sprintf(qnode_name, "ARCUS(%s)", GetRFID());
//  Serial.println(qnode_name);
  return qnode_name;
}

char disp_buff[32];
void LedSetup() {
  ledcSetup(LED_B, LED_FREQ, 8);
  ledcAttachPin(PIN_LED_B, LED_B);
  ledcSetup(LED_G, LED_FREQ, 8);
  ledcAttachPin(PIN_LED_G, LED_G);
  ledcSetup(LED_R, LED_FREQ, 8);
  ledcAttachPin(PIN_LED_R, LED_R);

  int led_delay = 2;
  for (int i=0; i<100; i++) {
    ledcWrite(LED_R, i);
    delay(led_delay);
  }
  for (int i=100; i>0; i--) {
    ledcWrite(LED_R, i);
    delay(led_delay);
  }
  ledcWrite(LED_R, 0);
  for (int i=0; i<100; i++) {
    ledcWrite(LED_G, i);
    delay(led_delay);
  }
  for (int i=100; i>0; i--) {
    ledcWrite(LED_G, i);
    delay(led_delay);
  }
  ledcWrite(LED_G, 0);
  for (int i=0; i<100; i++) {
    ledcWrite(LED_B, i);
    delay(led_delay);
  }
  for (int i=100; i>0; i--) {
    ledcWrite(LED_B, i);
    delay(led_delay);
  }
  ledcWrite(LED_B, 0);
}

char tmp_msg[256];
void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200);

  printf("\nStart\n");
  
  // led turned on/off from the iPhone app
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_485EN, OUTPUT);

  LedSetup();

  pinMode(PIN_MODE, INPUT);
  mode = 0;
  #ifdef ENABLE_WIFI
  mode = digitalRead(PIN_MODE);
  #endif
  printf("mode is %d\n", mode);

  LoadPreference();
  printf("AP=%s\n", GetPreferenceAP());
  printf("PW=%s\n", GetPreferencePW());
  printf("IP=%s\n", GetPreferenceIP());
  printf("ID=%s\n", GetPreferenceID());

#ifdef ENABLE_DISPLAY
  setupDisplay();
#endif
#ifdef ENABLE_SDCARD
  setupSdcard();
#endif

  if (mode)
    setupWifi();
  else
    setupBle();

  // ----- Start Timer -----//
  // Create semaphore to inform us when the timer has fired
  timerSemaphore = xSemaphoreCreateBinary();
  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more info).
  timer = timerBegin(0, 80, true);
  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true);
  // Set alarm to call onTimer function every 200 microsecond (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 100000, true);
  // Start an alarm
  timerAlarmEnable(timer);
  // ----- Start Timer -----//  

  system_ready = true;
}

void Send2Titan(const char *msg) {
#ifdef ENABLE_LOGGING
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

//  printf("Mode: %d\n", digitalRead(PIN_MODE));

  if (req_titan_monitor_f) {
    req_titan_monitor_f = false;

    // Request command to Titan
    if (mode) {
      uint8_t cmd_delay = 10;
      Send2Titan("\n@01:EX;VX;MST;CURQA;CURQD;CURDD;DIN;DOUT\n");
    }
  }

  sprintf(buff, "{\"EX\":%d,\"VX\":%d,\"MST\":%d,\"CURQA\":%1.04f,\"CURQD\":%1.04f,\"CURDD\":%1.04f,\"DIN\":%d,\"DOUT\":%d}\r\n", ex, vx, mst, curqa, curqd, curdd, din, dout);
//  Serial.print(buff);

  if (mode) {
    loopWifi();

    if (req_report_wifi_f) {
      req_report_wifi_f = false;

      wifi_send(buff);
    }
    
    if (GetWifiStatus() == 0) {
      ledIndicator.SetBlinkCount(LED_WIFI_AP_DISCONNECTED);
    }
    else if (GetWifiStatus() == 1) {
      ledIndicator.SetBlinkCount(LED_WIFI_SERVER_DISCONNECTED);
    }
    else if (GetWifiStatus() == 2) {
      ledIndicator.SetBlinkCount(LED_WIFI_RUNNING);
    }
  }
  else {
    loopBle();

    if (req_report_ble_f) {
      req_report_ble_f = false;
      
//      if (BleConnected()) {
//        SendSplit((uint8_t *)buff, strlen(buff), 20);
//      }
    }

    if (BleConnected()) {
      ledIndicator.SetBlinkCount(LED_BLE_RUNNING);
    }
    else {
      ledIndicator.SetBlinkCount(LED_BLE_DISCONNECT);
    }

    String ble_message = GetBleMessage();
    if (ble_message.length() > 0) {
      Send2Titan(ble_message.c_str());
      parse((char *)ble_message.c_str());
    }
  }

#ifdef ENABLE_DISPLAY
  if (!lcd_init_count) {
    displayPreProcess();

    displayLine(1, GetDeviceName());
    
    if (mode) {
      if (GetWifiStatus() == 0) {
        strcpy(disp_buff, "WiFi Disconnect");
      }
      else if (GetWifiStatus() == 1) {
        strcpy(disp_buff, "WiFi AP Connected");
      }
      else if (GetWifiStatus() == 2) {
        strcpy(disp_buff, "WiFi Connected");
      }
    }
    else {
      if (BleConnected()) {
        strcpy(disp_buff, "BT Connected");
      }
      else {
        strcpy(disp_buff, "BT Disconnect");
      }
    }
    displayLine(2, disp_buff);

    if (recv_packet_count%4 ==0) {
      strcpy(disp_buff, "Data -");
    }
    else if (recv_packet_count%4 == 1) {
      strcpy(disp_buff, "Data \\");
    }
    else if (recv_packet_count%4 == 2) {
      strcpy(disp_buff, "Data |");
    }
    else if (recv_packet_count%4 == 3) {
      strcpy(disp_buff, "Data /");
    }
    displayLine(3, disp_buff);

#ifdef ENABLE_SDCARD
    float sdUsed = SDUsedBytes();
    float sdTotal = SDTotalBytes();
    sprintf(disp_buff, "[SD]%0.1f/%0.1fG", sdUsed / (1024*1024*1024), sdTotal / (1024 * 1024 * 1024));    
    displayLine(4, disp_buff);
#endif
    displayPostProcess();
  }
#endif

  serialEvent();
}

void SendSplit(uint8_t *buff, int len, int unit)
{
  uint8_t sendCount = len / unit + 1;
  for (uint8_t i=0; i<sendCount; i++) {
    if (i < sendCount-1) {
      BleSend(buff+(i*unit), unit);
    }
    else {
      BleSend(buff+(i*unit), len%unit+1);
    }
  }
}

uint8_t recv_cmd[256+1];
uint8_t cmd_index;
uint8_t recv_cmd2[256+1];
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
        p_token = strtok((char *)recv_cmd, ":");
        p_token = strtok(NULL, ";");
        if (p_token) {
          strcpy(buf, p_token);
          
          LogD("buf: ");
          LogDln(buf);

          if (strncmp(buf, "WID", 3) == 0) {
            if (buf[3] == '=') {
              bool valid = false;
              strncpy(value, buf+4, 32);
              String str = value;
              if (str.length() <= 8) {
                valid = true;
                for (uint8_t i=0; i<str.length(); i++) {
                  if (('A' <= str.charAt(i) && str.charAt(i) <= 'Z') || ('0' <= str.charAt(i) && str.charAt(i) <= '9')) {
                  }
                  else {
                    valid = false;
                  }
                }
                if (valid) {
                  SavePreferenceID(str);
                  LogD("RFID value: ");
                  LogDln(value);
                }
              }
              u1_data_catch = true;
            }
            String msg = "#01:" + String(GetRFID());
//            sprintf(u1_resp_message, "#01:%s", GetRFID());
            Serial.println(msg.c_str());
            u1_data_catch = true;
          }
        }

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
      recv_cmd2[cmd2_index+1] = 0x00;
      Serial.printf("%s\n", recv_cmd2);
#ifdef ENABLE_SDCARD
      WriteMessage((char *)recv_cmd2);
#endif

      SendSplit((uint8_t *)recv_cmd2, strlen((char *)recv_cmd2), 20);

#ifdef ENABBLE_WIFI
      parseTitan((char *)recv_cmd2);
#endif

      recv_packet_f = true;

//      sprintf(buff, "{CURQA:%f}\r\n", curqa);
//      Serial.print(buff);

//      uint8_t sendCount = cmd2_index / 20 + 1;
//      for (uint8_t i=0; i<sendCount; i++) {
//        if (i < sendCount-1)
//          BleSend(recv_cmd2+(i*20), 20);
//        else
//          BleSend(recv_cmd2+(i*20), cmd2_index%20);
//      }

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
    if (strncmp(p_token, "#", 1) == 0) {
      p_token = strtok(NULL, ":");
//      Serial.println(p_token);

      p_token = strtok(p_token, ";");
      while (p_token) {
        strcpy(buf, p_token);
//        Serial.println(buf);

        if (strncmp(buf, "EX=", 3) == 0) {
          strcpy(value, buf+3);
          ex = atoi(value);
//          Serial.print("EX value: ");
//          Serial.println(ex);
        }
        else if (strncmp(buf, "VX=", 3) == 0) {
          strcpy(value, buf+3);
          vx = atoi(value);
//          Serial.print("VX value: ");
//          Serial.println(vx);
        }
        else if (strncmp(buf, "MST=", 4) == 0) {
          strcpy(value, buf+4);
          mst = atoi(value);
//          Serial.print("MST value: ");
//          Serial.println(mst);
        }
        else if (strncmp(buf, "CURQA=", 6) == 0) {
          strcpy(value, buf+6);
          curqa = atof(value);
//          Serial.print("CURQA value: ");
//          Serial.println(curqa);
        }
        else if (strncmp(buf, "CURQD=", 6) == 0) {
          strcpy(value, buf+6);
          curqd = atof(value);
//          Serial.print("CURQD value: ");
//          Serial.println(curqd);
        }
        else if (strncmp(buf, "CURDD=", 6) == 0) {
          strcpy(value, buf+6);
          curdd = atof(value);
//          Serial.print("CURDD value: ");
//          Serial.println(curdd);
        }
        else if (strncmp(buf, "DIN=", 4) == 0) {
          strcpy(value, buf+4);
          din = atoi(value);
//          Serial.print("DIN value: ");
//          Serial.println(din);
        }
        else if (strncmp(buf, "DOUT=", 5) == 0) {
          strcpy(value, buf+5);
          dout = atoi(value);
//          Serial.print("DOUT value: ");
//          Serial.println(dout);
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
  else if (strncmp(p_token,"id",2)==0) {
    sprintf(msg, "id=%s\n", GetDeviceName());
    printf(msg);
    SendSplit((uint8_t *)msg, strlen(msg), 20);
  }
  else if (strncmp(p_token,"mo",2)==0) {
    p_token = (char *)strtok(NULL, "=");
    if (p_token) {
      SavePreferenceMode(p_token);
    }
    sprintf(msg, "mo=%s\n", GetPreferenceMode());
    printf(msg);
    SendSplit((uint8_t *)msg, strlen(msg), 20);
  }
  else if (strncmp(p_token,"ap",2)==0) {
    p_token = (char *)strtok(NULL, "=");
    if (p_token) {
      SavePreferenceAP(p_token);
    }
    sprintf(msg, "ap=%s\n", GetPreferenceAP());
    printf(msg);
    SendSplit((uint8_t *)msg, strlen(msg), 20);
  }
  else if (strncmp(p_token,"pw",2)==0) {
    p_token = (char *)strtok(NULL, "=");
    if (p_token) {
      SavePreferencePW(p_token);
    }
    sprintf(msg, "pw=%s\n", GetPreferencePW());
    printf(msg);
    SendSplit((uint8_t *)msg, strlen(msg), 20);
  }
  else if (strncmp(p_token,"ip",2)==0) {
    p_token = (char *)strtok(NULL, "=");
    if (p_token) {
      SavePreferenceIP(p_token);
      printf("Save ip to %s\n", p_token);
    }
    sprintf(msg, "ip=%s\n", GetPreferenceIP());
    printf(msg);
    SendSplit((uint8_t *)msg, strlen(msg), 20);
  }
  else if (strncmp(p_token,"re",2)==0) {
    ESP.restart();
  }
  else {
    Serial.printf("Unknown Command %s\n", cmd);
  }
}
