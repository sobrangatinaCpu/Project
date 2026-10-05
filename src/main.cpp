#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <time.h>

// =====================================================
// VERSION INFORMATION
// =====================================================
const char* VERSION = "3.0.0";
const int BUILD_NUMBER = 3;

// Real Git timestamp for Version 1
const char* VERSION_1_DATE = "2026-10-05 17:48:09 PHT";

// Version descriptions
const char* VERSION_1_CHANGE =
  "Initial working version";

const char* VERSION_2_CHANGE =
  "Added dual LED blinking and version history";

const char* VERSION_3_CHANGE =
  "Bug fix: corrected LED counter detection";

// =====================================================
// WIFI
// =====================================================
const char* ssid = "PLDT FIBR 5G";
const char* password = "cheese_91125";

// =====================================================
// LED PINS
// =====================================================
const int LED1_PIN = 2;
const int LED2_PIN = 4;

// =====================================================
// OBJECTS
// =====================================================
Preferences preferences;
WiFiServer server(80);

// =====================================================
// LED COUNTER
// =====================================================
unsigned long ledOnCount = 0;

bool lastLedState = LOW;

// =====================================================
// PHILIPPINE TIME
// UTC +8
// =====================================================
const long GMT_OFFSET_SEC = 8 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0;

// =====================================================
// VERSION HISTORY
// =====================================================
#define MAX_HISTORY 10

// =====================================================
// GET CURRENT PHILIPPINE DATE/TIME
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
// SAVE CURRENT VERSION
// =====================================================
void saveCurrentVersion() {

  int historyCount =
    preferences.getInt(
      "historyCount",
      0
    );

  // Check if this version already exists
  for (
    int i = 1;
    i <= historyCount;
    i++
  ) {

    String versionKey =
      "ver" + String(i);

    String buildKey =
      "build" + String(i);

    String storedVersion =
      preferences.getString(
        versionKey.c_str(),
        ""
      );

    int storedBuild =
      preferences.getInt(
        buildKey.c_str(),
        0
      );

    if (
      storedVersion == VERSION &&
      storedBuild == BUILD_NUMBER
    ) {

      Serial.println(
        "Version already exists in history."
      );

      return;
    }
  }

  // Prevent overflow
  if (
    historyCount >= MAX_HISTORY
  ) {

    Serial.println(
      "Version history is full."
    );

    return;
  }

  historyCount++;

  String versionKey =
    "ver" + String(historyCount);

  String buildKey =
    "build" + String(historyCount);

  String dateKey =
    "date" + String(historyCount);

  String changeKey =
    "change" + String(historyCount);

  // Save version
  preferences.putString(
    versionKey.c_str(),
    VERSION
  );

  // Save build
  preferences.putInt(
    buildKey.c_str(),
    BUILD_NUMBER
  );

  // Save date/time
  preferences.putString(
    dateKey.c_str(),
    getDateTime()
  );

  // Save description
  preferences.putString(
    changeKey.c_str(),
    VERSION_3_CHANGE
  );

  // Save history count
  preferences.putInt(
    "historyCount",
    historyCount
  );

  Serial.println();
  Serial.println(
    "======================================"
  );

  Serial.println(
    "VERSION SAVED"
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
    "Date/Time: "
  );

  Serial.println(
    getDateTime()
  );

  Serial.println(
    "======================================"
  );
}

// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(115200);

  // ===================================================
  // LED SETUP
  // ===================================================
  pinMode(
    LED1_PIN,
    OUTPUT
  );

  pinMode(
    LED2_PIN,
    OUTPUT
  );

  digitalWrite(
    LED1_PIN,
    LOW
  );

  digitalWrite(
    LED2_PIN,
    LOW
  );

  // ===================================================
  // PREFERENCES
  // ===================================================
  preferences.begin(
    "led-counter",
    false
  );

  // Load counter
  ledOnCount =
    preferences.getULong(
      "counter",
      0
    );

  // Start known LED state
  lastLedState =
    digitalRead(
      LED1_PIN
    );

  // ===================================================
  // SERIAL INFORMATION
  // ===================================================
  Serial.println();
  Serial.println(
    "======================================"
  );

  Serial.println(
    "ESP32 LED COUNTER"
  );

  Serial.println(
    "======================================"
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
    "Counter loaded: "
  );

  Serial.println(
    ledOnCount
  );

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

    Serial.print(
      "."
    );
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

  Serial.println(
    "Synchronizing Philippine time..."
  );

  struct tm timeinfo;

  if (
    getLocalTime(&timeinfo)
  ) {

    Serial.print(
      "Current PHT: "
    );

    Serial.println(
      getDateTime()
    );

  } else {

    Serial.println(
      "Could not synchronize time."
    );
  }

  // ===================================================
  // SAVE VERSION 3
  // ===================================================
  saveCurrentVersion();

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

    Serial.println();
    Serial.println(
      "End OTA Update"
    );
  });

  ArduinoOTA.onProgress(
    [](unsigned int progress,
       unsigned int total) {

      Serial.printf(
        "OTA Progress: %u%%\r",
        (progress / (total / 100))
      );
    }
  );

  ArduinoOTA.onError(
    [](ota_error_t error) {

      Serial.printf(
        "OTA Error[%u]: ",
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

  Serial.println();
  Serial.println(
    "======================================"
  );

  Serial.println(
    "SYSTEM READY"
  );

  Serial.println(
    "======================================"
  );
}

// =====================================================
// LOOP
// =====================================================
void loop() {

  // ===================================================
  // OTA
  // ===================================================
  ArduinoOTA.handle();

  // ===================================================
  // VERSION 3
  // TWO LEDs BLINK TOGETHER EVERY 500ms
  // ===================================================
  static unsigned long lastToggleTime = 0;

  if (
    millis() - lastToggleTime >= 500
  ) {

    lastToggleTime = millis();

    bool newState =
      !digitalRead(
        LED1_PIN
      );

    digitalWrite(
      LED1_PIN,
      newState
    );

    digitalWrite(
      LED2_PIN,
      newState
    );
  }

  // ===================================================
  // VERSION 3 BUG FIX
  // ONLY COUNT A REAL OFF -> ON TRANSITION
  // ===================================================
  bool currentLedState =
    digitalRead(
      LED1_PIN
    );

  if (
    currentLedState != lastLedState
  ) {

    // Only count when LED changes to ON
    if (
      currentLedState == HIGH
    ) {

      ledOnCount++;

      preferences.putULong(
        "counter",
        ledOnCount
      );

      Serial.print(
        "Bug Fix: LED activation counted. Total: "
      );

      Serial.println(
        ledOnCount
      );
    }

    // Update previous state
    lastLedState =
      currentLedState;
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
              "<title>"
              "ESP32 LED Dashboard"
              "</title>"
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
              "min-width:450px;"
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
            // CURRENT VERSION
            // =========================================
            client.println(
              "<p>"
              "<b>Current Version:</b> "
            );

            client.print(
              VERSION
            );

            client.println(
              "</p>"
            );

            client.println(
              "<p>"
              "<b>Build:</b> "
            );

            client.print(
              BUILD_NUMBER
            );

            client.println(
              "</p>"
            );

            // =========================================
            // VERSION HISTORY
            // =========================================
            client.println(
              "<h2>"
              "Version History"
              "</h2>"
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

            // =========================================
            // VERSION 1
            // =========================================
            client.println(
              "<tr>"
              "<td>1.0.0</td>"
              "<td>1</td>"
              "<td>"
              "2026-10-05 17:48:09 PHT"
              "</td>"
              "<td>"
              "Initial working version"
              "</td>"
              "</tr>"
            );

            // =========================================
            // STORED VERSION HISTORY
            // =========================================
            int historyCount =
              preferences.getInt(
                "historyCount",
                0
              );

            for (
              int i = 1;
              i <= historyCount;
              i++
            ) {

              String versionKey =
                "ver" + String(i);

              String buildKey =
                "build" + String(i);

              String dateKey =
                "date" + String(i);

              String changeKey =
                "change" + String(i);

              String storedVersion =
                preferences.getString(
                  versionKey.c_str(),
                  ""
                );

              int storedBuild =
                preferences.getInt(
                  buildKey.c_str(),
                  0
                );

              String storedDate =
                preferences.getString(
                  dateKey.c_str(),
                  "Unknown"
                );

              String storedChange =
                preferences.getString(
                  changeKey.c_str(),
                  "Unknown"
                );

              // Version 1 is already displayed manually.
              if (
                storedVersion == "1.0.0"
              ) {

                continue;
              }

              client.print(
                "<tr>"
              );

              client.print(
                "<td>"
              );

              client.print(
                storedVersion
              );

              client.print(
                "</td>"
              );

              client.print(
                "<td>"
              );

              client.print(
                storedBuild
              );

              client.print(
                "</td>"
              );

              client.print(
                "<td>"
              );

              client.print(
                storedDate
              );

              client.print(
                "</td>"
              );

              client.print(
                "<td>"
              );

              client.print(
                storedChange
              );

              client.print(
                "</td>"
              );

              client.println(
                "</tr>"
              );
            }

            // =========================================
            // CURRENT VERSION IF NOT YET SAVED
            // =========================================
            bool currentVersionShown =
              false;

            for (
              int i = 1;
              i <= historyCount;
              i++
            ) {

              String storedVersion =
                preferences.getString(
                  ("ver" + String(i)).c_str(),
                  ""
                );

              int storedBuild =
                preferences.getInt(
                  ("build" + String(i)).c_str(),
                  0
                );

              if (
                storedVersion == VERSION &&
                storedBuild == BUILD_NUMBER
              ) {

                currentVersionShown =
                  true;

                break;
              }
            }

            if (
              !currentVersionShown
            ) {

              client.print(
                "<tr>"
              );

              client.print(
                "<td>"
              );

              client.print(
                VERSION
              );

              client.print(
                "</td>"
              );

              client.print(
                "<td>"
              );

              client.print(
                BUILD_NUMBER
              );

              client.print(
                "</td>"
              );

              client.print(
                "<td>"
              );

              client.print(
                getDateTime()
              );

              client.print(
                "</td>"
              );

              client.print(
                "<td>"
              );

              client.print(
                VERSION_3_CHANGE
              );

              client.print(
                "</td>"
              );

              client.println(
                "</tr>"
              );
            }

            client.println(
              "</table>"
            );

            // =========================================
            // LED COUNTER
            // =========================================
            client.println(
              "<p>"
              "Total times LEDs turned ON:"
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