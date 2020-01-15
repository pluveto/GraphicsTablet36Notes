#include <ntddk.h>
#include <wdf.h>
#include <vhf.h>

typedef struct _FDO_DATA
{
	WDFWMIINSTANCE              WmiDeviceArrivalEvent;
	BOOLEAN                     WmiPowerDeviceEnableRegistered;
} FDO_DATA, * PFDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH(FDO_DATA, ToasterFdoGetData);
WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fdoAttributes,
	FDO_DATA);

typedef struct _VIRTUAL_GT_DEVICE_CONTEXT {
	VHFHANDLE VhfHandle;
} VIRTUAL_GT_DEVICE_CONTEXT, *PVIRTUAL_GT_DEVICE_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(VIRTUAL_GT_DEVICE_CONTEXT, VirtualGTGetDeviceContext);

NTSTATUS VirtualKeyboardCreateDevice(PWDFDEVICE_INIT DeviceInit) {
	PAGED_CODE();

	TraceEnterFunc();

	// デバイス作成
	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, VIRTUAL_GT_DEVICE_CONTEXT);
	deviceAttributes.EvtCleanupCallback = VirtualKeyboardDeviceEvtCleanupCallback;

	WDFDEVICE device;
	NTSTATUS status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

	if (!NT_SUCCESS(status)) {
		TraceErrorStatus("WdfDeviceCreate", status);
		return status;
	}

	PVIRTUAL_KEYBOARD_DEVICE_CONTEXT deviceContext = VirtualKeyboardGetDeviceContext(device);

	// デバイスコンテキストの中身を全部 null にしておく（変なところで Cleanup 走ったら困るので）
	RtlZeroMemory(deviceContext, sizeof(VIRTUAL_KEYBOARD_DEVICE_CONTEXT));

	// リモート HID 作成
	VHF_CONFIG vhfConfig;
	VHF_CONFIG_INIT(&vhfConfig, WdfDeviceWdmGetDeviceObject(device), sizeof(ReportDescriptor), ReportDescriptor);
	vhfConfig.VhfClientContext = deviceContext; // EvtVhfCleanup で使う
	vhfConfig.EvtVhfCleanup = VirtualKeyboardEvtVhfCleanup;

	status = VhfCreate(&vhfConfig, &deviceContext->VhfHandle);

	if (!NT_SUCCESS(status)) {
		TraceErrorStatus("VhfCreate", status);
		return status;
	}

	status = VhfStart(deviceContext->VhfHandle);

	if (!NT_SUCCESS(status)) {
		TraceErrorStatus("VhfStart", status);
		VhfDelete(deviceContext->VhfHandle, FALSE);
		return status;
	}

	// タイマー作成
	status = VirtualKeyboardDeviceCreateKeyDownTimer(device);
	if (NT_SUCCESS(status)) {
		status = VirtualKeyboardDeviceCreateKeyUpTimer(device);
		if (NT_SUCCESS(status)) {
			WdfTimerStart(deviceContext->KeyDownTimer, WDF_REL_TIMEOUT_IN_MS(KEY_DOWN_TIMER_DUE_TIME_MS));
		}
	}

	return STATUS_SUCCESS;
}

VOID VirtualKeyboardDeviceEvtCleanupCallback(_In_ WDFOBJECT Object) {
	PAGED_CODE();

	TraceEnterFunc();

	// デバイスを削除
	PVIRTUAL_KEYBOARD_DEVICE_CONTEXT deviceContext = VirtualKeyboardGetDeviceContext(Object);
	VHFHANDLE hVhf = deviceContext->VhfHandle;
	if (hVhf != NULL) {
		deviceContext->VhfHandle = NULL;
		VhfDelete(hVhf, FALSE);
	}
}

VOID VirtualKeyboardEvtVhfCleanup(_In_ PVOID VhfClientContext) {
	TraceEnterFunc();

	PVIRTUAL_KEYBOARD_DEVICE_CONTEXT deviceContext = (PVIRTUAL_KEYBOARD_DEVICE_CONTEXT)VhfClientContext;
	deviceContext->VhfHandle = NULL;
}
