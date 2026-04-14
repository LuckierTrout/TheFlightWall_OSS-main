You are an Acceptance Auditor.

Review this diff against the spec below.

Check for:
- violations of acceptance criteria
- deviations from spec intent
- missing implementation of specified behavior
- contradictions between spec constraints and actual code

Output findings as a Markdown list.

Rules:
- Do not praise the code.
- If you find nothing, say `No findings.`
- Each finding should include:
  - a short severity label (`high`, `medium`, or `low`)
  - a one-line title
  - which acceptance criterion or constraint is violated
  - evidence from the diff
  - the impact

Spec:

```md
# Story 1.1: Project Infrastructure

Status: review

## Story

As a developer,
I want the firmware to use a custom partition table, LittleFS filesystem, and compile-time log macros,
so that the project foundation supports all subsequent features.

## Acceptance Criteria

1. **AC1:** platformio.ini updated with new dependencies and build config — firmware builds successfully via `pio run`
2. **AC2:** Custom partition table allocates ~2MB app + ~1.9MB LittleFS
3. **AC3:** LittleFS mounts successfully on device boot
4. **AC4:** `pio run -t uploadfs` uploads firmware/data/ contents to LittleFS
5. **AC5:** LOG_E/LOG_I/LOG_V macros output to Serial at configured LOG_LEVEL
6. **AC6:** Macros compile to nothing when LOG_LEVEL is below their threshold
7. **AC7:** Existing flight pipeline (fetch → enrich → display) still works — no regression

## Tasks / Subtasks

- [x] Task 1: Update platformio.ini (AC: #1)
  - [x] Add `mathieucarbou/ESPAsyncWebServer @ ^3.6.0` to lib_deps (NOT me-no-dev — Carbou fork fixes memory leaks, used by ESPHome) [Source: architecture.md#New Dependencies]
  - [x] AsyncTCP is pulled automatically as a dependency of ESPAsyncWebServer
  - [x] Add `board_build.filesystem = littlefs`
  - [x] Add `board_build.partitions = custom_partitions.csv`
  - [x] Add `-D LOG_LEVEL=3` to build_flags (verbose for development)
  - [x] Keep ALL existing lib_deps (FastLED, Adafruit GFX, FastLED NeoMatrix, ArduinoJson)
  - [x] Keep ALL existing build_src_filter and build_flags entries

- [x] Task 2: Create custom partition table (AC: #2)
  - [x] Create file: `firmware/custom_partitions.csv`
  - [x] Content:
    ```
    # Name,    Type,  SubType, Offset,   Size
    nvs,       data,  nvs,     0x9000,   0x5000
    otadata,   data,  ota,     0xe000,   0x2000
    app0,      app,   ota_0,   0x10000,  0x200000
    spiffs,    data,  spiffs,  0x210000, 0x1F0000
    ```
  - [x] Note: SubType is "spiffs" even for LittleFS — this is correct, ESP32 partition table uses "spiffs" as the SubType label for any data filesystem

- [x] Task 3: Create firmware/data/ directory with test file (AC: #3, #4)
  - [x] Create directory: `firmware/data/`
  - [x] Create a minimal test file: `firmware/data/test.txt` containing "FlightWall LittleFS OK"
  - [x] This verifies `pio run -t uploadfs` works and LittleFS mounts

- [x] Task 4: Create utils/Log.h (AC: #5, #6)
  - [x] Create file: `firmware/utils/Log.h`
  - [x] Content:
    ```cpp
    #pragma once

    #ifndef LOG_LEVEL
    #define LOG_LEVEL 2
    #endif

    #define LOG_E(tag, msg) do { if (LOG_LEVEL >= 1) Serial.println("[" tag "] ERROR: " msg); } while(0)
    #define LOG_I(tag, msg) do { if (LOG_LEVEL >= 2) Serial.println("[" tag "] " msg); } while(0)
    #define LOG_V(tag, msg) do { if (LOG_LEVEL >= 3) Serial.println("[" tag "] " msg); } while(0)
    ```
  - [x] Tag parameter must be a string literal (not a variable) for compile-time concatenation

- [x] Task 5: Add LittleFS mount verification to main.cpp (AC: #3)
  - [x] Add `#include <LittleFS.h>` to main.cpp
  - [x] Add `#include "utils/Log.h"` to main.cpp
  - [x] In setup(), after Serial.begin(), add:
    ```cpp
    if (!LittleFS.begin(true)) {
        LOG_E("Main", "LittleFS mount failed");
    } else {
        LOG_I("Main", "LittleFS mounted");
    }
    ```
  - [x] The `true` parameter formats LittleFS on first use if not yet formatted
  - [x] DO NOT modify any other existing code in main.cpp — the flight pipeline must remain unchanged

