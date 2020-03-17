#ifndef LedIndicator_h
#define LedIndicator_h


class LedIndicator {
  public:
    void SetBlinkCount(int count);
    bool process();
  
  private:
    int led_blinking_target;
    bool led_blinking_status;
    bool led_blinking_level;
    int led_blinking_count;
    int led_pause_target = 8;
    int led_pause_count;
};

#endif
