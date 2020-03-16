// pin 2 on the Hiletgo
// (can be turned on/off from the iPhone app)
const uint32_t led = 2;

// pin 5 on the RGB shield is button 1
// (button press will be shown on the iPhone app)
const uint32_t button = 0;

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200);
  
  // led turned on/off from the iPhone app
  pinMode(led, OUTPUT);
  
  // button press will be shown on the iPhone app)
  pinMode(button, INPUT);
  
  setupBle();
}

void loop()
{
  delay(100);
  loopBle();
}