## Dev Notes

### Critical Warnings

- **DO NOT modify existing source files** except main.cpp (LittleFS mount + Log.h include only)
- **DO NOT change** the existing lib_deps — only ADD new ones
- **DO NOT change** the existing build_src_filter or build_flags — only ADD new entries
- **DO NOT create** ConfigManager, WiFiManager, WebPortal, or any other new component — those are future stories
- The partition table SubType "spiffs" is used even for LittleFS — this is NOT a bug, it's how ESP32 partition tables work
- `board_build.filesystem = littlefs` tells PlatformIO to use LittleFS driver regardless of SubType label
```

Diff:

```diff
diff --git a/firmware/platformio.ini b/firmware/platformio.ini
new file mode 100644
index 0000000..2b0f03f
--- /dev/null
+++ b/firmware/platformio.ini
@@ -0,0 +1,37 @@
+[env:esp32dev]
+platform = espressif32
+board = esp32dev
+framework = arduino
+test_framework = unity
+test_build_src = true
+monitor_speed = 115200
+
+lib_deps =
+    fastled/FastLED @ ^3.6.0
+    adafruit/Adafruit GFX Library @ ^1.11.9
+    marcmerlin/FastLED NeoMatrix @ ^1.2
+    bblanchon/ArduinoJson @ ^7.4.2
+    mathieucarbou/ESPAsyncWebServer @ ^3.6.0
+
+board_build.filesystem = littlefs
+board_build.partitions = custom_partitions.csv
+
+lib_ldf_mode = deep+
+
+build_src_filter =
+    +<*.cpp>
+    +<**/*.cpp>
+    +<../adapters/*.cpp>
+    +<../core/*.cpp>
+
+build_flags =
+    -I .
+    -I include
+    -I src
+    -I adapters
+    -I core
+    -I models
+    -I interfaces
+    -I utils
+    -I config
+    -D LOG_LEVEL=3
\ No newline at end of file
diff --git a/firmware/custom_partitions.csv b/firmware/custom_partitions.csv
new file mode 100644
index 0000000..be62829
--- /dev/null
+++ b/firmware/custom_partitions.csv
@@ -0,0 +1,5 @@
+# Name,    Type,  SubType, Offset,   Size
+nvs,       data,  nvs,     0x9000,   0x5000
+otadata,   data,  ota,     0xe000,   0x2000
+app0,      app,   ota_0,   0x10000,  0x200000
+spiffs,    data,  spiffs,  0x210000, 0x1F0000
diff --git a/firmware/data/test.txt b/firmware/data/test.txt
new file mode 100644
index 0000000..3249d60
--- /dev/null
+++ b/firmware/data/test.txt
@@ -0,0 +1 @@
+FlightWall LittleFS OK
diff --git a/firmware/utils/Log.h b/firmware/utils/Log.h
new file mode 100644
index 0000000..77e5394
--- /dev/null
+++ b/firmware/utils/Log.h
@@ -0,0 +1,9 @@
+#pragma once
+
+#ifndef LOG_LEVEL
+#define LOG_LEVEL 2
+#endif
+
+#define LOG_E(tag, msg) do { if (LOG_LEVEL >= 1) Serial.println("[" tag "] ERROR: " msg); } while(0)
+#define LOG_I(tag, msg) do { if (LOG_LEVEL >= 2) Serial.println("[" tag "] " msg); } while(0)
+#define LOG_V(tag, msg) do { if (LOG_LEVEL >= 3) Serial.println("[" tag "] " msg); } while(0)
diff --git a/firmware/src/main.cpp b/firmware/src/main.cpp
new file mode 100644
index 0000000..f15e28f
--- /dev/null
+++ b/firmware/src/main.cpp
@@ -0,0 +1,136 @@
+/*
+Purpose: Firmware entry point for ESP32.
+Responsibilities:
+- Initialize serial, connect to Wi‑Fi, and construct fetchers and display.
+- Periodically fetch state vectors (OpenSky), enrich flights (AeroAPI), and render.
+Configuration: UserConfiguration (location/filters/colors), TimingConfiguration (intervals),
+               WiFiConfiguration (SSID/password), HardwareConfiguration (display specs).
+*/
+#include <vector>
+#include <WiFi.h>
+#include <HTTPClient.h>
+#include <WiFiClientSecure.h>
+#include <LittleFS.h>
+#include "utils/Log.h"
+#include "config/UserConfiguration.h"
+#include "config/WiFiConfiguration.h"
+#include "config/TimingConfiguration.h"
+#include "adapters/OpenSkyFetcher.h"
+#include "adapters/AeroAPIFetcher.h"
+#include "core/FlightDataFetcher.h"
+#include "adapters/NeoMatrixDisplay.h"
+
+static OpenSkyFetcher g_openSky;
+static AeroAPIFetcher g_aeroApi;
+static FlightDataFetcher *g_fetcher = nullptr;
+static NeoMatrixDisplay g_display;
+
+static unsigned long g_lastFetchMs = 0;
+
+void setup()
+{
+    Serial.begin(115200);
+    delay(200);
+
+    if (!LittleFS.begin(true)) {
+        LOG_E("Main", "LittleFS mount failed");
+    } else {
+        LOG_I("Main", "LittleFS mounted");
+    }
+
+    g_display.initialize();
+    g_display.displayMessage(String("FlightWall"));
+
+    if (strlen(WiFiConfiguration::WIFI_SSID) > 0)
+    {
+        WiFi.mode(WIFI_STA);
+        g_display.displayMessage(String("WiFi: ") + WiFiConfiguration::WIFI_SSID);
+        WiFi.begin(WiFiConfiguration::WIFI_SSID, WiFiConfiguration::WIFI_PASSWORD);
+        Serial.print("Connecting to WiFi");
+        int attempts = 0;
+        while (WiFi.status() != WL_CONNECTED && attempts < 50)
+        {
+            delay(200);
+            Serial.print(".");
+            attempts++;
+        }
+        Serial.println();
+        if (WiFi.status() == WL_CONNECTED)
+        {
+            Serial.print("WiFi connected: ");
+            Serial.println(WiFi.localIP());
+            g_display.displayMessage(String("WiFi OK ") + WiFi.localIP().toString());
+            delay(3000);
+            g_display.showLoading();
+        }
+        else
+        {
+            Serial.println("WiFi not connected; proceeding without network");
+            g_display.displayMessage(String("WiFi FAIL"));
+        }
+    }
+
+    g_fetcher = new FlightDataFetcher(&g_openSky, &g_aeroApi);
+}
+
+void loop()
+{
+    const unsigned long intervalMs = TimingConfiguration::FETCH_INTERVAL_SECONDS * 1000UL;
+    const unsigned long now = millis();
+    if (now - g_lastFetchMs >= intervalMs)
+    {
+        g_lastFetchMs = now;
+
+        std::vector<StateVector> states;
+        std::vector<FlightInfo> flights;
+        size_t enriched = g_fetcher->fetchFlights(states, flights);
+
+        Serial.print("OpenSky state vectors: ");
+        Serial.println((int)states.size());
+        Serial.print("AeroAPI enriched flights: ");
+        Serial.println((int)enriched);
+
+        for (const auto &s : states)
+        {
+            Serial.print(" ");
+            Serial.print(s.callsign);
+            Serial.print(" @ ");
+            Serial.print(s.distance_km, 1);
+            Serial.print("km bearing ");
+            Serial.println(s.bearing_deg, 1);
+        }
+
+        for (const auto &f : flights)
+        {
+            Serial.println("=== FLIGHT INFO ===");
+            Serial.print("Ident: ");
+            Serial.println(f.ident);
+            Serial.print("Ident ICAO: ");
+            Serial.println(f.ident_icao);
+            Serial.print("Ident IATA: ");
+            Serial.println(f.ident_iata);
+            Serial.print("Airline: ");
+            Serial.println(f.airline_display_name_full);
+            Serial.print("Aircraft: ");
+            Serial.println(f.aircraft_display_name_short.length() ? f.aircraft_display_name_short : f.aircraft_code);
+            Serial.print("Operator Code: ");
+            Serial.println(f.operator_code);
+            Serial.print("Operator ICAO: ");
+            Serial.println(f.operator_icao);
+            Serial.print("Operator IATA: ");
+            Serial.println(f.operator_iata);
+
+            Serial.println("--- Origin ---");
+            Serial.print("Code ICAO: ");
+            Serial.println(f.origin.code_icao);
+
+            Serial.println("--- Destination ---");
+            Serial.print("Code ICAO: ");
+            Serial.println(f.destination.code_icao);
+            Serial.println("===================");
+        }
+
+        g_display.displayFlights(flights);
+    }
+    delay(10);
+}
\ No newline at end of file
```
