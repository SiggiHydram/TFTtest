#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"

// Pin definitions for ESP32
#define TFT_CS    5   // Chip Select
#define TFT_DC    15   // Data/Command
#define TFT_RST   4   // Reset (optional, can be -1 if not used)
#define TFT_MOSI  2  // SDA/MOSI - ESP32 default SPI MOSI
#define TFT_SCLK  14  // SCL/SCK  - ESP32 default SPI SCLK
#define TFT_MISO  -1  // MISO (not usually needed for TFT, but ESP32 default)

// Constructor: Adafruit_GC9A01A(CS, DC, MOSI, SCLK, RST, MISO)
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST, TFT_MISO);

// Sensor configuration
struct Sensor {
  const char* name;
  float value;
  float minVal;
  float maxVal;
  const char* unit;
  uint16_t color;
};

Sensor sensors[3] = {
  {"Temperature", 0, 0, 100, "C", GC9A01A_RED},
  {"Humidity", 0, 0, 100, "%", GC9A01A_BLUE},
  {"Pressure", 0, 900, 1100, "hPa", GC9A01A_GREEN}
};

int currentSensor = 0;
unsigned long lastSwitchTime = 0;
const unsigned long SWITCH_INTERVAL = 7000; // 7 seconds per sensor

// Display constants
const int GAUGE_CENTER_X = 120;
const int GAUGE_CENTER_Y = 120;
const int GAUGE_RADIUS = 100;
const int NEEDLE_LENGTH = 80;

void drawGauge(int sensorIndex) {
  Sensor& sensor = sensors[sensorIndex];
  
  // Clear screen
  tft.fillScreen(GC9A01A_BLACK);
  
  // Draw outer circle
  tft.drawCircle(GAUGE_CENTER_X, GAUGE_CENTER_Y, GAUGE_RADIUS, GC9A01A_WHITE);
  tft.drawCircle(GAUGE_CENTER_X, GAUGE_CENTER_Y, GAUGE_RADIUS-1, GC9A01A_WHITE);
  
  // Draw scale marks
  for(int i = 0; i <= 10; i++) {
    float angle = 225 - (i * 27); // 270 degrees total arc, starting from 225°
    float radians = angle * PI / 180;
    
    int x1 = GAUGE_CENTER_X + (GAUGE_RADIUS - 10) * cos(radians);
    int y1 = GAUGE_CENTER_Y + (GAUGE_RADIUS - 10) * sin(radians);
    int x2 = GAUGE_CENTER_X + (GAUGE_RADIUS - 20) * cos(radians);
    int y2 = GAUGE_CENTER_Y + (GAUGE_RADIUS - 20) * sin(radians);
    
    tft.drawLine(x1, y1, x2, y2, GC9A01A_WHITE);
  }
  
  // Calculate needle position
  float percentage = (sensor.value - sensor.minVal) / (sensor.maxVal - sensor.minVal);
  percentage = constrain(percentage, 0, 1);
  float needleAngle = 225 - (percentage * 270); // 270 degrees total arc
  float needleRadians = needleAngle * PI / 180;
  
  int needleX = GAUGE_CENTER_X + NEEDLE_LENGTH * cos(needleRadians);
  int needleY = GAUGE_CENTER_Y + NEEDLE_LENGTH * sin(needleRadians);
  
  // Draw needle
  tft.drawLine(GAUGE_CENTER_X, GAUGE_CENTER_Y, needleX, needleY, sensor.color);
  tft.drawLine(GAUGE_CENTER_X-1, GAUGE_CENTER_Y, needleX-1, needleY, sensor.color);
  tft.drawLine(GAUGE_CENTER_X+1, GAUGE_CENTER_Y, needleX+1, needleY, sensor.color);
  
  // Draw center dot
  tft.fillCircle(GAUGE_CENTER_X, GAUGE_CENTER_Y, 5, sensor.color);
  
  // Display sensor name
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(2);
  int textWidth = strlen(sensor.name) * 12; // Approximate width
  tft.setCursor((240 - textWidth) / 2, 80);
  tft.println(sensor.name);
  
  // Display current value
  char valueStr[20];
  sprintf(valueStr, "%.1f %s", sensor.value, sensor.unit);
  tft.setTextColor(sensor.color);
  tft.setTextSize(2);
  textWidth = strlen(valueStr) * 12; // Approximate width for size 2
  tft.setCursor((240 - textWidth) / 2, 160);
  tft.println(valueStr);
  
  // Display min/max range
  char rangeStr[30];
  sprintf(rangeStr, "%.0f - %.0f %s", sensor.minVal, sensor.maxVal, sensor.unit);
  tft.setTextColor(GC9A01A_CYAN);
  tft.setTextSize(1);
  textWidth = strlen(rangeStr) * 6; // Approximate width for size 1
  tft.setCursor((240 - textWidth) / 2, 220);
  tft.println(rangeStr);
}

void updateSensorValues() {
  // Generate random sensor values (replace with real sensor readings)
  sensors[0].value = random(150, 350) / 10.0; // Temperature: 15.0 - 35.0°C
  sensors[1].value = random(300, 800) / 10.0; // Humidity: 30.0 - 80.0%
  sensors[2].value = random(9800, 10200) / 10.0; // Pressure: 980.0 - 1020.0 hPa
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10); // Wait for serial port to connect
  
  Serial.println("Multi-Sensor Display Starting...");
  Serial.println("Pin Configuration:");
  Serial.printf("CS: %d, DC: %d, RST: %d\n", TFT_CS, TFT_DC, TFT_RST);
  Serial.printf("MOSI: %d, SCLK: %d, MISO: %d\n", TFT_MOSI, TFT_SCLK, TFT_MISO);
  
  Serial.println("Initializing display...");
  tft.begin();
  tft.setRotation(0); // Set to portrait mode
  Serial.println("Display initialized successfully!");
  
  // Initialize random seed
  randomSeed(analogRead(0));
  
  // Update initial sensor values
  updateSensorValues();
  
  // Draw initial gauge
  drawGauge(currentSensor);
  lastSwitchTime = millis();
  
  Serial.println("Sensor display ready!");
  Serial.println("Cycling through: Temperature -> Humidity -> Pressure");
}

void loop(void) {
  unsigned long currentTime = millis();
  
  // Update sensor values every 500ms
  static unsigned long lastUpdateTime = 0;
  if (currentTime - lastUpdateTime >= 5000) {
    updateSensorValues();
    lastUpdateTime = currentTime;
    
    // Print current sensor values to serial
    Serial.printf("%s: %.1f%s, %s: %.1f%s, %s: %.1f%s\n",
                  sensors[0].name, sensors[0].value, sensors[0].unit,
                  sensors[1].name, sensors[1].value, sensors[1].unit,
                  sensors[2].name, sensors[2].value, sensors[2].unit);
  }
  
  // Switch sensor display every SWITCH_INTERVAL
  if (currentTime - lastSwitchTime >= SWITCH_INTERVAL) {
    currentSensor = (currentSensor + 1) % 3; // Cycle through 0, 1, 2
    drawGauge(currentSensor);
    lastSwitchTime = currentTime;
    
    Serial.printf("Switched to sensor %d: %s\n", currentSensor, sensors[currentSensor].name);
  }
  
  // Redraw current gauge with updated values (smooth needle movement)
  static unsigned long lastRedrawTime = 0;
  if (currentTime - lastRedrawTime >= 6900) { // Redraw every 5000ms for smooth updates
    drawGauge(currentSensor);
    lastRedrawTime = currentTime;
  }
}