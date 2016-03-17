//Copyright (C) Microsoft Corporation, All Rights Reserved
//
//Abstract:
//
//    This module contains the type definitions for the client
//    driver's device callback class.
//
//Environment:
//
//    Windows User-Mode Driver Framework (UMDF)

#pragma once

#include <windows.h>
#include <wdf.h>
#include <reshub.h>
#include <strsafe.h>

#include <SensorsDef.h>
#include <SensorsCx.h>
#include <sensorsutils.h>
#include <SensorsDriversUtils.h>

#include "TFA9890.h"
#include "SensorsTrace.h"



#define PA_POOL_TAG_ACCELEROMETER 'NXPA'


// Sensor Common Properties
typedef enum
{
    SENSOR_PROPERTY_STATE = 0,
    SENSOR_PROPERTIES_COUNT
} SENSOR_PROPERTIES_INDEX;

// Sensor Enumeration Properties
typedef enum
{
    SENSOR_ENUMERATION_PROPERTY_TYPE = 0,
	SENSOR_ENUMERATION_PROPERTY_CATEGORY,
    SENSOR_ENUMERATION_PROPERTY_MANUFACTURER,
    SENSOR_ENUMERATION_PROPERTY_MODEL,
    SENSOR_ENUMERATION_PROPERTY_PERSISTENT_UNIQUE_ID,
	SENSOR_ENUMERATION_PROPERTY_SUBTYPE,
    SENSOR_ENUMERATION_PROPERTIES_COUNT
} SENSOR_ENUMERATION_PROPERTIES_INDEX;

// Data-field Properties
typedef enum
{
    SENSOR_DATA_ACCELERATION_X_G = 0,
    SENSOR_DATA_ACCELERATION_Y_G,
    SENSOR_DATA_ACCELERATION_Z_G,
    SENSOR_DATA_TIMESTAMP,
    SENSOR_DATA_COUNT
} SENSOR_DATA_INDEX;

typedef enum
{
    SENSOR_DATA_FIELD_PROPERTY_RESOLUTION = 0,
    SENSOR_DATA_FIELD_PROPERTY_RANGE_MIN,
    SENSOR_DATA_FIELD_PROPERTY_RANGE_MAX,
    SENSOR_DATA_FIELD_PROPERTIES_COUNT
} SENSOR_DATA_FIELD_PROPERTIES_INDEX;

typedef struct _REGISTER_SETTING
{
    BYTE Register;
    BYTE Value;
} REGISTER_SETTING, *PREGISTER_SETTING;


typedef class _NxpTfa9890Device
{
private:
    // WDF
    WDFDEVICE                   m_Device;
    WDFIOTARGET                 m_I2CIoTarget1;
	WDFIOTARGET                 m_I2CIoTarget2;
    WDFWAITLOCK                 m_I2CWaitLock;
    WDFINTERRUPT                m_Interrupt;

    // Sensor Operation
    bool                        m_PoweredOn;
    bool                        m_Started;
    ULONG                       m_Interval;

    bool                        m_FirstSample;
    VEC3D                       m_CachedThresholds;
    VEC3D                       m_LastSample;

    SENSOROBJECT                m_SensorInstance;

    //// Sensor Specific Properties
    PSENSOR_COLLECTION_LIST     m_pEnumerationProperties;
    PSENSOR_COLLECTION_LIST     m_pSensorProperties;

public:
    // WDF callbacks
    static EVT_WDF_DRIVER_DEVICE_ADD                OnDeviceAdd;
    static EVT_WDF_DEVICE_PREPARE_HARDWARE          OnPrepareHardware;
    static EVT_WDF_DEVICE_RELEASE_HARDWARE          OnReleaseHardware;
    static EVT_WDF_DEVICE_D0_ENTRY                  OnD0Entry;
    static EVT_WDF_DEVICE_D0_EXIT                   OnD0Exit;

    // CLX callbacks
    static EVT_SENSOR_DRIVER_START_SENSOR               OnStart;
    static EVT_SENSOR_DRIVER_STOP_SENSOR                OnStop;
    static EVT_SENSOR_DRIVER_GET_SUPPORTED_DATA_FIELDS  OnGetSupportedDataFields;
    static EVT_SENSOR_DRIVER_GET_PROPERTIES             OnGetProperties;
    static EVT_SENSOR_DRIVER_GET_DATA_FIELD_PROPERTIES  OnGetDataFieldProperties;
    static EVT_SENSOR_DRIVER_GET_DATA_INTERVAL          OnGetDataInterval;
    static EVT_SENSOR_DRIVER_SET_DATA_INTERVAL          OnSetDataInterval;
    static EVT_SENSOR_DRIVER_GET_DATA_THRESHOLDS        OnGetDataThresholds;
    static EVT_SENSOR_DRIVER_SET_DATA_THRESHOLDS        OnSetDataThresholds;
    static EVT_SENSOR_DRIVER_DEVICE_IO_CONTROL          OnIoControl;

    // Interrupt callbacks
    //static EVT_WDF_INTERRUPT_ISR       OnInterruptIsr;
    //static EVT_WDF_INTERRUPT_WORKITEM  OnInterruptWorkItem;

private:
    NTSTATUS                    GetData();

    // Helper function for OnPrepareHardware to initialize sensor to default properties
    NTSTATUS                    Initialize(_In_ WDFDEVICE Device, _In_ SENSOROBJECT SensorInstance);
    VOID                        DeInit();

    // Helper function for OnPrepareHardware to get resources from ACPI and configure the I/O target
    NTSTATUS                    ConfigureIoTarget(_In_ WDFCMRESLIST ResourceList,
                                                  _In_ WDFCMRESLIST ResourceListTranslated);

    // Helper function for OnD0Entry which sets up device to default configuration
    NTSTATUS                    PowerOn();
    NTSTATUS                    PowerOff();

} NxpTfa9890Device, *PNxpTfa9890Device;

// Set up accessor function to retrieve device context
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(NxpTfa9890Device, GetNxpTfa9890ContextFromSensorInstance);
