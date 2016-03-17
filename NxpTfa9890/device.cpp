//Copyright (C) Microsoft Corporation, All Rights Reserved.
//
//Abstract:
//
//    This module contains the implementation of WDF callback functions 
//    for the NxpTfa9890 driver.
//
//Environment:
//
//   Windows User-Mode Driver Framework (UMDF)

#include "Device.h"

#include "Device.tmh"


// This routine is the AddDevice entry point for the custom sensor client
// driver. This routine is called by the framework in response to AddDevice
// call from the PnP manager. It will create and initialize the device object
// to represent a new instance of the sensor client.
NTSTATUS NxpTfa9890Device::OnDeviceAdd(
    _In_    WDFDRIVER /* Driver */,         // Supplies a handle to the driver object created in DriverEntry
    _Inout_ PWDFDEVICE_INIT pDeviceInit) // Supplies a pointer to a framework-allocated WDFDEVICE_INIT structure
{
    WDFDEVICE Device = nullptr;

    SENSOR_FunctionEnter();
	DLog("PA: Enter OnDeviceAdd.\n");

	WdfDeviceInitSetPowerPolicyOwnership(pDeviceInit, true);

    WDF_OBJECT_ATTRIBUTES FdoAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&FdoAttributes);

    // Initialize FDO (functional device object) attributes and set up file object with sensor extension
	NTSTATUS Status = SensorsCxDeviceInitConfig(pDeviceInit, &FdoAttributes, 0);
    if (!NT_SUCCESS(Status))
    {
        TraceError("ACC %!FUNC! SensorsCxDeviceInitConfig failed %!STATUS!", Status);
		DLog("PA: SensorsCxDeviceInitConfig failed %d\n", Status);
    }

    else // if (NT_SUCCESS(Status))
    {
        // Register the PnP callbacks with the framework.
        WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
        pnpPowerCallbacks.EvtDevicePrepareHardware = NxpTfa9890Device::OnPrepareHardware;
        pnpPowerCallbacks.EvtDeviceReleaseHardware = NxpTfa9890Device::OnReleaseHardware;
        pnpPowerCallbacks.EvtDeviceD0Entry = NxpTfa9890Device::OnD0Entry;
        pnpPowerCallbacks.EvtDeviceD0Exit = NxpTfa9890Device::OnD0Exit;
    
		WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &pnpPowerCallbacks);
    
        // Call the framework to create the device
		Status = WdfDeviceCreate(&pDeviceInit, &FdoAttributes, &Device);
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! WdfDeviceCreate failed %!STATUS!", Status);
			DLog("PA: WdfDeviceCreate failed %d\n", Status);//DebugLog
        }
    }

    if (NT_SUCCESS(Status))
    {
        // Register CLX callback function pointers
        SENSOR_CONTROLLER_CONFIG SensorConfig;
        SENSOR_CONTROLLER_CONFIG_INIT(&SensorConfig);
        SensorConfig.DriverIsPowerPolicyOwner = WdfUseDefault;
    
        SensorConfig.EvtSensorStart = NxpTfa9890Device::OnStart;
        SensorConfig.EvtSensorStop = NxpTfa9890Device::OnStop;
        SensorConfig.EvtSensorGetSupportedDataFields = NxpTfa9890Device::OnGetSupportedDataFields;
        SensorConfig.EvtSensorGetDataInterval = NxpTfa9890Device::OnGetDataInterval;
        SensorConfig.EvtSensorSetDataInterval = NxpTfa9890Device::OnSetDataInterval;
        SensorConfig.EvtSensorGetDataFieldProperties = NxpTfa9890Device::OnGetDataFieldProperties;
        SensorConfig.EvtSensorGetDataThresholds = NxpTfa9890Device::OnGetDataThresholds;
        SensorConfig.EvtSensorSetDataThresholds = NxpTfa9890Device::OnSetDataThresholds;
        SensorConfig.EvtSensorGetProperties = NxpTfa9890Device::OnGetProperties;
        SensorConfig.EvtSensorDeviceIoControl = NxpTfa9890Device::OnIoControl;
    
        // Initialize the sensor device with the Sensor CLX
        // This lets the CLX call the above callbacks when
        // necessary and allows applications to retrieve and
        // set device data.
        Status = SensorsCxDeviceInitialize(Device, &SensorConfig);
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! SensorsCxDeviceInitialize failed %!STATUS!", Status);
			DLog("PA: SensorsCxDeviceInitialize failed %d\n", Status);//DebugLog
        }
    }

    // Ensure device is disable-able
    // By default, devices enumerated by ACPI are not disable-able
    // Our accelerometer is enumerated by the ACPI so we must
    // explicitly make it disable-able.
    if (NT_SUCCESS(Status))
    {
        WDF_DEVICE_STATE DeviceState;
        WDF_DEVICE_STATE_INIT(&DeviceState);
        DeviceState.NotDisableable = WdfFalse;
        WdfDeviceSetDeviceState(Device, &DeviceState);
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// This routine is called by the framework when the PnP manager sends an
// IRP_MN_START_DEVICE request to the driver stack. This routine is
// responsible for performing operations that are necessary to make the
// driver's device operational (for e.g. mapping the hardware resources
// into memory).
NTSTATUS NxpTfa9890Device::OnPrepareHardware(
    _In_ WDFDEVICE Device,                  // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST ResourcesRaw,         // Supplies a handle to a collection of framework resource
                                            // objects. This collection identifies the raw (bus-relative) hardware
                                            // resources that have been assigned to the device.
    _In_ WDFCMRESLIST ResourcesTranslated)  // Supplies a handle to a collection of framework
                                            // resource objects. This collection identifies the translated
                                            // (system-physical) hardware resources that have been assigned to the
                                            // device. The resources appear from the CPU's point of view.
{
    PNxpTfa9890Device pDevice = nullptr;
    
    SENSOR_FunctionEnter();
	DLog("PA: Enter OnPrepareHardware.\n");

    // Create WDFOBJECT for the sensor
    WDF_OBJECT_ATTRIBUTES sensorAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&sensorAttributes, NxpTfa9890Device);

    // Register sensor instance with clx
    SENSOROBJECT SensorInstance = NULL;
    NTSTATUS Status = SensorsCxSensorCreate(Device, &sensorAttributes, &SensorInstance);
    if (!NT_SUCCESS(Status))
    {
        TraceError("ACC %!FUNC! SensorsCxSensorCreate failed %!STATUS!", Status);
		DLog("PA: SensorsCxSensorCreate failed %d\n", Status);//DebugLog
    }

    else // if (NT_SUCCESS(Status))
    {    
		pDevice = GetNxpTfa9890ContextFromSensorInstance(SensorInstance);
		if (nullptr == pDevice)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            TraceError("ACC %!FUNC! SensorsCxSensorCreate failed %!STATUS!", Status);
			DLog("PA: SensorsCxSensorCreate failed %d\n", Status);//DebugLog
        }
    }

    // Fill out sensor context
    if (NT_SUCCESS(Status))
    {
		Status = pDevice->Initialize(Device, SensorInstance);
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! Initialize device object failed %!STATUS!", Status);
			DLog("PA: Initialize device object failed failed %d\n", Status);//DebugLog        
		}
    }
        
    // Initialize sensor instance with clx
    if (NT_SUCCESS(Status))
    {
        SENSOR_CONFIG SensorConfig;
        SENSOR_CONFIG_INIT(&SensorConfig);
		SensorConfig.pEnumerationList = pDevice->m_pEnumerationProperties;
        Status = SensorsCxSensorInitialize(SensorInstance, &SensorConfig);
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! SensorsCxSensorInitialize failed %!STATUS!", Status);
			DLog("PA: SensorsCxSensorInitialize failed %d\n", Status);//DebugLog
        }
    }
    
    // ACPI and IoTarget configuration
    if (NT_SUCCESS(Status))
    {
		Status = pDevice->ConfigureIoTarget(ResourcesRaw, ResourcesTranslated);
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! Failed to configure IoTarget %!STATUS!", Status);
			DLog("PA: Failed to configure IoTarget %d\n", Status);//DebugLog        
		}
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// This routine is called by the framework when the PnP manager is revoking
// ownership of our resources. This may be in response to either
// IRP_MN_STOP_DEVICE or IRP_MN_REMOVE_DEVICE. This routine is responsible for
// performing cleanup of resources allocated in PrepareHardware callback.
// This callback is invoked before passing  the request down to the lower driver.
// This routine will also be invoked by the framework if the prepare hardware
// callback returns a failure.
NTSTATUS NxpTfa9890Device::OnReleaseHardware(
    _In_ WDFDEVICE Device,                      // Supplies a handle to the framework device object
    _In_ WDFCMRESLIST /*ResourcesTranslated*/)  // Supplies a handle to a collection of framework
                                                // resource objects. This collection identifies the translated
                                                // (system-physical) hardware resources that have been assigned to the
                                                // device. The resources appear from the CPU's point of view.
{
    PNxpTfa9890Device pAccDevice = nullptr;

    SENSOR_FunctionEnter();
	DLog("PA: Enter OnReleaseHardware.\n");

    // Get the sensor instance
    ULONG SensorInstanceCount = 1;
    SENSOROBJECT SensorInstance = NULL;
    NTSTATUS Status = SensorsCxDeviceGetSensorList(Device, &SensorInstance, &SensorInstanceCount);
    if (!NT_SUCCESS(Status) || 0 == SensorInstanceCount || NULL == SensorInstance)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
		DLog("PA: SensorsCxDeviceGetSensorList failed %d\n", Status);//DebugLog
	}

    else // if (NT_SUCCESS(Status))
    {
        pAccDevice = GetNxpTfa9890ContextFromSensorInstance(SensorInstance);
        if (nullptr == pAccDevice)
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("ACC %!FUNC! GetNxpTfa9890ContextFromSensorInstance failed %!STATUS!", Status);
			DLog("PA: GetNxpTfa9890ContextFromSensorInstance failed %d\n", Status);//DebugLog
		}
    }

    if (NT_SUCCESS(Status))
    {
        pAccDevice->DeInit();
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// This routine is invoked by the framework to program the device to goto 
// D0, which is the working state. The framework invokes callback every
// time the hardware needs to be (re-)initialized.  This includes after
// IRP_MN_START_DEVICE, IRP_MN_CANCEL_STOP_DEVICE, IRP_MN_CANCEL_REMOVE_DEVICE,
// and IRP_MN_SET_POWER-D0.
NTSTATUS NxpTfa9890Device::OnD0Entry(
    _In_  WDFDEVICE Device,                         // Supplies a handle to the framework device object
    _In_  WDF_POWER_DEVICE_STATE /*PreviousState*/) // WDF_POWER_DEVICE_STATE-typed enumerator that identifies
                                                    // the device power state that the device was in before this transition to D0
{
    PNxpTfa9890Device pAccDevice = nullptr;

    SENSOR_FunctionEnter();
	DLog("PA: Enter OnD0Entry.\n");

    // Get the sensor instance
    ULONG SensorInstanceCount = 1;
    SENSOROBJECT SensorInstance = NULL;
    NTSTATUS Status = SensorsCxDeviceGetSensorList(Device, &SensorInstance, &SensorInstanceCount);
    if (!NT_SUCCESS(Status) || 0 == SensorInstanceCount || NULL == SensorInstance)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
		DLog("PA: SensorsCxDeviceGetSensorList failed %d\n", Status);//DebugLog
	}

    // Get the device context
    else // if (NT_SUCCESS(Status))
    {
        pAccDevice = GetNxpTfa9890ContextFromSensorInstance(SensorInstance);
        if (nullptr == pAccDevice)
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("ACC %!FUNC! GetNxpTfa9890ContextFromSensorInstance failed %!STATUS!", Status);
			DLog("PA: GetNxpTfa9890ContextFromSensorInstance failed %d\n", Status);//DebugLog
		}
    }

    if (NT_SUCCESS(Status))
    {
        Status = pAccDevice->PowerOn();
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// This routine is invoked by the framework to program the device to go into
// a certain Dx state. The framework invokes callback every the the device is 
// leaving the D0 state, which happens when the device is stopped, when it is 
// removed, and when it is powered off. EvtDeviceD0Exit event callback must 
// perform any operations that are necessary before the specified device is 
// moved out of the D0 state.  If the driver needs to save hardware state 
// before the device is powered down, then that should be done here.
NTSTATUS NxpTfa9890Device::OnD0Exit(
    _In_ WDFDEVICE Device,                      // Supplies a handle to the framework device object
    _In_ WDF_POWER_DEVICE_STATE)/*TargetState*/ // Supplies the device power state which the device will be put
                                                // in once the callback is complete
{
    PNxpTfa9890Device pAccDevice = nullptr;

    SENSOR_FunctionEnter();
	DLog("PA: Enter OnD0Exit.\n");

    // Get the sensor instance
    ULONG SensorInstanceCount = 1;
    SENSOROBJECT SensorInstance = NULL;
    NTSTATUS Status = SensorsCxDeviceGetSensorList(Device, &SensorInstance, &SensorInstanceCount);
    if (!NT_SUCCESS(Status) || 0 == SensorInstanceCount || NULL == SensorInstance)
    {
        Status = STATUS_INVALID_PARAMETER;
        TraceError("ACC %!FUNC! SensorsCxDeviceGetSensorList failed %!STATUS!", Status);
		DLog("PA: SensorsCxDeviceGetSensorList failed %d\n", Status);//DebugLog
	}

    // Get the device context
    else // if (NT_SUCCESS(Status))
    {
        pAccDevice = GetNxpTfa9890ContextFromSensorInstance(SensorInstance);
        if (nullptr == pAccDevice)
        {
            Status = STATUS_INVALID_PARAMETER;
            TraceError("ACC %!FUNC! GetNxpTfa9890ContextFromSensorInstance failed %!STATUS!", Status);
			DLog("PA: GetNxpTfa9890ContextFromSensorInstance failed %d\n", Status);//DebugLog
		}
    }

    if (NT_SUCCESS(Status))
    {
        //Status = pAccDevice->PowerOff();
    }

    SENSOR_FunctionExit(Status);
    return Status;
}

// Get the HW resource from the ACPI, then configure and store the IoTarget
NTSTATUS NxpTfa9890Device::ConfigureIoTarget(
    _In_ WDFCMRESLIST ResourcesRaw,         // Supplies a handle to a collection of framework resource
                                            // objects. This collection identifies the raw (bus-relative) hardware
                                            // resources that have been assigned to the device.
    _In_ WDFCMRESLIST ResourcesTranslated)  // Supplies a handle to a collection of framework
                                            // resource objects. This collection identifies the translated
                                            // (system-physical) hardware resources that have been assigned to the
                                            // device. The resources appear from the CPU's point of view.
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG I2CConnectionResourceCount = 0;
	LARGE_INTEGER I2CConnId[2] = { {}, {} };


    DECLARE_UNICODE_STRING_SIZE(deviceName1, RESOURCE_HUB_PATH_SIZE);
	DECLARE_UNICODE_STRING_SIZE(deviceName2, RESOURCE_HUB_PATH_SIZE);
	
    SENSOR_FunctionEnter();
	DLog("PA: Enter ConfigureIoTarget.\n");

    // Get hardware resource from ACPI and set up IO target
    ULONG ResourceCount = WdfCmResourceListGetCount(ResourcesTranslated);
    for (ULONG i = 0; i < ResourceCount; i++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR DescriptorRaw = WdfCmResourceListGetDescriptor(ResourcesRaw, i);
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);
        switch (Descriptor->Type) 
        {
            // Check we have I2C bus assigned in ACPI
            case CmResourceTypeConnection:
                TraceInformation("ACC %!FUNC! I2C resource found.");
				DLog("PA: I2C resource found\n");
                if (Descriptor->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL &&
                    Descriptor->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_I2C) 
                {
                    I2CConnId[i].LowPart = Descriptor->u.Connection.IdLowPart;
                    I2CConnId[i].HighPart = Descriptor->u.Connection.IdHighPart;
                    I2CConnectionResourceCount++;
                }
                break;
    

            default:
                break;
        }
    }

    if (NT_SUCCESS(Status) && I2CConnectionResourceCount != 2)
    {
        Status = STATUS_UNSUCCESSFUL;
        TraceError("ACC %!FUNC! Did not find I2C resource! %!STATUS!", Status);
		DLog("PA: Did not find I2C resource %d\n", Status);//DebugLog
	}

    // Set up I2C I/O target. Issued with I2C R/W transfers
    if (NT_SUCCESS(Status))
    {
        m_I2CIoTarget1 = NULL;
		m_I2CIoTarget2 = NULL;
        Status = WdfIoTargetCreate(m_Device, WDF_NO_OBJECT_ATTRIBUTES, &m_I2CIoTarget1);
        if (!NT_SUCCESS(Status))
        {
            TraceError("ACC %!FUNC! WdfIoTargetCreate failed! %!STATUS!", Status);
			DLog("PA: WdfIoTargetCreate failed %d\n", Status);//DebugLog
		}
		Status = WdfIoTargetCreate(m_Device, WDF_NO_OBJECT_ATTRIBUTES, &m_I2CIoTarget2);
		if (!NT_SUCCESS(Status))
		{
			TraceError("ACC %!FUNC! WdfIoTargetCreate failed! %!STATUS!", Status);
			DLog("PA: WdfIoTargetCreate failed %d\n", Status);//DebugLog
		}
    }

	// Setup Target string (\\\\.\\RESOURCE_HUB\\<ConnID from ResHub>
	if (NT_SUCCESS(Status))
	{
		Status = StringCbPrintfW(deviceName1.Buffer, RESOURCE_HUB_PATH_SIZE, L"%s\\%0*I64x", RESOURCE_HUB_DEVICE_NAME, static_cast<unsigned int>(sizeof(LARGE_INTEGER) * 2), I2CConnId[0].QuadPart);
		deviceName1.Length = _countof(deviceName1_buffer);

		DLog("PA: Device Name 1: %ws \n", deviceName1.Buffer); //DebugLog

		if (!NT_SUCCESS(Status))
		{
			TraceError("ACC %!FUNC! RESOURCE_HUB_CREATE_PATH_FROM_ID failed!");
			DLog("PA: RESOURCE_HUB_CREATE_PATH_FROM_ID failed %d\n", Status);//DebugLog
		}

		Status = StringCbPrintfW(deviceName2.Buffer, RESOURCE_HUB_PATH_SIZE, L"%s\\%0*I64x", RESOURCE_HUB_DEVICE_NAME, static_cast<unsigned int>(sizeof(LARGE_INTEGER) * 2), I2CConnId[1].QuadPart);
		deviceName2.Length = _countof(deviceName2_buffer);

		DLog("PA: Device Name 2: %ws \n", deviceName2.Buffer); //DebugLog

		if (!NT_SUCCESS(Status))
		{
			TraceError("ACC %!FUNC! RESOURCE_HUB_CREATE_PATH_FROM_ID failed!");
			DLog("PA: RESOURCE_HUB_CREATE_PATH_FROM_ID failed %d\n", Status);//DebugLog
		}
	}

	// Connect to I2C target
	if (NT_SUCCESS(Status))
	{
		WDF_IO_TARGET_OPEN_PARAMS OpenParams1;
		WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParams1, &deviceName1, FILE_ALL_ACCESS);

		Status = WdfIoTargetOpen(m_I2CIoTarget1, &OpenParams1);
		if (!NT_SUCCESS(Status))
		{
			TraceError("ACC %!FUNC! WdfIoTargetOpen failed! %!STATUS!", Status);
			DLog("PA: WdfIoTargetOpen failed %d\n", Status);//DebugLog
		}

		WDF_IO_TARGET_OPEN_PARAMS OpenParams2;
		WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParams2, &deviceName2, FILE_ALL_ACCESS);

		Status = WdfIoTargetOpen(m_I2CIoTarget2, &OpenParams2);
		if (!NT_SUCCESS(Status))
		{
			TraceError("ACC %!FUNC! WdfIoTargetOpen failed! %!STATUS!", Status);
			DLog("PA: WdfIoTargetOpen failed %d\n", Status);//DebugLog
		}
	}

    SENSOR_FunctionExit(Status);
    return Status;
}

