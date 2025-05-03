
//  🔥 WARNING: Code by Chaudhary Rajiv Gujjar  ⚡     

//  🙈 If it works, I wrote it. If it doesn't... who knows?     

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 170

#define BACKGROUND_COLOR 0x0000
#define PANEL_BG_COLOR 0x0001
#define PANEL_BORDER 0x4A69
#define CPU_COLOR 0xFA20
#define RAM_COLOR 0x04F9
#define STORAGE_COLOR 0x7D3C
#define NETWORK_COLOR 0xFFE0
#define TEXT_COLOR 0xFFFF
#define SUBTEXT_COLOR 0xAD55
#define GAUGE_BG_COLOR 0x1082

#define PIN_LCD_BL 38  // LCD backlight pin on T-Display-S3

const char* ssid = ""; //your wifi ssid
const char* password = "";//your wifi password
String base_url = ""; //  glances api url

const int BTN_PREV = 0;     // BOOT
const int BTN_NEXT = 14;    // IO14

int currentView = 0;
unsigned long lastUpdate = 0;
const int updateInterval = 2000;

float cpu, ram, storage, net;
float prevCpu = -1, prevRam = -1, prevStorage = -1, prevNet = -1; // Initialize with invalid values

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  // BACKLIGHT PWM SETUP
  ledcSetup(0, 10000, 8);
  ledcAttachPin(PIN_LCD_BL, 0);
  ledcWrite(0, 128);  // 50% brightness

  WiFi.begin(ssid, password);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BACKGROUND_COLOR);
  tft.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
  tft.setTextFont(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("Connecting to WiFi...", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  tft.fillScreen(BACKGROUND_COLOR);
  tft.drawString("Connected to " + String(ssid), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
  delay(1000);
}

void loop() {
  static bool needRedraw = true;
  static bool needPartialUpdate = false;

  static unsigned long nextBtnPressStart = 0;
  static bool nextBtnHandled = false;
  static unsigned long prevBtnPress = 0;

  bool nextPressed = digitalRead(BTN_NEXT) == LOW;
  bool prevPressed = digitalRead(BTN_PREV) == LOW;

  // BTN_NEXT logic with long-press detection
  if (nextPressed && nextBtnPressStart == 0) {
    nextBtnPressStart = millis();
    nextBtnHandled = false;
  } else if (!nextPressed && nextBtnPressStart > 0) {
    unsigned long pressDuration = millis() - nextBtnPressStart;

    if (pressDuration < 1000 && !nextBtnHandled) {
      currentView = (currentView + 1) % 6;
      needRedraw = true;
    }

    nextBtnPressStart = 0;
    nextBtnHandled = false;
  }

  if (currentView == 5 && nextPressed && nextBtnPressStart > 0) {
    if (!nextBtnHandled && millis() - nextBtnPressStart > 2000) {
      nextBtnHandled = true;
      tft.fillScreen(BACKGROUND_COLOR);
      tft.setTextFont(4);
      tft.setTextColor(TEXT_COLOR);
      tft.setTextDatum(TC_DATUM);
      tft.drawString("Shutting down...", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
      delay(1000);
      esp_deep_sleep_start();
    }
  }

  // BTN_PREV
  if (prevPressed && millis() - prevBtnPress > 300) {
    currentView = (currentView + 5) % 6;
    needRedraw = true;
    prevBtnPress = millis();
  }

  if (millis() - lastUpdate > updateInterval) {
    cpu = getCPU();
    ram = getRAM();
    storage = getStorage();
    net = getNetworkKB();
    
    // Only update if values changed or we need a full redraw
    if (cpu != prevCpu || ram != prevRam || storage != prevStorage || net != prevNet || needRedraw) {
      if (currentView == 0) {
        needPartialUpdate = true;
      } else {
        needRedraw = true;
      }
    }
    
    prevCpu = cpu;
    prevRam = ram;
    prevStorage = storage;
    prevNet = net;
    lastUpdate = millis();
  }

  if (needRedraw) {
    tft.fillScreen(BACKGROUND_COLOR);
    switch (currentView) {
      case 0: drawAll(cpu, ram, storage, net); break;
      case 1: drawFullPanel("CPU LOAD", CPU_COLOR, cpu, "Temp: 55C"); break;
      case 2: drawFullPanel("RAM USAGE", RAM_COLOR, ram, "Free: " + String(100 - (int)ram) + "%"); break;
      case 3: drawFullPanel("STORAGE", STORAGE_COLOR, storage, "Free: " + String(100 - (int)storage) + "%"); break;
      case 4: drawFullPanel("NETWORK", NETWORK_COLOR, net, String(net, 1) + " KB/s"); break;
      case 5: drawPowerOffScreen(); break;
    }
    needRedraw = false;
  }
  else if (needPartialUpdate && currentView == 0) {
    updateDashboardValues(cpu, ram, storage, net);
    needPartialUpdate = false;
  }
}

void drawPowerOffScreen() {
  tft.setTextDatum(TC_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(TEXT_COLOR);
  tft.drawString("Hold to Power Off", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 10);
  tft.setTextFont(2);
  tft.setTextColor(SUBTEXT_COLOR);
  tft.drawString("Hold IO14 for 2 sec", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20);
}

void drawAll(float cpu, float ram, float storage, float net) {
  drawPanel(10, 10, "CPU", "Load", String((int)cpu) + "%", CPU_COLOR, map(cpu, 0, 100, 0, 360));
  drawPanel(165, 10, "RAM", "Usage", String((int)ram) + "%", RAM_COLOR, map(ram, 0, 100, 0, 360));
  drawPanel(10, 90, "STORAGE", "Used", String((int)storage) + "%", STORAGE_COLOR, map(storage, 0, 100, 0, 360));
  drawPanel(165, 90, "NET", "KB/s", String(net, 1), NETWORK_COLOR, map(constrain(net, 0, 1000), 0, 1000, 0, 360));
}

void drawPanel(int x, int y, String title, String label, String value, uint16_t color, int arcAngle) {
  const int width = 145, height = 78, arcSize = 30;

  tft.fillRoundRect(x + 2, y + 2, width, height, 6, GAUGE_BG_COLOR);
  tft.fillRoundRect(x, y, width, height, 6, PANEL_BG_COLOR);
  tft.drawRoundRect(x, y, width, height, 6, color);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(color);    tft.setTextFont(2); tft.drawString(title, x + 8, y + 6);
  tft.setTextColor(SUBTEXT_COLOR); tft.drawString(label, x + 8, y + 24);
  tft.setTextColor(TEXT_COLOR);    tft.setTextFont(4); tft.drawString(value, x + 8, y + 42);

  drawArcGauge(x + width - arcSize - 8, y + (height - arcSize) / 2, color, arcAngle, arcSize);
}

void updateDashboardValues(float cpu, float ram, float storage, float net) {
  // this will Only update the changing parts of each panel
  updatePanelValue(10, 10, String((int)cpu) + "%", CPU_COLOR, map(cpu, 0, 100, 0, 360));
  updatePanelValue(165, 10, String((int)ram) + "%", RAM_COLOR, map(ram, 0, 100, 0, 360));
  updatePanelValue(10, 90, String((int)storage) + "%", STORAGE_COLOR, map(storage, 0, 100, 0, 360));
  updatePanelValue(165, 90, String(net, 1), NETWORK_COLOR, map(constrain(net, 0, 1000), 0, 1000, 0, 360));
}

void updatePanelValue(int x, int y, String value, uint16_t color, int arcAngle) {
  const int width = 145, height = 78, arcSize = 30;
  
  // Clear only the value area
  tft.fillRect(x + 8, y + 42, 60, 20, PANEL_BG_COLOR);
  tft.setTextColor(TEXT_COLOR);
  tft.setTextFont(4);
  tft.drawString(value, x + 8, y + 42);
  
  // Update the arc gauge
  int centerX = x + width - arcSize/2 - 8;
  int centerY = y + height/2;
  
  // Clear previous arc
  tft.drawSmoothArc(centerX, centerY, arcSize/2, arcSize/2 - 6, 0, 360, GAUGE_BG_COLOR, PANEL_BG_COLOR, true);
  
  // Draw new arc
  for (int i = 0; i < arcAngle; i += 6) {
    int endAngle = min(i + 6, arcAngle);
    uint16_t segmentColor = tft.alphaBlend(map(i, 0, arcAngle, 100, 255), color, GAUGE_BG_COLOR);
    tft.drawSmoothArc(centerX, centerY, arcSize/2, arcSize/2 - 6, i, endAngle, segmentColor, PANEL_BG_COLOR, true);
  }
  
  tft.fillCircle(centerX, centerY, 2, color);
}

void drawFullPanel(String title, uint16_t color, float value, String subinfo) {
  tft.setTextDatum(TC_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(color);
  tft.drawString(title, SCREEN_WIDTH / 2, 14);

  drawArcGauge((SCREEN_WIDTH - 90) / 2, 30, color, map(value, 0, 100, 0, 360), 90);

  tft.setTextFont(2);
  tft.setTextColor(TEXT_COLOR);
  tft.drawString("Usage: " + String((int)value) + "%", SCREEN_WIDTH / 2, 125);
  tft.drawString(subinfo, SCREEN_WIDTH / 2, 145);
}

void drawArcGauge(int x, int y, uint16_t color, int angle, int size) {
  int centerX = x + size / 2;
  int centerY = y + size / 2;
  int radius = size / 2;
  int thickness = 6;

  tft.drawSmoothArc(centerX, centerY, radius, radius - thickness, 0, 360, GAUGE_BG_COLOR, BACKGROUND_COLOR, true);

  for (int i = 0; i < angle; i += 6) {
    int endAngle = min(i + 6, angle);
    uint16_t segmentColor = tft.alphaBlend(map(i, 0, angle, 100, 255), color, GAUGE_BG_COLOR);
    tft.drawSmoothArc(centerX, centerY, radius, radius - thickness, i, endAngle, segmentColor, BACKGROUND_COLOR, true);
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
    String payload = http.getString();
    Serial.println("Network JSON: " + payload);  // Debug print

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // Loop through the interfaces in the JSON array
      for (JsonObject iface : doc.as<JsonArray>()) {
        const char* name = iface["interface_name"];
        Serial.println("Interface: " + String(name));  // Debug print

        if (String(name) == "eth0") {
          // Explicitly cast to float
          float rx = (float)iface["bytes_recv_rate_per_sec"];
          float tx = (float)iface["bytes_sent_rate_per_sec"];

          // Directly print the raw rx and tx values
          Serial.println("Raw RX: " + String(rx));
          Serial.println("Raw TX: " + String(tx));

          // Now apply the conversion and print the result
          totalKB = (rx + tx) / 1024.0;  // Convert to KB/s

          // Print the final total KB/s value
          Serial.println("Total KB/s: " + String(totalKB));
          break;  // Exit the loop after processing eth0
        }
      }
    } else {
      Serial.println("JSON deserialization error: " + String(error.c_str()));
    }
  } else {
    Serial.println("HTTP GET failed with code: " + String(code));
  }

  http.end();
  return totalKB;
}
// End of the code
