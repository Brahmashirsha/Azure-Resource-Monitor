📟 ESP32 System Monitor Dashboard

A colorful real-time system monitoring dashboard built with ESP32, TFT display, and Glances API.

Created by: Chaudhary Rajiv Gujjar

    🙈 If it works, I wrote it. If it doesn't... who knows?

🚀 Overview

This project connects an ESP32 to Wi-Fi and fetches system metrics from a remote server (using Glances) via REST API. The data is displayed on a 320x170 TFT screen in a clean, colorful, and rotating dashboard interface.

Metrics displayed:

    🧠 CPU Load

    💾 RAM Usage

    📦 Storage Utilization

    🌐 Network Traffic (KB/s)

🖼 Demo UI

The UI cycles through 5 different views:

    All metrics overview

    CPU view

    RAM view

    Storage view

    Network view

Each view shows dynamic gauges with real-time data pulled from the Glances API.
🧰 Hardware Required

    ESP32 board

    320x170 TFT Display (compatible with TFT_eSPI)

    Wi-Fi connection

    Remote server running Glances in web mode

🔌 Installation

    Set up Glances on your server (e.g., Azure VM):

pip install glances
glances -w

It should be accessible at: http://<server-ip>:61208/api/

Install required Arduino libraries:

    WiFi.h

    HTTPClient.h

    ArduinoJson

    TFT_eSPI (configured for your screen)

Edit the following fields in the code:

    const char* ssid = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";
    String base_url = "http://<your-server-ip>:61208/api";

    Upload to your ESP32 board using the Arduino IDE or PlatformIO.

⚙️ Configuration Tips

    Make sure your TFT screen is set up correctly in User_Setup.h (for TFT_eSPI).

    Adjust switchInterval to change the time between view transitions (default is 6000ms).

    You can also assign a button to manually switch views for better UX.

📄 File Structure
File/Section	Description
setup()	Connects to Wi-Fi and initializes TFT display.
loop()	Fetches metrics, handles view switching, and draws UI.
drawPanel() / drawFullPanel()	Handles drawing of metrics panels and full-screen views.
getCPU(), getRAM() etc.	HTTP GET requests to fetch JSON metrics.
📡 API Endpoints Used

These are the endpoints the ESP32 polls:

    /cpu → { "total": 23.4 }

    /mem → { "percent": 45.6 }

    /fs → [ { "percent": 70.1 } ]

    /network → { "tx": 123456, "rx": 654321 } (bytes)

🧠 Notes

    This project is designed for quick visual reference, not long-term logging or alerts.

    Recommended update interval: ≥3 seconds to avoid unnecessary API load.

    Add Wi-Fi reconnection logic for robustness in production use.

📜 License

MIT License — feel free to fork, modify, and build on top of this.
