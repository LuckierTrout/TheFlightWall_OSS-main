#pragma once

#include <Arduino.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 2
#endif

#define LOG_E(tag, msg) do { if (LOG_LEVEL >= 1) Serial.println("[" tag "] ERROR: " msg); } while(0)
#define LOG_W(tag, msg) do { if (LOG_LEVEL >= 1) Serial.println("[" tag "] WARNING: " msg); } while(0)
#define LOG_I(tag, msg) do { if (LOG_LEVEL >= 2) Serial.println("[" tag "] " msg); } while(0)
#define LOG_V(tag, msg) do { if (LOG_LEVEL >= 3) Serial.println("[" tag "] " msg); } while(0)
