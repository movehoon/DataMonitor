#include "led_indicator.h"

#define PREFS_MODE "MODE"
#define PREFS_AP "AP"
#define PREFS_PW "PW"
#define PREFS_IP "IP"

#define LED_BUILTIN 2
#define PIN_MODE 32
#define PIN_485EN 4

#define PIN_LED1  25
#define PIN_LED2  26
#define PIN_LED3  27

enum {
  LED_NONE,
  LED_BLE_RUNNING,
  LED_WIFI_RUNNING,
  LED_BLE_DISCONNECT,
  LED_WIFI_SERVER_DISCONNECTED,
  LED_WIFI_AP_DISCONNECTED,
};

// pin 5 on the RGB shield is button 1
// (button press will be shown on the iPhone app)
const uint32_t button = 0;

LedIndicator ledIndicator;

bool mode_wifi_f = false;

bool req_ble_report_f;
bool req_titan_monitor_f;

int test_led_count;

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

  if (timer_count_100ms%2) {
    digitalWrite(LED_BUILTIN, ledIndicator.process());
  }

  if (timer_count_100ms%5) {
    RequestWifiProcess();
  }

  if (timer_count_100ms % 10) {
    test_led_count++;
  }

  req_ble_report_f = true;

  req_titan_monitor_f = true;

  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}
// ----- Timer Setup -----//

char tmp_msg[256];
void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200);

  printf("\nStart\n");
  
  // led turned on/off from the iPhone app
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_485EN, OUTPUT);
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  
  // button press will be shown on the iPhone app)
  pinMode(button, INPUT);
  pinMode(PIN_MODE, INPUT);
  mode_wifi_f = digitalRead(PIN_MODE);

  LoadPreference();
  printf("ap=%s\n", GetPreferenceAP());
  printf("pw=%s\n", GetPreferencePW());
  printf("ip=%s\n", GetPreferenceIP());

  if (mode_wifi_f)
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
}

void loop()
{
  delay(1);

//  switch(test_led_count%3) {
//    case 0:
//      digitalWrite(PIN_LED1, 1);
//      digitalWrite(PIN_LED2, 0);
//      digitalWrite(PIN_LED3, 0);
//      break;
//    case 1:
//      digitalWrite(PIN_LED1, 0);
//      digitalWrite(PIN_LED2, 1);u
//      digitalWrite(PIN_LED3, 0);
//      break;
//    case 2:
//      digitalWrite(PIN_LED1, 0);
//      digitalWrite(PIN_LED2, 0);
//      digitalWrite(PIN_LED3, 1);
//      break;
//  }

//  printf("Mode: %d\n", digitalRead(PIN_MODE));

  if (req_titan_monitor_f) {
    req_titan_monitor_f = false;

    // Request command to Titan
    digitalWrite(PIN_485EN, HIGH);
    Serial2.write("\n@01:CURQA\n");
    Serial2.flush();
    digitalWrite(PIN_485EN, LOW);
  }

  if (mode_wifi_f) {
    loopWifi();
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
    if (BleConnected()) {
      ledIndicator.SetBlinkCount(LED_BLE_RUNNING);
    }
    else {
      ledIndicator.SetBlinkCount(LED_BLE_DISCONNECT);
    }
  }

  serialEvent();

  if (req_ble_report_f) {
    req_ble_report_f = false;

    if (BleConnected()) {
//      sprintf(buff, "{CURQA:-0.1234, COUNT:%d, HELLO THIS IS TEST NOW RUNNING}\r\n", count++);
//      SendSplit((uint8_t *)buff, strlen(buff), 20);
//      Serial.print(buff);

      String ble_message = GetBleMessage();
      if (ble_message.length() > 0) {
        parse((char *)ble_message.c_str());
      }
    }
  }
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

      if (BleConnected()) {
        SendSplit(recv_cmd2, cmd2_index+1, 20);
      }
//      uint8_t sendCount = cmd2_index / 20 + 1;
//      for (uint8_t i=0; i<sendCount; i++) {
//        if (i < sendCount-1)
//          BleSend(recv_cmd2+(i*20), 20);
//        else
//          BleSend(recv_cmd2+(i*20), cmd2_index%20);
//      }
      Serial.printf("[U2]%s\n", recv_cmd2);

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
