#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <time.h>

// =====================================================
// VERSION INFORMATION
// =====================================================
const char* VERSION = "2.0.0";
const int BUILD_NUMBER = 2;

// =====================================================
// Wi-Fi Configuration
// =====================================================
const char* ssid = "PLDT FIBR 5G";
const char* password = "cheese_91125";

// =====================================================
// Pin Definitions
// =====================================================
const int LED1_PIN = 2;
const int LED2_PIN = 4;

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
// Philippine Time
// UTC +8
// =====================================================
const long GMT_OFFSET_SEC = 8 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0;

// =====================================================
// Get Current Date and Time
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

  // ===================================================
  // Initialize LEDs
  // ===================================================
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  // ===================================================
  // 1. Initialize Preferences
  // ===================================================
  preferences.begin("led-counter", false);

  // Load saved counter
  // If no counter exists, start at 0
  ledOnCount = preferences.getULong("counter", 0);

  Serial.println();
  Serial.println("======================================");
  Serial.println("ESP32 LED COUNTER");
  Serial.println("======================================");

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

  Serial.println("Synchronizing time...");

  struct tm timeinfo;

  if (getLocalTime(&timeinfo)) {

    Serial.println("Time synchronized.");

    Serial.print("Current Date/Time: ");
    Serial.println(getDateTime());

  } else {

    Serial.println("Time synchronization failed.");
  }

  // ===================================================
  // 4. Arduino OTA
  // ===================================================
  ArduinoOTA.setHostname("esp32-led-counter");

  ArduinoOTA.onStart([]() {

    Serial.println("Start OTA Update");
  });

  ArduinoOTA.onEnd([]() {

    Serial.println();
    Serial.println("End OTA Update");
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
  Serial.println("======================================");
  Serial.println("SYSTEM READY");
  Serial.println("======================================");
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
  // VERSION 2
  // Two LEDs blink simultaneously every 500 ms
  // ===================================================
  static unsigned long lastToggleTime = 0;

  if (millis() - lastToggleTime >= 500) {

    lastToggleTime = millis();

    bool currentState = digitalRead(LED1_PIN);

    // Both LEDs receive the same state
    digitalWrite(LED1_PIN, !currentState);
    digitalWrite(LED2_PIN, !currentState);
  }

  // ===================================================
  // LED COUNTER
  // Count only LED 1 so one blink = one count
  // ===================================================
  bool currentLedState = digitalRead(LED1_PIN);

  // Detect OFF -> ON
  if (currentLedState && !lastLedState) {

    ledOnCount++;

    // Save counter to ESP32 flash
    preferences.putULong(
      "counter",
      ledOnCount
    );

    Serial.print("LEDs turned ON! Total activations: ");
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

            // =========================================
            // RESET COUNTER
            // =========================================
            if (requestString.indexOf("GET /reset") != -1) {

              ledOnCount = 0;

              preferences.putULong(
                "counter",
                ledOnCount
              );

              Serial.println(
                "Counter reset to 0 via dashboard!"
              );
            }

            // =========================================
            // HTTP RESPONSE
            // =========================================
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // =========================================
            // HTML
            // =========================================
            client.println("<!DOCTYPE html>");
            client.println("<html>");

            client.println("<head>");

            client.println(
              "<meta name=\"viewport\" "
              "content=\"width=device-width, initial-scale=1\">"
            );

            // Refresh every 5 seconds
            client.println(
              "<meta http-equiv=\"refresh\" content=\"5\">"
            );

            client.println(
              "<title>ESP32 LED Dashboard</title>"
            );

            // =========================================
            // CSS
            // =========================================
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
              "min-width: 330px;"
              "}"
            );

            client.println(
              "h1 {"
              "color: #333;"
              "}"
            );

            // =========================================
            // TABLE
            // =========================================
            client.println(
              "table {"
              "margin: 20px auto;"
              "border-collapse: collapse;"
              "width: 100%;"
              "}"
            );

            client.println(
              "th, td {"
              "border: 1px solid #ccc;"
              "padding: 10px;"
              "text-align: center;"
              "}"
            );

            client.println(
              "th {"
              "background-color: #eeeeee;"
              "}"
            );

            // =========================================
            // COUNTER
            // =========================================
            client.println(
              ".counter {"
              "font-size: 60px;"
              "color: #007bff;"
              "font-weight: bold;"
              "margin: 20px 0;"
              "}"
            );

            // =========================================
            // BUTTON
            // =========================================
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

            // =========================================
            // BODY
            // =========================================
            client.println("<body>");

            client.println("<div class=\"card\">");

            client.println(
              "<h1>ESP32 LED Dashboard</h1>"
            );

            // =========================================
            // VERSION TABLE
            // =========================================
            client.println("<table>");

            client.println("<tr>");

            client.println("<th>Version</th>");
            client.println("<th>Build</th>");
            client.println("<th>Date Time</th>");

            client.println("</tr>");

            client.println("<tr>");

            client.print("<td>");
            client.print(VERSION);
            client.println("</td>");

            client.print("<td>");
            client.print(BUILD_NUMBER);
            client.println("</td>");

            client.print("<td>");
            client.print(getDateTime());
            client.println("</td>");

            client.println("</tr>");

            client.println("</table>");

            // =========================================
            // COUNTER
            // =========================================
            client.println(
              "<p>Total times LEDs turned ON:</p>"
            );

            client.print(
              "<div class=\"counter\">"
            );

            client.print(ledOnCount);

            client.println("</div>");

            // =========================================
            // BUTTONS
            // =========================================
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
              "Reset Counter"
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