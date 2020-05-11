#include "led_indicator.h"

#define PREFS_MODE "MODE"
#define PREFS_AP "AP"
#define PREFS_PW "PW"
#define PREFS_IP "IP"

#define LED_BUILTIN 15
#define LED_WIFI 2
#define PIN_MODE 32
#define PIN_485EN 4

enum {
  LED_NONE,
  LED_BLE_RUNNING,
  LED_WIFI_RUNNING,
  LED_BLE_DISCONNECT,
  LED_WIFI_SERVER_DISCONNECTED,
  LED_WIFI_AP_DISCONNECTED,
};


LedIndicator ledBle;
LedIndicator ledWifi;

int mode;
int lcd_init_count = 30;
bool system_ready = false;

bool req_report_ble_f;
bool req_report_wifi_f;

bool recv_packet_f;
int recv_packet_count;

bool req_led_refresh_f;

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

  // Process 100ms periods
  req_report_ble_f = true;
  req_led_refresh_f = true;

  if (lcd_init_count) {
    lcd_init_count--;
  }

  if(recv_packet_f) {
    recv_packet_f = false;
    recv_packet_count++;
  }

  // Process 200ms Periods
  if ((timer_count_100ms%2)==0) {
    digitalWrite(LED_BUILTIN, ledBle.process());
    digitalWrite(LED_WIFI, ledWifi.process());
  }

  // Process 500ms Periods
  if ((timer_count_100ms%5)==0) {
    RequestWifiProcess();
  }

  // Process 1s Periods
  if ((timer_count_100ms%10)==0) {
    req_report_wifi_f = true;
  }

  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}
// ----- Timer Setup -----//

char qnode_name[32];
const char *GetDeviceName() {
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(qnode_name, "OASIS_%04X", (uint16_t)chipid);
  Serial.println(qnode_name);
  return qnode_name;
}

char disp_buff[32];

char tmp_msg[256];
void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200);

  printf("\nStart\n");
  
  // led turned on/off from the iPhone app
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_WIFI, OUTPUT);
  pinMode(PIN_485EN, OUTPUT);

  pinMode(PIN_MODE, INPUT);
  mode = digitalRead(PIN_MODE);
  mode = 0;
  printf("mode is %d\n", mode);

  LoadPreference();
  printf("ap=%s\n", GetPreferenceAP());
  printf("pw=%s\n", GetPreferencePW());
  printf("ip=%s\n", GetPreferenceIP());

  setupDisplay();
//  setupSdcard();

  if (mode)
    setupWifi();
  else
    setupBle();

  // ----- Start Timer -----//
  // Create semaphore to inform us when the timer has fired
  timerSemaphore = xSemaphoreCreateBinary();
  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
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

void loop()
{
  delay(1);

//  printf("Mode: %d\n", digitalRead(PIN_MODE));

  if (mode) {
    loopWifi();

    if (req_report_wifi_f) {
      req_report_wifi_f = false;

      wifi_send(buff);
    }
    
    if (GetWifiStatus() == 0) {
      ledWifi.SetBlinkCount(1);
    }
    else if (GetWifiStatus() == 1) {
      ledWifi.SetBlinkCount(2);
    }
    else if (GetWifiStatus() == 2) {
      ledWifi.SetBlinkCount(3);
    }
  }
  else {
    loopBle();

    if (BleConnected()) {
      ledBle.SetBlinkCount(1);
    }
    else {
      ledBle.SetBlinkCount(2);
    }

    String ble_message = GetBleMessage();
    if (ble_message.length() > 0) {
      parse((char *)ble_message.c_str());
    }
  }


  if (!lcd_init_count && req_led_refresh_f) {
    req_led_refresh_f = false;
    
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

    displayLine(4, "from OnAirSoft");
    
    displayPostProcess();
  }

  serialEvent();
}

void SendSplit(uint8_t *buff, int len, int unit)
{
  uint8_t sendCount = len / unit + 1;
  for (uint8_t i=0; i<sendCount; i++) {
    if (i < sendCount-1)
      BleSend(buff+(i*unit), unit);
    else
      BleSend(buff+(i*unit), len%unit);
  }
}

uint8_t recv_cmd[256];
uint8_t cmd_index;
uint8_t recv_cmd2[256];
uint8_t cmd2_index;
void serialEvent () {
  while(Serial.available()) {
    recv_cmd[cmd_index] = (uint8_t)Serial.read();
    if (recv_cmd[cmd_index] == 0x0A || recv_cmd[cmd_index] == 0x0D) {
      recv_cmd[cmd_index] = 0x00;

      Serial.printf("[U1]%s\n", recv_cmd);
      parse((char *)recv_cmd);

      cmd_index = 0;
      continue;
    }
    cmd_index++;
  }
  
  while(Serial2.available()) {
    recv_cmd2[cmd2_index] = (uint8_t)Serial2.read();
    if (recv_cmd2[cmd2_index] == 0x0A) {
      recv_cmd2[cmd2_index+1] = 0x00;
      Serial.printf("[U2]%s\n", recv_cmd2);

      SendSplit((uint8_t *)recv_cmd2, strlen((char *)recv_cmd2), 20);

      cmd2_index = 0;
      continue;
    }
    cmd2_index++;
  }
}

char msg[256];
void parse(char* cmd) {
  Serial.print("parsing: ");
  Serial.println(cmd);
  
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
