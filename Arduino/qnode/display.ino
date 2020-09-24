#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_WIDTH    128
#define LOGO_HEIGHT   64
//static const unsigned char PROGMEM logo_bmp[] =
//{ B00000000, B11000000,
//  B00000001, B11000000,
//  B00000001, B11000000,
//  B00000011, B11100000,
//  B11110011, B11100000,
//  B11111110, B11111000,
//  B01111110, B11111111,
//  B00110011, B10011111,
//  B00011111, B11111100,
//  B00001101, B01110000,
//  B00011011, B10100000,
//  B00111111, B11100000,
//  B00111111, B11110000,
//  B01111100, B11110000,
//  B01110000, B01110000,
//  B00000000, B00110000 
//};
const unsigned char logo_bmp [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0x80, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0xC0, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0x80,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0x80,
0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xFF, 0xC0,
0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xC0,
0x00, 0x00, 0x00, 0x00, 0x0F, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xC0,
0x00, 0x00, 0x00, 0x01, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x80,
0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x80,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xF0, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE0, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00,
0x00, 0x3C, 0x00, 0xFF, 0xF8, 0x03, 0xFF, 0xDC, 0x00, 0xE1, 0xFF, 0xF0, 0x00, 0x1C, 0x00, 0x00,
0x00, 0x7C, 0x01, 0xFF, 0xFC, 0x0F, 0xFF, 0xDC, 0x00, 0xF3, 0xFF, 0xF8, 0x00, 0x38, 0x00, 0x00,
0x00, 0x7E, 0x03, 0xFF, 0xFE, 0x1F, 0xFF, 0xDC, 0x00, 0xF7, 0xFF, 0xF8, 0x00, 0x60, 0x00, 0x00,
0x00, 0xFE, 0x03, 0xFF, 0xFF, 0x3F, 0xFF, 0x9C, 0x00, 0xF7, 0xFF, 0xF0, 0x00, 0xC0, 0x00, 0x00,
0x00, 0xFF, 0x03, 0xC0, 0x0F, 0x7C, 0x00, 0x1C, 0x00, 0xFF, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00,
0x01, 0xEF, 0x03, 0xC0, 0x0F, 0x78, 0x00, 0x1C, 0x00, 0xFF, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
0x01, 0xE7, 0x83, 0xC0, 0x0F, 0x78, 0x00, 0x1C, 0x00, 0xFF, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00,
0x01, 0xC7, 0x83, 0xC0, 0x0F, 0x70, 0x00, 0x1C, 0x00, 0xF7, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00,
0x03, 0xC3, 0x83, 0xC0, 0x0F, 0x70, 0x00, 0x1C, 0x00, 0xF7, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x03, 0xC3, 0xC3, 0xC0, 0x0F, 0x70, 0x00, 0x1C, 0x00, 0xF3, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
0x07, 0x83, 0xC3, 0xCF, 0xFE, 0x70, 0x00, 0x1C, 0x00, 0xF1, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00,
0x07, 0x81, 0xE3, 0xDF, 0xFE, 0x70, 0x00, 0x1C, 0x00, 0xF0, 0x7F, 0xE0, 0x00, 0x00, 0x00, 0x00,
0x07, 0xBD, 0xE3, 0xDF, 0xFC, 0x70, 0x00, 0x1C, 0x00, 0xF0, 0x0F, 0xF8, 0x00, 0x00, 0x00, 0x00,
0x0F, 0xFF, 0xE3, 0xCF, 0xE0, 0x70, 0x00, 0x1C, 0x00, 0xF0, 0x01, 0xF8, 0x00, 0x00, 0x00, 0x00,
0x0F, 0xFF, 0xF3, 0xC7, 0xC0, 0x70, 0x00, 0x1C, 0x00, 0xF0, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00,
0x1F, 0xFF, 0xF3, 0xC3, 0xE0, 0x78, 0x00, 0x1C, 0x00, 0xF0, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00,
0x1C, 0x00, 0x7B, 0xC1, 0xF0, 0x78, 0x00, 0x1C, 0x00, 0xE0, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00,
0x1C, 0x00, 0x39, 0xC0, 0xF8, 0x3C, 0x00, 0x1E, 0x01, 0xE0, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00,
0x3C, 0x00, 0x39, 0xC0, 0x3C, 0x1F, 0xFF, 0xCF, 0xC7, 0xC6, 0xE0, 0xF8, 0x00, 0x00, 0x00, 0x00,
0x38, 0x00, 0x3D, 0xC0, 0x1E, 0x0F, 0xFF, 0xC7, 0xFF, 0x87, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00,
0x78, 0x00, 0x1F, 0xC0, 0x0F, 0x83, 0xFF, 0xC1, 0xFF, 0x07, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFC, 0x00, 0x60, 0x00, 0x03, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x60, 0x00, 0x03, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0xCF, 0xFE, 0xFC, 0xFB, 0x7E, 0x7F, 0x0C,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCF, 0xDF, 0x7E, 0xFD, 0xFF, 0x7E, 0xFF, 0x8C,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCC, 0x70, 0x63, 0xC7, 0x0F, 0xC3, 0xC7, 0x98,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCF, 0xF0, 0x63, 0xC7, 0x0F, 0xC3, 0xC6, 0x98,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCF, 0xF0, 0x63, 0xC7, 0x0F, 0xC3, 0xC6, 0xD0,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCC, 0x10, 0x63, 0xC7, 0x0F, 0xC3, 0xC6, 0xF0,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCF, 0xDF, 0xE3, 0xC5, 0xFF, 0x7E, 0xFE, 0x60,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0xCF, 0xE2, 0x84, 0xFB, 0x3C, 0x7E, 0x60,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x60,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xC0,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


void setupDisplay() {
  // Init Display
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
//    for(;;); // Don't proceed, loop forever
  }
//  display.setFont(&FreeMono9pt7b);
  display.setFont(&FreeSans9pt7b);
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();

  display.clearDisplay();
  display.drawBitmap(0,0,logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();

  delay(1000);
}

void loopDisplay() {
//  Serial.println("loopDisplay");

//  displayInfo();
}

void displayPreProcess() {
  display.clearDisplay();     // Clear display buffer
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
}
void displayPostProcess() {
  display.display();
}

void displayLines(const char *line1, const char *line2, const char *line3, const char *line4) {
  display.clearDisplay();     // Clear display buffer
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  displayLine(1, line1);
  displayLine(2, line2);
  displayLine(3, line3);
  displayLine(4, line4);
  display.display();
}

char tmpLine[256];
void displayInfo() {
  display.clearDisplay();     // Clear display buffer
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  sprintf(tmpLine, "[O]AP:OAS");
  displayLine(1, (char *)GetDeviceName(""));
  displayLine(2, "line22");
//  sprintf(tmpLine, "C=%1.4f", curqa);
  displayLine(3, tmpLine);
//  float sdUsed = SD.usedBytes();
//  float sdTotal = SD.totalBytes();
//  sprintf(tmpLine, "[SD]%0.1f/%0.1fG", sdUsed / (1024*1024*1024), sdTotal / (1024 * 1024 * 1024));
  displayLine(4, tmpLine);
  display.display();
}
void displayLine(int line_no, const char *line) {
  display.setCursor(0, line_no*15);     // Start at top-left corner
  display.print(line);
}
