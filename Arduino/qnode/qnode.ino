#include "led_indicator.h"

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

// ----- Timer Setup -----//
hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);

  digitalWrite(LED_BUILTIN, ledIndicator.process());

  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}
// ----- Timer Setup -----//

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200);
  
  // led turned on/off from the iPhone app
  pinMode(LED_BUILTIN, OUTPUT);
  
  // button press will be shown on the iPhone app)
  pinMode(button, INPUT);

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
}


uint8_t recv_cmd2[256];
uint8_t cmd2_index;
void serialEvent () {
  while(Serial2.available()) {
    recv_cmd2[cmd2_index] = (uint8_t)Serial2.read();
    if (recv_cmd2[cmd2_index] == 0x0A) {
      recv_cmd2[cmd2_index+1] = 0x00;

      uint8_t sendCount = cmd2_index / 20 + 1;
      for (uint8_t i=0; i<sendCount; i++) {
        if (i < sendCount-1)
          BleSend(recv_cmd2+(i*20), 20);
        else
          BleSend(recv_cmd2+(i*20), cmd2_index%20);
      }
      Serial.printf("[U]%s\n", recv_cmd2);

      cmd2_index = 0;
      continue;
    }
    cmd2_index++;
  }
}
