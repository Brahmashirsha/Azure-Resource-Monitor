
//  🔥 WARNING: Code by Chaudhary Rajiv Gujjar  ⚡     

//  🙈 If it works, I wrote it. If it doesn't... who knows?     

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 170

#define BACKGROUND_COLOR 0x18E0
#define PANEL_BG_COLOR 0x2966
#define CPU_COLOR 0xFD20
#define RAM_COLOR 0x3666
#define STORAGE_COLOR 0x7D3C
#define NETWORK_COLOR 0xFF80
#define TEXT_COLOR 0xFFFF
#define SUBTEXT_COLOR 0xAD55
#define GAUGE_BG_COLOR 0x2104

const char* ssid = "test";
const char* password = "test1234";

// First setup glances on you azure VM easier than prometheus

// only enter glances url not prometheus

String base_url = "Enter your api url ";

// You can change this interval but recommended >3
// or just assign a GPIO pin/button

int currentView = 0;
unsigned long lastSwitch = 0;
const int switchInterval = 6000;

// Lcd setup 
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BACKGROUND_COLOR);
  tft.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
  tft.setTextFont(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("Connecting to WiFi...", SCREEN_WIDTH/2, SCREEN_HEIGHT/2);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  tft.fillScreen(BACKGROUND_COLOR);
  tft.drawString("Connected to " + String(ssid), SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
  delay(1000);
}

void loop() {
  float cpu = getCPU();
  float ram = getRAM();
  float storage = getStorage();
  float net = getNetworkKB();

  tft.fillScreen(BACKGROUND_COLOR);

  if (millis() - lastSwitch > switchInterval) {
    currentView = (currentView + 1) % 5;
    lastSwitch = millis();
  }

  if (currentView == 0) drawAll(cpu, ram, storage, net);
  else if (currentView == 1) drawCpuView(cpu);
  else if (currentView == 2) drawRamView(ram);
  else if (currentView == 3) drawStorageView(storage);
  else if (currentView == 4) drawNetworkView(net);

  delay(500);
}

void drawAll(float cpu, float ram, float storage, float net) {
  drawPanel(10, 10, "CPU", "Load", String((int)cpu) + "%", CPU_COLOR, map(cpu, 0, 100, 0, 360));
  drawPanel(165, 10, "RAM", "Usage", String((int)ram) + "%", RAM_COLOR, map(ram, 0, 100, 0, 360));
  drawPanel(10, 90, "STORAGE", "Used", String((int)storage) + "%", STORAGE_COLOR, map(storage, 0, 100, 0, 360));
  drawPanel(165, 90, "NET", "KB/s", String(net, 1), NETWORK_COLOR, map(constrain(net, 0, 1000), 0, 1000, 0, 360));
}

void drawCpuView(float cpu) {
  drawFullPanel("CPU LOAD", CPU_COLOR, cpu, "Temp: 55C");
}

void drawRamView(float ram) {
  drawFullPanel("RAM USAGE", RAM_COLOR, ram, "Free: " + String(100 - (int)ram) + "%");
}

void drawStorageView(float storage) {
  drawFullPanel("STORAGE", STORAGE_COLOR, storage, "Free: " + String(100 - (int)storage) + "%");
}

void drawNetworkView(float net) {
  drawFullPanel("NETWORK", NETWORK_COLOR, net, "KB/s");
}

void drawPanel(int x, int y, String title, String label, String value, uint16_t color, int arcAngle) {
  const int width = 145;
  const int height = 78;
  const int arcSize = 32;

  tft.fillRoundRect(x + 1, y + 1, width, height, 6, 0x1082);
  tft.fillRoundRect(x, y, width, height, 6, PANEL_BG_COLOR);
  tft.drawRoundRect(x, y, width, height, 6, color);

  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(color);
  tft.drawString(title, x + 8, y + 6);

  tft.setTextColor(SUBTEXT_COLOR);
  tft.drawString(label, x + 8, y + 24);

  tft.setTextColor(TEXT_COLOR);
  tft.setTextFont(4);
  tft.drawString(value, x + 8, y + 40);

  drawArcGauge(x + width - arcSize - 8, y + (height - arcSize) / 2, color, arcAngle, arcSize);
}

void drawFullPanel(String title, uint16_t color, float value, String subinfo) {
  tft.setTextDatum(TC_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(color);
  tft.drawString(title, SCREEN_WIDTH/2, 10);

  drawArcGauge((SCREEN_WIDTH - 80) / 2, 30, color, map(value, 0, 100, 0, 360), 80);

  tft.setTextFont(2);
  tft.setTextColor(TEXT_COLOR);
  tft.drawString("Usage: " + String((int)value) + "%", SCREEN_WIDTH/2, 120);
  tft.drawString(subinfo, SCREEN_WIDTH/2, 140);
}

void drawArcGauge(int x, int y, uint16_t color, int angle, int size) {
  int centerX = x + size / 2;
  int centerY = y + size / 2;
  int radius = size / 2;
  int thickness = 5;

  tft.drawSmoothArc(centerX, centerY, radius, radius - thickness, 0, 360, GAUGE_BG_COLOR, PANEL_BG_COLOR, true);

  for (int i = 0; i < angle; i += 6) {
    int endAngle = min(i + 6, angle);
    uint16_t segmentColor = tft.alphaBlend(map(i, 0, angle, 128, 255), color, GAUGE_BG_COLOR);
    tft.drawSmoothArc(centerX, centerY, radius, radius - thickness, i, endAngle, segmentColor, PANEL_BG_COLOR, true);
  }

  tft.fillCircle(centerX, centerY, 2, color);
}

float getCPU() {
  HTTPClient http;
  http.begin(base_url + "/cpu");
  int code = http.GET();
  float val = 0;
  if (code == 200) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getString());
    val = doc["total"];
  }
  http.end();
  return val;
}

float getRAM() {
  HTTPClient http;
  http.begin(base_url + "/mem");
  int code = http.GET();
  float val = 0;
  if (code == 200) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getString());
    val = doc["percent"];
  }
  http.end();
  return val;
}

float getStorage() {
  HTTPClient http;
  http.begin(base_url + "/fs");
  int code = http.GET();
  float val = 0;
  if (code == 200) {
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, http.getString());
    val = doc[0]["percent"];
  }
  http.end();
  return val;
}

float getNetworkKB() {
  HTTPClient http;
  http.begin(base_url + "/network");
  int code = http.GET();
  float totalKB = 0;
  if (code == 200) {
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, http.getString());
    float tx = doc["tx"];
    float rx = doc["rx"];
    totalKB = (tx + rx) / 1024.0;
  }
  http.end();
  return totalKB;
}
// End of the code
