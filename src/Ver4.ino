#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <time.h>

// =====================================================
// VERSION
// =====================================================
const char* VERSION = "4.0.0";
const int BUILD_NUMBER = 4;

// =====================================================
// WIFI
// =====================================================
const char* ssid = "PLDT FIBR 5G";
const char* password = "cheese_91125";

// =====================================================
// FIVE LED PINS
// =====================================================
// LED 1 = GPIO 2
// LED 2 = GPIO 4
// LED 3 = GPIO 5
// LED 4 = GPIO 18
// LED 5 = GPIO 19

const int ledPins[] = {
  2, 4, 18, 19, 22
};

const int LED_COUNT = 5;

// =====================================================
// OBJECTS
// =====================================================
Preferences preferences;
WiFiServer server(80);

// =====================================================
// COUNTER
// =====================================================
// Version 4 starts the counter at 5.
unsigned long ledOnCount = 5;

// =====================================================
// CHASER
// =====================================================
int currentLED = 0;
int direction = 1;

unsigned long lastChaseTime = 0;

// 500 milliseconds between LEDs
const unsigned long CHASE_DELAY = 500;

// =====================================================
// PHILIPPINE TIME
// =====================================================
const long GMT_OFFSET_SEC = 8 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0;

// =====================================================
// DATE / TIME
// =====================================================
String getDateTime() {

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    return "Time not available";
  }

  char buffer[40];

  strftime(
    buffer,
    sizeof(buffer),
    "%Y-%m-%d %H:%M:%S",
    &timeinfo
  );

  return String(buffer) + " PHT";
}

// =====================================================
// TURN ALL LEDS OFF
// =====================================================
void allLEDsOff() {

  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(
      ledPins[i],
      LOW
    );
  }
}

// =====================================================
// SHOW CURRENT CHASER LED
// =====================================================
void showChaserLED() {

  allLEDsOff();

  digitalWrite(
    ledPins[currentLED],
    HIGH
  );

  Serial.print("Chaser LED: ");
  Serial.println(currentLED + 1);
}

// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(115200);

  // ===================================================
  // LED SETUP
  // ===================================================

  for (int i = 0; i < LED_COUNT; i++) {

    pinMode(
      ledPins[i],
      OUTPUT
    );

    digitalWrite(
      ledPins[i],
      LOW
    );
  }

  // ===================================================
  // PREFERENCES
  // ===================================================
  preferences.begin(
    "led-counter",
    false
  );

  // Version 4 counter starts at 5.
  // Do NOT load the previous version's counter.
  ledOnCount = 5;

  preferences.putULong(
    "counter",
    ledOnCount
  );

  // ===================================================
  // SERIAL INFORMATION
  // ===================================================

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "ESP32 LED DASHBOARD"
  );

  Serial.println(
    "================================"
  );

  Serial.print(
    "Version: "
  );

  Serial.println(
    VERSION
  );

  Serial.print(
    "Build: "
  );

  Serial.println(
    BUILD_NUMBER
  );

  Serial.print(
    "LED count: "
  );

  Serial.println(
    LED_COUNT
  );

  Serial.print(
    "Counter: "
  );

  Serial.println(
    ledOnCount
  );

  // ===================================================
  // SHOW FIRST LED
  // ===================================================

  showChaserLED();

  // ===================================================
  // WIFI
  // ===================================================

  WiFi.mode(
    WIFI_STA
  );

  WiFi.begin(
    ssid,
    password
  );

  Serial.print(
    "Connecting to Wi-Fi"
  );

  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();

  Serial.println(
    "Wi-Fi connected."
  );

  Serial.print(
    "IP Address: "
  );

  Serial.println(
    WiFi.localIP()
  );

  // ===================================================
  // PHILIPPINE TIME
  // ===================================================

  configTime(
    GMT_OFFSET_SEC,
    DAYLIGHT_OFFSET_SEC,
    "pool.ntp.org",
    "time.nist.gov"
  );

  delay(1000);

  Serial.print(
    "Current PHT: "
  );

  Serial.println(
    getDateTime()
  );

  // ===================================================
  // OTA
  // ===================================================

  ArduinoOTA.setHostname(
    "esp32-led-counter"
  );

  ArduinoOTA.onStart([]() {

    Serial.println(
      "Start OTA Update"
    );

  });

  ArduinoOTA.onEnd([]() {

    Serial.println(
      "\nEnd OTA Update"
    );

  });

  ArduinoOTA.onProgress(
    [](unsigned int progress,
       unsigned int total) {

      Serial.printf(
        "Progress: %u%%\r",
        progress * 100 / total
      );

    }
  );

  ArduinoOTA.onError(
    [](ota_error_t error) {

      Serial.printf(
        "Error[%u]: ",
        error
      );

      if (
        error == OTA_AUTH_ERROR
      ) {

        Serial.println(
          "Auth Failed"
        );

      } else if (
        error == OTA_BEGIN_ERROR
      ) {

        Serial.println(
          "Begin Failed"
        );

      } else if (
        error == OTA_CONNECT_ERROR
      ) {

        Serial.println(
          "Connect Failed"
        );

      } else if (
        error == OTA_RECEIVE_ERROR
      ) {

        Serial.println(
          "Receive Failed"
        );

      } else if (
        error == OTA_END_ERROR
      ) {

        Serial.println(
          "End Failed"
        );
      }

    }
  );

  ArduinoOTA.begin();

  // ===================================================
  // WEB SERVER
  // ===================================================

  server.begin();

  Serial.println(
    "Web server started."
  );

  Serial.println(
    "System ready."
  );
}