// Write the default device configuration to the device
NTSTATUS NxpTfa9890Device::PowerOn()
{
	DLog("PA: Enter PowerOn.\n");

    NTSTATUS Status = STATUS_SUCCESS;

    WdfWaitLockAcquire(m_I2CWaitLock, NULL);

	WORD registerBits = 0;

	//I2C address 34
	registerBits = TFA9890_I2S_CONTROL_BYPASS;
	Status = I2CSensorWriteRegister(m_I2CIoTarget1, TFA9890_I2S_CONTROL, (BYTE*)&registerBits, sizeof(registerBits));
	DLog("PA: I2CSensorWriteRegister to 0x%02x with value 0x%02x\n", TFA9890_I2S_CONTROL, registerBits);//DebugLog
	if (!NT_SUCCESS(Status))
	{
		TraceError("ACC %!FUNC! I2CSensorWriteRegister from 0x%02x failed! %!STATUS!", TFA9890_I2S_CONTROL, Status);
		DLog("PA: I2CSensorWriteRegister from 0x%02x failed %d\n", TFA9890_I2S_CONTROL, Status);//DebugLog
		WdfWaitLockRelease(m_I2CWaitLock);
		return Status;
	}

	registerBits = TFA9890_SYSTEM_CONTROL_BYPASS_1;
	Status = I2CSensorWriteRegister(m_I2CIoTarget1, TFA9890_SYSTEM_CONTROL, (BYTE*)&registerBits, sizeof(registerBits));
	DLog("PA: I2CSensorWriteRegister to 0x%02x with value 0x%02x\n", TFA9890_SYSTEM_CONTROL, registerBits);//DebugLog
	if (!NT_SUCCESS(Status))
	{
		TraceError("ACC %!FUNC! I2CSensorWriteRegister from 0x%02x failed! %!STATUS!", TFA9890_SYSTEM_CONTROL, Status);
		DLog("PA: I2CSensorWriteRegister from 0x%02x failed %d\n", TFA9890_SYSTEM_CONTROL, Status);//DebugLog
		WdfWaitLockRelease(m_I2CWaitLock);
		return Status;
	}

	registerBits = TFA9890_SYSTEM_CONTROL_BYPASS_2;
	Status = I2CSensorWriteRegister(m_I2CIoTarget1, TFA9890_SYSTEM_CONTROL, (BYTE*)&registerBits, sizeof(registerBits));
	DLog("PA: I2CSensorWriteRegister to 0x%02x with value 0x%02x\n", TFA9890_SYSTEM_CONTROL, registerBits);//DebugLog
	if (!NT_SUCCESS(Status))
	{
		TraceError("ACC %!FUNC! I2CSensorWriteRegister from 0x%02x failed! %!STATUS!", TFA9890_SYSTEM_CONTROL, Status);
		DLog("PA: I2CSensorWriteRegister from 0x%02x failed %d\n", TFA9890_SYSTEM_CONTROL, Status);//DebugLog
		WdfWaitLockRelease(m_I2CWaitLock);
		return Status;
	}

	//I2C address 36
	registerBits = TFA9890_I2S_CONTROL_BYPASS;
	Status = I2CSensorWriteRegister(m_I2CIoTarget2, TFA9890_I2S_CONTROL, (BYTE*)&registerBits, sizeof(registerBits));
	DLog("PA: I2CSensorWriteRegister to 0x%02x with value 0x%02x\n", TFA9890_I2S_CONTROL, registerBits);//DebugLog
	if (!NT_SUCCESS(Status))
	{
		TraceError("ACC %!FUNC! I2CSensorWriteRegister from 0x%02x failed! %!STATUS!", TFA9890_I2S_CONTROL, Status);
		DLog("PA: I2CSensorWriteRegister from 0x%02x failed %d\n", TFA9890_I2S_CONTROL, Status);//DebugLog
		WdfWaitLockRelease(m_I2CWaitLock);
		return Status;
	}

	registerBits = TFA9890_SYSTEM_CONTROL_BYPASS_1;
	Status = I2CSensorWriteRegister(m_I2CIoTarget2, TFA9890_SYSTEM_CONTROL, (BYTE*)&registerBits, sizeof(registerBits));
	DLog("PA: I2CSensorWriteRegister to 0x%02x with value 0x%02x\n", TFA9890_SYSTEM_CONTROL, registerBits);//DebugLog
	if (!NT_SUCCESS(Status))
	{
		TraceError("ACC %!FUNC! I2CSensorWriteRegister from 0x%02x failed! %!STATUS!", TFA9890_SYSTEM_CONTROL, Status);
		DLog("PA: I2CSensorWriteRegister from 0x%02x failed %d\n", TFA9890_SYSTEM_CONTROL, Status);//DebugLog
		WdfWaitLockRelease(m_I2CWaitLock);
		return Status;
	}

	registerBits = TFA9890_SYSTEM_CONTROL_BYPASS_2;
	Status = I2CSensorWriteRegister(m_I2CIoTarget2, TFA9890_SYSTEM_CONTROL, (BYTE*)&registerBits, sizeof(registerBits));
	DLog("PA: I2CSensorWriteRegister to 0x%02x with value 0x%02x\n", TFA9890_SYSTEM_CONTROL, registerBits);//DebugLog
	if (!NT_SUCCESS(Status))
	{
		TraceError("ACC %!FUNC! I2CSensorWriteRegister from 0x%02x failed! %!STATUS!", TFA9890_SYSTEM_CONTROL, Status);
		DLog("PA: I2CSensorWriteRegister from 0x%02x failed %d\n", TFA9890_SYSTEM_CONTROL, Status);//DebugLog
		WdfWaitLockRelease(m_I2CWaitLock);
		return Status;
	}


    WdfWaitLockRelease(m_I2CWaitLock);

    //InitPropVariantFromUInt32(SensorState_Idle, &(m_pSensorProperties->List[SENSOR_PROPERTY_STATE].Value));
    m_PoweredOn = true;

    return Status;
}

NTSTATUS NxpTfa9890Device::PowerOff()
{
	DLog("PA: Enter PowerOff.\n");

	NTSTATUS Status = STATUS_SUCCESS;

    m_PoweredOn = false;


    return Status;
}
