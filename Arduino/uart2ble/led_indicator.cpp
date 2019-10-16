#include "led_indicator.h"

void LedIndicator::SetBlinkCount(int count) {
  led_blinking_target = count;
}

bool LedIndicator::process() {
  if (led_blinking_status) {
    if (led_blinking_count < led_blinking_target*2) {
      led_blinking_level = !led_blinking_level;
      led_blinking_count++;
      return !led_blinking_level;
    }
    else {
      led_pause_count = 0;
      led_blinking_count = 0;
      led_blinking_status = false;
      return false;
    }
  }
  else {
    if (led_pause_count < led_pause_target) {
      led_pause_count++;
    }
    else {
      led_pause_count = 0;
      led_blinking_count = 0;
      led_blinking_status = true;
    }
  }
}
