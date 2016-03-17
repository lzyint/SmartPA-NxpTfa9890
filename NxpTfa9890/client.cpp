//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of driver callback functions
//    from clx to TFA9890 accelerometer.
//
//Environment:
//
//   Windows User-Mode Driver Framework (UMDF)

#include "Device.h"
#include "TFA9890.h"

#include <timeapi.h>

#include "Client.tmh"


// Analog TFA9890 Unique ID
// {EF2C014C-DEBA-43F4-890D-978095684DD6}
DEFINE_GUID(GUID_TFA9890Device_UniqueID,
    0xef2c014c, 0xdeba, 0x43f4, 0x89, 0xd, 0x97, 0x80, 0x95, 0x68, 0x4d, 0xd6);

// {F0113F45-0810-49EA-BC6A-0FFD297C1266}
DEFINE_GUID(GUID_TFA9890Device_SubType,
	0xf0113f45, 0x810, 0x49ea, 0xbc, 0x6a, 0xf, 0xfd, 0x29, 0x7c, 0x12, 0x66);


// Helper function for initializing NxpTfa9890Device. Returns status.
inline NTSTATUS InitSensorCollection(
    _In_ ULONG CollectionListCount,
    _Outptr_ PSENSOR_COLLECTION_LIST *CollectionList,
    _In_ SENSOROBJECT SensorInstance)                   // SENSOROBJECT for sensor instance
{
    WDF_OBJECT_ATTRIBUTES MemoryAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&MemoryAttributes);
    MemoryAttributes.ParentObject = SensorInstance;

    WDFMEMORY MemoryHandle = NULL;
    ULONG MemorySize = SENSOR_COLLECTION_LIST_SIZE(CollectionListCount);
    NTSTATUS Status = WdfMemoryCreate(&MemoryAttributes,
                                      PagedPool,
                                      PA_POOL_TAG_ACCELEROMETER,
                                      MemorySize,
                                      &MemoryHandle,
                                      reinterpret_cast<PVOID*>(CollectionList));
    if (!NT_SUCCESS(Status) || nullptr == *CollectionList)
    {
        Status = STATUS_UNSUCCESSFUL;
        TraceError("ACC %!FUNC! WdfMemoryCreate failed %!STATUS!", Status);
        return Status;
    }

    SENSOR_COLLECTION_LIST_INIT(*CollectionList, MemorySize);
    (*CollectionList)->Count = CollectionListCount;
    return Status;
}


