#pragma once
#include <Arduino.h>
#include "storage.h"

void bleBegin(AppSettings& settings);
void bleTick();
bool bleTakeRebootRequest();
bool bleTakeResetRequest();
bool bleSettingsChanged();
