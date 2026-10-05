#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <time.h>

// =====================================================
// VERSION INFORMATION
// =====================================================
const char* VERSION = "1.0.0";
const int BUILD_NUMBER = 1;

// =====================================================
// Wi-Fi Credentials
// =====================================================
const char* ssid = "301 TECH";
const char* password = "qwerty301!!!";

// =====================================================
// Pin Definitions
// =====================================================
const int LED_PIN = 2;

// =====================================================
// Objects
// =====================================================
Preferences preferences;
WiFiServer server(80);

// =====================================================
// Variables
// =====================================================
unsigned long ledOnCount = 0;
bool lastLedState = false;

// =====================================================
// Date and Time Configuration
// Philippines = UTC+8
// =====================================================
const long GMT_OFFSET_SEC = 8 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0;

// =====================================================
// Get current date and time
// =====================================================
String getDateTime() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    return "Time not available";
  }

  char dateTime[30];

  strftime(
    dateTime,
    sizeof(dateTime),
    "%Y-%m-%d %H:%M:%S",
    &timeinfo
  );

  return String(dateTime);
}

// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // ===================================================
  // 1. Initialize Preferences
  // ===================================================
  preferences.begin("led-counter", false);

  // If the ESP32 has been completely erased,
  // the counter will start at 50.
  ledOnCount = preferences.getULong("counter", 50);

  Serial.println();
  Serial.println("=================================");
  Serial.println("ESP32 LED COUNTER");
  Serial.println("=================================");
  Serial.print("Version: ");
  Serial.println(VERSION);

  Serial.print("Build: ");
  Serial.println(BUILD_NUMBER);

  Serial.print("Counter loaded: ");
  Serial.println(ledOnCount);

  // ===================================================
  // 2. Connect to Wi-Fi
  // ===================================================
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi connected.");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // ===================================================
  // 3. Configure Philippine Time
  // ===================================================
  configTime(
    GMT_OFFSET_SEC,
    DAYLIGHT_OFFSET_SEC,
    "pool.ntp.org",
    "time.nist.gov"
  );

  Serial.println("Waiting for time synchronization...");

  struct tm timeinfo;

  if (getLocalTime(&timeinfo)) {
    Serial.println("Time synchronized.");
    Serial.println(getDateTime());
  } else {
    Serial.println("Time synchronization failed.");
  }

  // ===================================================
  // 4. Setup Arduino OTA
  // ===================================================
  ArduinoOTA.setHostname("esp32-led-counter");

  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA Update");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA Update");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {

    Serial.printf(
      "Progress: %u%%\r",
      (progress / (total / 100))
    );

  });

  ArduinoOTA.onError([](ota_error_t error) {

    Serial.printf("Error[%u]: ", error);

    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");

    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");

    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");

    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");

    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  // ===================================================
  // 5. Start Web Server
  // ===================================================
  server.begin();

  Serial.println("Web server started.");

  Serial.println();
  Serial.println("=================================");
  Serial.println("SYSTEM READY");
  Serial.println("=================================");
}

// =====================================================
// LOOP
// =====================================================
void loop() {

  // ===================================================
  // Handle OTA
  // ===================================================
  ArduinoOTA.handle();

  // ===================================================
  // VERSION 1
  // LED blinks every 1 second
  // ===================================================
  static unsigned long lastToggleTime = 0;

  if (millis() - lastToggleTime >= 1000) {

    lastToggleTime = millis();

    bool currentState = digitalRead(LED_PIN);

    digitalWrite(LED_PIN, !currentState);
  }

  // ===================================================
  // LED COUNTER
  // ===================================================
  bool currentLedState = digitalRead(LED_PIN);

  // Detect OFF → ON
  if (currentLedState && !lastLedState) {

    ledOnCount++;

    // Save counter to ESP32 flash
    preferences.putULong("counter", ledOnCount);

    Serial.print("LED turned ON! Total activations: ");
    Serial.println(ledOnCount);
  }

  lastLedState = currentLedState;

  // ===================================================
  // WEB DASHBOARD
  // ===================================================
  WiFiClient client = server.available();

  if (client) {

    String currentLine = "";
    String requestString = "";

    while (client.connected()) {

      if (client.available()) {

        char c = client.read();

        requestString += c;

        if (c == '\n') {

          if (currentLine.length() == 0) {

            // =================================================
            // RESET COUNTER
            // =================================================
            if (requestString.indexOf("GET /reset") != -1) {

              ledOnCount = 50;

              preferences.putULong(
                "counter",
                ledOnCount
              );

              Serial.println(
                "Counter reset to 50 via dashboard!"
              );
            }

            // =================================================
            // HTTP RESPONSE
            // =================================================
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // =================================================
            // DASHBOARD HTML
            // =================================================
            client.println("<!DOCTYPE html>");
            client.println("<html>");

            client.println("<head>");

            client.println(
              "<meta name=\"viewport\" "
              "content=\"width=device-width, initial-scale=1\">"
            );

            client.println(
              "<meta http-equiv=\"refresh\" content=\"5\">"
            );

            client.println(
              "<title>ESP32 LED Dashboard</title>"
            );

            // =================================================
            // CSS
            // =================================================
            client.println("<style>");

            client.println(
              "body {"
              "font-family: Arial, sans-serif;"
              "text-align: center;"
              "margin-top: 40px;"
              "background-color: #f4f4f9;"
              "}"
            );

            client.println(
              ".card {"
              "background: white;"
              "padding: 30px;"
              "border-radius: 15px;"
              "box-shadow: 0px 4px 15px rgba(0,0,0,0.1);"
              "display: inline-block;"
              "min-width: 300px;"
              "}"
            );

            client.println(
              "h1 {"
              "color: #333;"
              "}"
            );

            client.println(
              ".counter {"
              "font-size: 60px;"
              "color: #007bff;"
              "font-weight: bold;"
              "margin: 20px 0;"
              "}"
            );

            client.println(
              ".info {"
              "font-size: 18px;"
              "margin: 10px;"
              "color: #444;"
              "}"
            );

            client.println(
              ".label {"
              "font-weight: bold;"
              "}"
            );

            client.println(
              ".btn {"
              "padding: 10px 20px;"
              "font-size: 16px;"
              "margin: 5px;"
              "cursor: pointer;"
              "border: none;"
              "border-radius: 5px;"
              "}"
            );

            client.println(
              ".btn-refresh {"
              "background-color: #007bff;"
              "color: white;"
              "}"
            );

            client.println(
              ".btn-reset {"
              "background-color: #dc3545;"
              "color: white;"
              "}"
            );

            client.println("</style>");

            client.println("</head>");

            // =================================================
            // BODY
            // =================================================
            client.println("<body>");

            client.println("<div class=\"card\">");

            client.println(
              "<h1>ESP32 LED Dashboard</h1>"
            );

            // =================================================
            // VERSION
            // =================================================
            client.println(
              "<div class=\"info\">"
              "<span class=\"label\">Version:</span> "
              + String(VERSION) +
              "</div>"
            );

            // =================================================
            // BUILD
            // =================================================
            client.println(
              "<div class=\"info\">"
              "<span class=\"label\">Build:</span> "
              + String(BUILD_NUMBER) +
              "</div>"
            );

            // =================================================
            // DATE AND TIME
            // =================================================
            client.println(
              "<div class=\"info\">"
              "<span class=\"label\">Date / Time:</span><br>"
              + getDateTime() +
              "</div>"
            );

            // =================================================
            // COUNTER
            // =================================================
            client.println(
              "<p>Total times LED turned ON:</p>"
            );

            client.print(
              "<div class=\"counter\">"
            );

            client.print(ledOnCount);

            client.println("</div>");

            // =================================================
            // BUTTONS
            // =================================================
            client.println("<p>");

            client.println(
              "<a href=\"/\">"
              "<button class=\"btn btn-refresh\">"
              "Refresh"
              "</button>"
              "</a>"
            );

            client.println(
              "<a href=\"/reset\">"
              "<button class=\"btn btn-reset\">"
              "Reset to 50"
              "</button>"
              "</a>"
            );

            client.println("</p>");

            client.println("</div>");

            client.println("</body>");

            client.println("</html>");

            break;

          } else {

            currentLine = "";
          }

        } else if (c != '\r') {

          currentLine += c;
        }
      }
    }

    client.stop();
  }
}