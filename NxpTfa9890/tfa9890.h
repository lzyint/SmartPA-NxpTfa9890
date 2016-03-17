//Copyright (C) Microsoft Corporation, All Rights Reserved
//
//Abstract:
//
//      This module contains the constant definitions for the accelerometer's
//      register interface and default property values.

#pragma once

#include "WTypesbase.h"

#if DBG
#define DLog(...)  DbgPrintEx(0, DPFLTR_ERROR_LEVEL, __VA_ARGS__); 
#else
#define DLog(...)
#endif

// Register interface
#define TFA9890_I2S_CONTROL                 0x04
#define TFA9890_SYSTEM_CONTROL              0x09

// I2S control register bits
#define TFA9890_I2S_CONTROL_BYPASS          0x0B88


// System control register bits
#define TFA9890_SYSTEM_CONTROL_BYPASS_1     0x0982
#define TFA9890_SYSTEM_CONTROL_BYPASS_2     0x0806


const unsigned short SENSOR_PA_MANUFACTURER[] = L"NXP";
const unsigned short SENSOR_PA_MODEL[] = L"TFA9890";