// =====================================================
// LOOP
// =====================================================
void loop() {

  ArduinoOTA.handle();

  // ===================================================
  // LED CHASER
  // ===================================================

  if (
    millis() - lastChaseTime >= CHASE_DELAY
  ) {

    lastChaseTime = millis();

    // Move to next LED
    currentLED += direction;

    // Reached the right side
    if (
      currentLED >= LED_COUNT - 1
    ) {

      currentLED = LED_COUNT - 1;

      direction = -1;
    }

    // Reached the left side
    else if (
      currentLED <= 0
    ) {

      currentLED = 0;

      direction = 1;
    }

    showChaserLED();
  }

  // ===================================================
  // WEB SERVER
  // ===================================================

  WiFiClient client =
    server.available();

  if (client) {

    String currentLine = "";
    String requestString = "";

    while (
      client.connected()
    ) {

      if (
        client.available()
      ) {

        char c =
          client.read();

        requestString += c;

        if (
          c == '\n'
        ) {

          if (
            currentLine.length() == 0
          ) {

            // =========================================
            // RESET COUNTER
            // =========================================

            if (
              requestString.indexOf(
                "GET /reset"
              ) != -1
            ) {

              ledOnCount = 0;

              preferences.putULong(
                "counter",
                0
              );

              Serial.println(
                "Counter reset to 0."
              );
            }

            // =========================================
            // HTTP RESPONSE
            // =========================================

            client.println(
              "HTTP/1.1 200 OK"
            );

            client.println(
              "Content-type:text/html"
            );

            client.println(
              "Connection: close"
            );

            client.println();

            // =========================================
            // HTML
            // =========================================

            client.println(
              "<!DOCTYPE html>"
            );

            client.println(
              "<html>"
            );

            client.println(
              "<head>"
            );

            client.println(
              "<meta name=\"viewport\" "
              "content=\"width=device-width,"
              "initial-scale=1\">"
            );

            client.println(
              "<title>ESP32 LED Dashboard</title>"
            );

            // =========================================
            // CSS
            // =========================================

            client.println(
              "<style>"
              "body{"
              "font-family:Arial,sans-serif;"
              "text-align:center;"
              "margin-top:40px;"
              "background:#f4f4f9;"
              "}"
              ".card{"
              "background:white;"
              "padding:30px;"
              "border-radius:15px;"
              "box-shadow:0 4px 15px "
              "rgba(0,0,0,.1);"
              "display:inline-block;"
              "min-width:500px;"
              "}"
              "h1{color:#333;}"
              "table{"
              "border-collapse:collapse;"
              "width:100%;"
              "margin:20px 0;"
              "}"
              "th,td{"
              "border:1px solid #ccc;"
              "padding:10px;"
              "}"
              "th{"
              "background:#eeeeee;"
              "}"
              ".counter{"
              "font-size:60px;"
              "font-weight:bold;"
              "color:#007bff;"
              "margin:20px;"
              "}"
              ".chaser{"
              "font-size:18px;"
              "font-weight:bold;"
              "}"
              ".btn{"
              "padding:10px 20px;"
              "font-size:16px;"
              "margin:5px;"
              "cursor:pointer;"
              "border:none;"
              "border-radius:5px;"
              "}"
              ".refresh{"
              "background:#007bff;"
              "color:white;"
              "}"
              ".reset{"
              "background:#dc3545;"
              "color:white;"
              "}"
              "</style>"
            );

            client.println(
              "</head>"
            );

            client.println(
              "<body>"
            );

            client.println(
              "<div class=\"card\">"
            );

            // =========================================
            // TITLE
            // =========================================

            client.println(
              "<h1>"
              "ESP32 LED Dashboard"
              "</h1>"
            );

            // =========================================
            // VERSION INFORMATION
            // =========================================

            client.print(
              "<p><b>Current Version:</b> "
            );

            client.print(
              VERSION
            );

            client.println(
              "</p>"
            );

            client.print(
              "<p><b>Build:</b> "
            );

            client.print(
              BUILD_NUMBER
            );

            client.println(
              "</p>"
            );

            client.println(
              "<p><b>LEDs:</b> 5</p>"
            );

            client.println(
              "<p class=\"chaser\">"
              "Chaser: LEFT → RIGHT → LEFT"
              "</p>"
            );

            // =========================================
            // VERSION HISTORY
            // =========================================

            client.println(
              "<h2>Version History</h2>"
            );

            client.println(
              "<table>"
            );

            client.println(
              "<tr>"
              "<th>Version</th>"
              "<th>Build</th>"
              "<th>Date / Time</th>"
              "<th>Changes</th>"
              "</tr>"
            );

            // Version 1

            client.println(
              "<tr>"
              "<td>1.0.0</td>"
              "<td>1</td>"
              "<td>2026-10-05 17:48:09 PHT</td>"
              "<td>Initial working version</td>"
              "</tr>"
            );

            // Version 2

            client.println(
              "<tr>"
              "<td>2.0.0</td>"
              "<td>2</td>"
              "<td>2026-10-05</td>"
              "<td>Dual LED 500ms blink</td>"
              "</tr>"
            );

            // Version 3

            client.println(
              "<tr>"
              "<td>3.0.0</td>"
              "<td>3</td>"
              "<td>2026-10-05</td>"
              "<td>Bug fix: corrected LED counter detection</td>"
              "</tr>"
            );

            // Version 4

            client.println(
              "<tr>"
              "<td>4.0.0</td>"
              "<td>4</td>"
              "<td>2026-10-05</td>"
              "<td>5 LED racing/chaser effect</td>"
              "</tr>"
            );

            client.println(
              "</table>"
            );

            // =========================================
            // COUNTER
            // =========================================

            client.println(
              "<p>"
              "LED Count:"
              "</p>"
            );

            client.print(
              "<div class=\"counter\">"
            );

            client.print(
              ledOnCount
            );

            client.println(
              "</div>"
            );

            // =========================================
            // BUTTONS
            // =========================================

            client.println(
              "<a href=\"/\">"
              "<button class=\"btn refresh\">"
              "Refresh"
              "</button>"
              "</a>"
            );

            client.println(
              "<a href=\"/reset\">"
              "<button class=\"btn reset\">"
              "Reset Counter"
              "</button>"
              "</a>"
            );

            // =========================================
            // END HTML
            // =========================================

            client.println(
              "</div>"
            );

            client.println(
              "</body>"
            );

            client.println(
              "</html>"
            );

            break;

          } else {

            currentLine = "";
          }

        } else if (
          c != '\r'
        ) {

          currentLine += c;
        }
      }
    }

    client.stop();
  }
}