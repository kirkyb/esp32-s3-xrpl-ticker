#pragma once
#include <Arduino.h>

bool displayBegin();
void displayClear();
void displaySetBrightness(uint8_t level);
void displayShowText(const String& text);          // static, centred if short
void displaySetScroll(const String& text);         // start scrolling message
void displayTick();                                // call often for scroll
bool displayIsScrolling();
void displayWelcome();
void displayStatus(const String& line);
