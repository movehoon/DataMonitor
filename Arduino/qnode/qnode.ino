#include "led_indicator.h"

#define PREFS_MODE "MODE"
#define PREFS_AP "AP"
#define PREFS_PW "PW"
#define PREFS_IP "IP"

#define LED_BUILTIN 2

enum {
  LED_NONE,
  LED_RUNNING,
  LED_DISCONNECT_BLE,
};

// pin 5 on the RGB shield is button 1
// (button press will be shown on the iPhone app)
const uint32_t button = 0;

LedIndicator ledIndicator;


bool req_ble_report_f;

int count;
char buff[256];
// ----- Timer Setup -----//
hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);

  digitalWrite(LED_BUILTIN, ledIndicator.process());

  req_ble_report_f = true;

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
  
  // button press will be shown on the iPhone app)
  pinMode(button, INPUT);

  LoadPreference();
  printf("ap=%s\n", GetPreferenceAP());
  printf("pw=%s\n", GetPreferencePW());
  printf("ip=%s\n", GetPreferenceIP());

  setupBle();

  ledIndicator.SetBlinkCount(LED_DISCONNECT_BLE);

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
  timerAlarmWrite(timer, 200000, true);
  // Start an alarm
  timerAlarmEnable(timer);
  // ----- Start Timer -----//  
}

void loop()
{
  delay(10);
  loopBle();

  serialEvent();

  if (BleConnected()) {
    ledIndicator.SetBlinkCount(LED_RUNNING);
  }
  else {
    ledIndicator.SetBlinkCount(LED_DISCONNECT_BLE);
  }

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

uint8_t recv_cmd2[256];
uint8_t cmd2_index;
void serialEvent () {
  while(Serial2.available()) {
    recv_cmd2[cmd2_index] = (uint8_t)Serial2.read();
    if (recv_cmd2[cmd2_index] == 0x0A) {
      recv_cmd2[cmd2_index+1] = 0x00;

      SendSplit(recv_cmd2, cmd2_index+1, 20);
//      uint8_t sendCount = cmd2_index / 20 + 1;
//      for (uint8_t i=0; i<sendCount; i++) {
//        if (i < sendCount-1)
//          BleSend(recv_cmd2+(i*20), 20);
//        else
//          BleSend(recv_cmd2+(i*20), cmd2_index%20);
//      }
      Serial.printf("[U]%s\n", recv_cmd2);

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