// This routine initializes the sensor to its default properties
NTSTATUS NxpTfa9890Device::Initialize(
    _In_ WDFDEVICE Device,            // WDFDEVICE object
    _In_ SENSOROBJECT SensorInstance) // SENSOROBJECT for each sensor instance
{
    SENSOR_FunctionEnter();

    //// Store device and instance
    m_Device = Device;
    m_SensorInstance = SensorInstance;
    m_Started = false;

    // Create Lock
    NTSTATUS Status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &(m_I2CWaitLock));
    if (!NT_SUCCESS(Status))
    {
        TraceError("ACC %!FUNC! WdfWaitLockCreate failed %!STATUS!", Status);
    }

    // Sensor Enumeration Properties
    if (NT_SUCCESS(Status))
    {
        Status = InitSensorCollection(SENSOR_ENUMERATION_PROPERTIES_COUNT, &m_pEnumerationProperties, SensorInstance);
        if (NT_SUCCESS(Status))
        {
            m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_TYPE].Key = DEVPKEY_Sensor_Type;
			InitPropVariantFromCLSID(GUID_SensorType_Custom,
                &(m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_TYPE].Value));

			m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_CATEGORY].Key = DEVPKEY_Sensor_Category;
			InitPropVariantFromCLSID(GUID_SensorCategory_Other,
				&(m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_CATEGORY].Value));
        
            m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_MANUFACTURER].Key = DEVPKEY_Sensor_Manufacturer;
            InitPropVariantFromString(SENSOR_PA_MANUFACTURER,
                &(m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_MANUFACTURER].Value));
        
            m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_MODEL].Key = DEVPKEY_Sensor_Model;
            InitPropVariantFromString(SENSOR_PA_MODEL,
                &(m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_MODEL].Value));
            
            m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_PERSISTENT_UNIQUE_ID].Key = DEVPKEY_Sensor_PersistentUniqueId;
            InitPropVariantFromCLSID(GUID_TFA9890Device_UniqueID,
                &(m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_PERSISTENT_UNIQUE_ID].Value));

			m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_SUBTYPE].Key = DEVPKEY_Sensor_VendorDefinedSubType;
			InitPropVariantFromCLSID(GUID_TFA9890Device_SubType,
				&(m_pEnumerationProperties->List[SENSOR_ENUMERATION_PROPERTY_SUBTYPE].Value));

        }
    }

	// Reset the FirstSample flag
    if (NT_SUCCESS(Status))
    {
        m_FirstSample = true;
    }
    // Trace to this function in case of failure
    else
    {
        TraceError("ACC %!FUNC! failed %!STATUS!", Status);
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

VOID NxpTfa9890Device::DeInit()
{
    // Delete lock
    if (NULL != m_I2CWaitLock)
    {
        WdfObjectDelete(m_I2CWaitLock);
        m_I2CWaitLock = NULL;
    }

    // Delete sensor instance
    if (NULL != m_SensorInstance)
    {
        WdfObjectDelete(m_SensorInstance);
    }
}

// This routine reads a single sample, compares threshold and pushes sample
// to sensor class extension. This routine is protected by the caller.
NTSTATUS NxpTfa9890Device::GetData() 
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to begin continously sampling the sensor.
NTSTATUS NxpTfa9890Device::OnStart(
    _In_ SENSOROBJECT SensorInstance)    // Sensor device object
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to stop continously sampling the sensor.
NTSTATUS NxpTfa9890Device::OnStop(
    _In_ SENSOROBJECT SensorInstance)   // Sensor device object
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to get supported data fields. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS NxpTfa9890Device::OnGetSupportedDataFields(
    _In_ SENSOROBJECT SensorInstance,          // Sensor device object
    _Inout_opt_ PSENSOR_PROPERTY_LIST pFields, // Pointer to a list of supported properties
    _Out_ PULONG pSize)                        // Number of bytes for the list of supported properties
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to get sensor properties. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS NxpTfa9890Device::OnGetProperties(
    _In_ SENSOROBJECT SensorInstance,                // Sensor device object
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties, // Pointer to a list of sensor properties
    _Out_ PULONG pSize)                              // Number of bytes for the list of sensor properties
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to get data field properties. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size 
// for the buffer, allocate buffer, then call the function again to retrieve 
// sensor information.
NTSTATUS NxpTfa9890Device::OnGetDataFieldProperties(
    _In_ SENSOROBJECT SensorInstance,                   // Sensor device object
    _In_ const PROPERTYKEY *pDataField,                 // Pointer to the propertykey of requested property
    _Inout_opt_ PSENSOR_COLLECTION_LIST pProperties,    // Pointer to a list of sensor properties
    _Out_ PULONG pSize)                                 // Number of bytes for the list of sensor properties
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to get sampling rate of the sensor.
NTSTATUS NxpTfa9890Device::OnGetDataInterval(
    _In_ SENSOROBJECT SensorInstance,   // Sensor device object
    _Out_ PULONG pDataRateMs)           // Sampling rate in milliseconds
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

	SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to set sampling rate of the sensor.
NTSTATUS NxpTfa9890Device::OnSetDataInterval(
    _In_ SENSOROBJECT SensorInstance, // Sensor device object
    _In_ ULONG DataRateMs)            // Sampling rate in milliseconds
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to get data thresholds. The typical usage is to call
// this function once with buffer pointer as NULL to acquire the required size
// for the buffer, allocate buffer, then call the function again to retrieve
// sensor information.
NTSTATUS NxpTfa9890Device::OnGetDataThresholds(
    _In_ SENSOROBJECT SensorInstance,                   // Sensor Device Object
    _Inout_opt_ PSENSOR_COLLECTION_LIST pThresholds,    // Pointer to a list of sensor thresholds
    _Out_ PULONG pSize)                                 // Number of bytes for the list of sensor thresholds
{

    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to set data thresholds.
NTSTATUS NxpTfa9890Device::OnSetDataThresholds(
    _In_ SENSOROBJECT SensorInstance,           // Sensor Device Object
    _In_ PSENSOR_COLLECTION_LIST pThresholds)   // Pointer to a list of sensor thresholds
{
    NTSTATUS Status = STATUS_SUCCESS;

    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(Status);
    return Status;
}

// Called by Sensor CLX to handle IOCTLs that clx does not support
NTSTATUS NxpTfa9890Device::OnIoControl(
    _In_ SENSOROBJECT /*SensorInstance*/, // WDF queue object
    _In_ WDFREQUEST /*Request*/,          // WDF request object
    _In_ size_t /*OutputBufferLength*/,   // number of bytes to retrieve from output buffer
    _In_ size_t /*InputBufferLength*/,    // number of bytes to retrieve from input buffer
    _In_ ULONG /*IoControlCode*/)         // IOCTL control code
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    SENSOR_FunctionEnter();

    SENSOR_FunctionExit(Status);
    return Status;
}
