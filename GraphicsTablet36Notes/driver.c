

#include <ntddk.h>
#include <wdf.h>
#include <hidclass.h>
#pragma comment(lib,"vhfkm.lib")
#include <vhf.h>
#include <initguid.h>
#include<hidport.h>
#include "trace.h"


#define DEVICE_OBJECT_NAME  L"\\Device\\GT36NotesDeviceObject"
#define DEVICE_LINK_NAME    L"\\??\\GT36NotesSymLink"

#define BUFFER_FILE_NAME L"\\SystemRoot\\Temp\\bbRemoteBuffer"

#define FILE_NAME_LEN 20
#define MAX_PATH 260
WDFDEVICE vhfDevice;
NTSTATUS VhfSourceCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit);
NTSTATUS SubmitReport(PUCHAR buffer, int);
NTSTATUS PenEvtDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit);
typedef UCHAR HID_REPORT_DESCRIPTOR, * PHID_REPORT_DESCRIPTOR;

// 设备上下文, 一个自定义结构, 想存啥存啥
// 注意: 它在 VhfSourceCreateDevice 完成了初始化
typedef struct _DEVICE_CONTEXT
{
	// 虚拟设备
	VHFHANDLE				VhfHandle;
} DEVICE_CONTEXT, * PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

DEFINE_GUID(GUID_HID,
	0xd0f38147, 0x08dc, 0x47bc, 0xb7, 0x39, 0x44, 0xf1, 0x0e, 0x94, 0x11, 0x17);
#define REPORTID_PEN  0x01
#define REPORTID_MOUSE 2

UCHAR PenHIDReportDescriptor[] = {
	0x05, 0x0d,                         // USAGE_PAGE (Digitizers)          0
	0x09, 0x02,                         // USAGE (Pen)                      2
	0xa1, 0x01,                         // COLLECTION (Application)         4
	0x85, REPORTID_PEN,                 //   REPORT_ID (Pen)                6
	0x09, 0x20,                         //   USAGE (Stylus)                 8
	0xa1, 0x00,                         //   COLLECTION (Physical)          10
	0x09, 0x42,                         //     USAGE (Tip Switch)           12
	0x09, 0x44,                         //     USAGE (Barrel Switch)        14
	0x09, 0x3c,                         //     USAGE (Invert)               16
	0x09, 0x45,                         //     USAGE (Eraser Switch)        18
	0x15, 0x00,                         //     LOGICAL_MINIMUM (0)          20
	0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)          22
	0x75, 0x01,                         //     REPORT_SIZE (1)              24
	0x95, 0x04,                         //     REPORT_COUNT (4)             26
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)         28
	0x95, 0x01,                         //     REPORT_COUNT (1)             30
	0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)         32
	0x09, 0x32,                         //     USAGE (In Range)             34
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)         36
	0x95, 0x02,                         //     REPORT_COUNT (2)             38
	0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)         40
	0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop) 42
	0x09, 0x30,                         //     USAGE (X)                    44
	0x75, 0x10,                         //     REPORT_SIZE (16)             46
	0x95, 0x01,                         //     REPORT_COUNT (1)             48
	0xa4,                               //     PUSH                         50
	0x55, 0x0d,                         //     UNIT_EXPONENT (-3)           
	0x65, 0x13,                         //     UNIT (Inch,EngLinear)   
	0x35, 0x00,                         //     PHYSICAL_MINIMUM (0)         55
	0x46, 0x4c, 0x20,                   //     PHYSICAL_MAXIMUM (8268)      57
	0x26,0x4c, 0x20,
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)         63
	0x09, 0x31,                         //     USAGE (Y)                    65
	0x35, 0x00,                         //     PHYSICAL_MINIMUM (0)         55
	0x46, 0xc3, 0x16,                   //     PHYSICAL_MAXIMUM (5827)      67
	 0x26, 0xc3, 0x16,
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)         73
	0xb4,                               //     POP                          75
	0x05, 0x0d,                         //     USAGE_PAGE (Digitizers)      76
	0x09, 0x30,                         //     USAGE (Tip Pressure)         78
	0x26, 0xff, 0x00,                   //     LOGICAL_MAXIMUM (255)        80
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)         83
	0x75, 0x08,                         //     REPORT_SIZE (8)              85
	0x09, 0x3d,                         //     USAGE (X Tilt)               87
	0x15, 0x81,                         //     LOGICAL_MINIMUM (-127)       89
	0x25, 0x7f,                         //     LOGICAL_MAXIMUM (127)        91
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)         93
	0x09, 0x3e,                         //     USAGE (Y Tilt)               95
	0x15, 0x81,                         //     LOGICAL_MINIMUM (-127)       97
	0x25, 0x7f,                         //     LOGICAL_MAXIMUM (127)        99
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)         101/103
	0xc0,                               //   END_COLLECTION                 0
	0xc0
};

typedef struct
{
	UINT8  reportId;                                 // Report ID = 0x01 (1)
													   // Collection: CA:Pen CP:Stylus
	UINT8  DIG_PenStylusTipSwitch : 1;               // Usage 0x000D0042: Tip Switch, Value = 0 to 1
	UINT8  DIG_PenStylusBarrelSwitch : 1;            // Usage 0x000D0044: Barrel Switch, Value = 0 to 1
	UINT8  DIG_PenStylusInvert : 1;                  // Usage 0x000D003C: Invert, Value = 0 to 1
	UINT8  DIG_PenStylusEraser : 1;                  // Usage 0x000D0045: Eraser, Value = 0 to 1
	UINT8 : 1;                                      // Pad
	UINT8  DIG_PenStylusInRange : 1;                 // Usage 0x000D0032: In Range, Value = 0 to 1
	UINT8 : 1;                                      // Pad
	UINT8 : 1;                                      // Pad
	UINT16 GD_PenStylusX;                            // Usage 0x00010030: X, Value = 0 to 21240, Physical = Value x 275 / 708 in 10⁻³ inch³ units
	UINT16 GD_PenStylusY;                            // Usage 0x00010031: Y, Value = 0 to 15980, Physical = Value x 91 / 235 in 10⁻³ inch³ units
	UINT16 DIG_PenStylusTipPressure;                 // Usage 0x000D0030: Tip Pressure, Value = 0 to 255
	INT8   DIG_PenStylusXTilt;                       // Usage 0x000D003D: X Tilt, Value = -127 to 127
	INT8   DIG_PenStylusYTilt;                       // Usage 0x000D003E: Y Tilt, Value = -127 to 127
} PenReportStruct;
typedef struct
{
	UINT8  reportId;                                 // Report ID = 0x02 (2)
													   // Collection: CA:Mouse CP:Pointer
	UINT8  BTN_MousePointerButton1 : 1;              // Usage 0x00090001: Button 1 Primary/trigger, Value = 0 to 1
	UINT8  BTN_MousePointerButton2 : 1;              // Usage 0x00090002: Button 2 Secondary, Value = 0 to 1
	UINT8 : 1;                                      // Pad
	UINT8 : 1;                                      // Pad
	UINT8 : 1;                                      // Pad
	UINT8 : 1;                                      // Pad
	UINT8 : 1;                                      // Pad
	UINT8 : 1;                                      // Pad
	INT8   GD_MousePointerX;                         // Usage 0x00010030: X, Value = -127 to 127
	INT8   GD_MousePointerY;                         // Usage 0x00010031: Y, Value = -127 to 127
} MouseReportStruct;
HID_DESCRIPTOR gHidDescriptor =
{
	sizeof(HID_DESCRIPTOR),             //bLength
	HID_HID_DESCRIPTOR_TYPE,            //bDescriptorType
	HID_REVISION,                       //bcdHID
	0,                                  //bCountry - not localized
	1,                                  //bNumDescriptors
	{                                   //DescriptorList[0]
		HID_REPORT_DESCRIPTOR_TYPE,     //bReportType
		sizeof(PenReportStruct)       //wReportLength
	}
};

VOID
VhfSourceDeviceCleanup(
	_In_ WDFOBJECT DeviceObject
)
{
	PDEVICE_CONTEXT deviceContext;

	PAGED_CODE();

	deviceContext = DeviceGetContext(DeviceObject);

	if (deviceContext->VhfHandle != WDF_NO_HANDLE)
	{
		VhfDelete(deviceContext->VhfHandle, TRUE);
	}

}
VOID DriverUnload(IN WDFDRIVER Driver)

{
	TraceEnterFunc();
	UNREFERENCED_PARAMETER(Driver);
	KdPrint(("unload.\r\n"));

}


// 参考了 https://www.cnblogs.com/lsh123/p/7354573.html

// 根据数据提交 HID 报告. 长度是固定的.
// type: 0 pen, 1 mouse
NTSTATUS SubmitReport(PUCHAR buffer, int type)
{
	TraceEnterFunc();
	PDEVICE_CONTEXT deviceContext = DeviceGetContext(vhfDevice);
	if (deviceContext == NULL) {
		KdPrint(("SubmitReport: deviceContext not initialized.\r\n"));
		return RPC_NT_NULL_REF_POINTER;
	}	
	// 配置报告内容
	HID_XFER_PACKET report;
	RtlZeroMemory(&report, sizeof(HID_XFER_PACKET));
	report.reportBuffer = buffer;
	report.reportBufferLen = type? sizeof(MouseReportStruct) :sizeof(PenReportStruct);
	report.reportId = 0;
	return VhfReadReportSubmit(deviceContext->VhfHandle, &report);
}

/*
	当设备出现后执行该事件
	A driver's EvtDriverDeviceAdd event callback function performs
	device initialization operations when the Plug and Play (PnP)
	manager reports the existence of a device.
*/
NTSTATUS _Function_class_(EVT_WDF_DRIVER_DEVICE_ADD) MyEvtDriverDeviceAdd(_In_	WDFDRIVER Driver, _Inout_	PWDFDEVICE_INIT DeviceInit)

{
	TraceEnterFunc();
	NTSTATUS status;
	status = VhfSourceCreateDevice(DeviceInit);
	return status;
	//--
}
/*
	在 EvtDriverContextCleanup 或 EvtDriverContextCleanup 中释放 DriverEntry 中创建的资源.
*/
NTSTATUS MyEvtDriverUnload(_In_	WDFDRIVER Driver) _Function_class_(EVT_WDF_DRIVER_UNLOAD)

{
	TraceEnterFunc();

	UNREFERENCED_PARAMETER(Driver);	
	PAGED_CODE();
	return STATUS_SUCCESS;
}
NTSTATUS MyEvtDriverContextCleanup (_In_ WDFOBJECT Object) _Function_class_(EVT_WDF_OBJECT_CONTEXT_CLEANUP)
{
	TraceEnterFunc();
	//WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)Object));
	return STATUS_SUCCESS;

}
// 处理封装了的 IRP_MJ_WRITE

VOID MyFileEvtIoWrite(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t Length) _Function_class_(EVT_WDF_IO_QUEUE_IO_WRITE)

{
	NTSTATUS status;
	TraceEnterFunc();
	PIRP Irp = WdfRequestWdmGetIrp(Request);
	PIO_STACK_LOCATION  IrpSp = IoGetCurrentIrpStackLocation(Irp);
	ULONG_PTR bytesWritten = 0;
	size_t bufferLength = IrpSp->Parameters.Write.Length;
	PUCHAR inputBuffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
	//FdoData = FdoGetData(WdfIoQueueGetDevice(Queue));
	
	// 无数据时不处理
	if (bufferLength == 0) {
		status = STATUS_INVALID_DEVICE_REQUEST;
		KdPrint(("No data input (bufferLength==0)"));
		WdfRequestComplete(Request, status);
		return;
	}
	KdPrint(("LENGTH: %lld\r\n", bufferLength));
	if (inputBuffer == NULL) {
		status = STATUS_INVALID_DEVICE_REQUEST;
		KdPrint(("InputData null"));
		WdfRequestComplete(Request, status);
		return;
	}
	if (inputBuffer[0] == 0x01) { //pen
		if (bufferLength != sizeof(PenReportStruct)) {
			status = STATUS_INVALID_DEVICE_REQUEST;
			KdPrint(("expected size: %d\r\n", sizeof(PenReportStruct)));
			KdPrintBinary(inputBuffer, (UINT8)bufferLength);
			WdfRequestComplete(Request, status);
			return;
		}
		KdPrintBinary(inputBuffer, (UINT8)bufferLength);
		SubmitReport(inputBuffer, 0);
	}
	else {//mouse
		if (bufferLength != sizeof(MouseReportStruct)) {
			status = STATUS_INVALID_DEVICE_REQUEST;
			KdPrint(("expected size: %d\r\n", sizeof(MouseReportStruct)));
			KdPrintBinary(inputBuffer, (UINT8)bufferLength);
			WdfRequestComplete(Request, status);
			return;
		}
		KdPrintBinary(inputBuffer, (UINT8)bufferLength);
		SubmitReport(inputBuffer, 1);
	}
	
	bytesWritten = bufferLength;
	status = STATUS_SUCCESS;

	WdfRequestCompleteWithInformation(Request, status, bytesWritten);

}

VOID MyShutdownNotification(_In_ WDFDEVICE Device) _Function_class_(EVT_WDF_DEVICE_SHUTDOWN_NOTIFICATION)

{
	TraceEnterFunc();

	UNREFERENCED_PARAMETER(Device);
	return;
}
/*++
Routine Description:
	This I/O in-process callback is called in the calling threads context/address
	space before the request is subjected to any framework locking or queueing
	scheme based on the device pnp/power or locking attributes set by the
	driver. The process context of the calling app is guaranteed as long as
	this driver is a top-level driver and no other filter driver is attached
	to it.
	This callback is only required if you are handling method-neither IOCTLs,
	or want to process requests in the context of the calling process.
	Driver developers should avoid defining neither IOCTLs and access user
	buffers, and use much safer I/O tranfer methods such as buffered I/O
	or direct I/O.
Arguments:
	Device - Handle to a framework device object.
	Request - Handle to a framework request object. Framework calls
			  PreProcess callback only for Read/Write/ioctls and internal
			  ioctl requests.
Return Value:
	VOID
--*/
VOID MyEvtDeviceIoInCallerContext(_In_ WDFDEVICE Device, _In_ WDFREQUEST Request)
{
	TraceEnterFunc();

	typedef struct _REQUEST_CONTEXT {

		WDFMEMORY InputMemoryBuffer;
		WDFMEMORY OutputMemoryBuffer;

	} REQUEST_CONTEXT, * PREQUEST_CONTEXT;

	NTSTATUS                   status = STATUS_SUCCESS;
	PREQUEST_CONTEXT            reqContext = NULL;
	WDF_OBJECT_ATTRIBUTES           attributes;
	WDF_REQUEST_PARAMETERS  params;
	size_t              inBufLen, outBufLen;
	PVOID              inBuf, outBuf;

	PAGED_CODE();

	WDF_REQUEST_PARAMETERS_INIT(&params);

	WdfRequestGetParameters(Request, &params);
}
NTSTATUS
NInputGetDeviceAttributes(
	__in WDFREQUEST Request
) {
	TraceEnterFunc();

	PHID_DEVICE_ATTRIBUTES deviceAttributes = NULL;
	NTSTATUS status = WdfRequestRetrieveOutputBuffer(Request,
		sizeof(HID_DEVICE_ATTRIBUTES),
		&deviceAttributes,
		NULL);
	deviceAttributes->Size = sizeof(HID_DEVICE_ATTRIBUTES);
	deviceAttributes->VendorID = 0x08;
	deviceAttributes->ProductID = 0x09;
	deviceAttributes->VersionNumber = 1;

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, sizeof(HID_DEVICE_ATTRIBUTES));
	return status;

}
// Finds the HID descriptor and copies it into the buffer provided by the 

NTSTATUS NInputGetHidDescriptor(
	__in WDFDEVICE Device,
	__in WDFREQUEST Request
) {
	TraceEnterFunc();

	NTSTATUS            status = STATUS_SUCCESS;
	size_t              bytesToCopy = 0;
	WDFMEMORY           memory = { 0 };

	UNREFERENCED_PARAMETER(Device);

	//
	// This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
	// will correctly retrieve buffer from Irp->UserBuffer. 
	// Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl.
	//
	status = WdfRequestRetrieveOutputMemory(Request, &memory);
	if (!NT_SUCCESS(status)) {
		KdPrint(("failed to retrieve output buffer status=0x%x", status));
		return status;
	}

	//
	// Use hardcoded "HID Descriptor" 
	//
	bytesToCopy = sizeof(gHidDescriptor);
	status = WdfMemoryCopyFromBuffer(memory,
		0, // Offset
		(PUCHAR)&gHidDescriptor,
		bytesToCopy);
	if (!NT_SUCCESS(status)) {
		KdPrint(("failed to copy descriptor to output buffer. status=0x%x", status));
		return status;
	}

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, bytesToCopy);

	return status;
}
NTSTATUS
NInputGetReportDescriptor(
	__in WDFDEVICE Device,
	__in WDFREQUEST Request
) {
	TraceEnterFunc();

	NTSTATUS            status = STATUS_SUCCESS;
	WDFMEMORY           memory = { 0 };

	UNREFERENCED_PARAMETER(Device);
	status = WdfRequestRetrieveOutputMemory(Request, &memory);
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfRequestRetrieveOutputMemory failed status=0x%x", status));
		return status;
	}

	//
	// Use hardcoded Report descriptor
	//
	status = WdfMemoryCopyFromBuffer(memory,
		0,
		(PUCHAR)PenHIDReportDescriptor,
		sizeof(PenHIDReportDescriptor));
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfMemoryCopyFromBuffer failed status=0x%x", status));
		return status;
	}

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, sizeof(PenHIDReportDescriptor));

	return status;
}
VOID MyInternalDeviceControl(
	__in WDFQUEUE     Queue,
	__in WDFREQUEST   Request,
	__in size_t       OutputBufferLength,
	__in size_t       InputBufferLength,
	__in ULONG        IoControlCode
) {
	TraceEnterFunc();

	NTSTATUS            status = STATUS_SUCCESS;
	WDFDEVICE           device = NULL;
	//PDEVICE_EXTENSION   devContext = NULL;
	BOOLEAN             RequestPending = FALSE;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	device = WdfIoQueueGetDevice(Queue);
	//
	// Please note that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl. So depending on the ioctl code, we will either
	// use retreive function or escape to WDM to get the UserBuffer.
	//

	switch (IoControlCode) {

	case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
		status = NInputGetHidDescriptor(device, Request);
		// pass
		break;

	case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
		//
		//Retrieves a device's attributes in a HID_DEVICE_ATTRIBUTES structure.
		//
		status = NInputGetDeviceAttributes(Request);
		break;

	case IOCTL_HID_GET_REPORT_DESCRIPTOR:
		//
		//Obtains the report descriptor for the HID device.
		//
		status = NInputGetReportDescriptor(device, Request);
		break;

	case IOCTL_HID_GET_STRING:
		//status = NInputGetString(device, Request);
		break;

	case IOCTL_HID_READ_REPORT:
		//status = NInputReadReport(devContext, Request, &RequestPending);
		break;

	case IOCTL_HID_SET_FEATURE:
		//
		// This sends a HID class feature report to a top-level collection of
		// a HID class device.
		//
		status = STATUS_NOT_SUPPORTED;
		break;

	case IOCTL_HID_GET_FEATURE:
		//
		// returns a feature report associated with a top-level collection
		//
		status = STATUS_NOT_SUPPORTED;
		break;

	case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:

	case IOCTL_HID_WRITE_REPORT:
		//
		//Transmits a class driver-supplied report to the device.
		//    
	case IOCTL_HID_ACTIVATE_DEVICE:
		//
		// Makes the device ready for I/O operations.
		//
	case IOCTL_HID_DEACTIVATE_DEVICE:
		//
		// Causes the device to cease operations and terminate all outstanding
		// I/O requests.
		//
	default:
		status = STATUS_NOT_SUPPORTED;
		break;
	}

	if (!RequestPending)
	{
		WdfRequestComplete(Request, status);
	}
	return;
}

// 创建设备对象, 本函数不会初始化设备上下文(DEVICE_CONTEXT)
NTSTATUS MyDeviceAdd(IN WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit) 
{
	TraceEnterFunc();

	// 互斥访问，同时刻只能一个应用程序与设备通信
	WdfDeviceInitSetExclusive(DeviceInit, TRUE);

	// 定义 PDO 和符号连接名

	DECLARE_CONST_UNICODE_STRING(ntDeviceObjectName, DEVICE_OBJECT_NAME);
	DECLARE_CONST_UNICODE_STRING(symbolicLinkName, DEVICE_LINK_NAME);

	// 使用 Neither 模式, 那么控制的 buffer 是一个原始 buffer 的虚拟地址副本.
	// https://docs.microsoft.com/en-us/windows-hardware/drivers/wdf/accessing-data-buffers-in-wdf-drivers#-accessing-data-buffers-for-buffered-io

	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

	// 分配 PDO 名
	NTSTATUS status = WdfDeviceInitAssignName(DeviceInit, &ntDeviceObjectName);

	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfDeviceInitAssignName failed"));
		goto End;
	}
	// WdfDeviceCreate 必须在 WdfDeviceCreate 之前调用
	//  A driver's EvtDeviceShutdownNotification event callback
	// function notifies the driver that the system is about to lose its power.
	WdfControlDeviceInitSetShutdownNotification(DeviceInit,
		MyShutdownNotification,	// 电脑快关机时的回调
		WdfDeviceShutdown);

	//WdfDeviceInitSetIoInCallerContextCallback(DeviceInit,
	//	MyEvtDeviceIoInCallerContext);
	

	// 该函数能够初始化驱动的 WDF对象属性, 然后把上下文结构插入到对象属性结构体中
	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);

	// 本设备句柄
	WDFDEVICE controlDevice;
	// 创建本驱动自身设备
	status = WdfDeviceCreate(&DeviceInit,
		&attributes,
		&controlDevice);
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfDeviceCreate failed"));
		goto End;
	}
	//
	// Create a symbolic link for the control object so that usermode can open
	// the device.
	// 创建符号链接, 以便用户与我们的设备通信
	//
	status = WdfDeviceCreateSymbolicLink(controlDevice,
		&symbolicLinkName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfDeviceCreateSymbolicLink failed"));
		goto End;
	}
	//
   // Configure a default queue so that requests that are not
   // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
   // other queues get dispatched here.
   //
	WDF_IO_QUEUE_CONFIG      ioQueueConfig;
	// 初始化 ioQueueConfig 结构体. IO 队列用来容纳 IO 请求. I/O请求分发处理方式为串行
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
		WdfIoQueueDispatchParallel);
	// IoWrite 事件回调, 用来处理 IRP_MJ_WRITE
	ioQueueConfig.EvtIoWrite = MyFileEvtIoWrite;//ok
	ioQueueConfig.EvtIoInternalDeviceControl = MyInternalDeviceControl;
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	//
	// By default, Static Driver Verifier (SDV) displays a warning if it 
	// doesn't find the EvtIoStop callback on a power-managed queue. 
	// 当设备离开工作状态(D0 State)就会调用 EvtIoStop 事件
	__analysis_assume(ioQueueConfig.EvtIoStop != 0);
	WDFQUEUE                            queue;
	status = WdfIoQueueCreate(controlDevice,
		&ioQueueConfig,
		&attributes,
		&queue // pointer to default queue
	);
	__analysis_assume(ioQueueConfig.EvtIoStop == 0);
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfIoQueueCreate  failed"));
		goto End;
	}
	WdfControlFinishInitializing(controlDevice);
End:
	//
	// If the device is created successfully, framework would clear the
	// DeviceInit value. Otherwise device create must have failed so we
	// should free the memory ourself.
	//
	if (DeviceInit != NULL) {
		WdfDeviceInitFree(DeviceInit);
	}

	return status;
}


// 划水
VOID EvtDeviceSelfManagedIoCleanup(	WDFDEVICE WdfDevice){
	TraceEnterFunc();
	PDEVICE_CONTEXT	deviceContext;

	PAGED_CODE();

	deviceContext = DeviceGetContext(WdfDevice);

	if (deviceContext->VhfHandle)
	{
		VhfDelete(deviceContext->VhfHandle, FALSE);
	}
}
// 划水
NTSTATUS EvtDeviceSelfManagedIoInit(	_In_ WDFDEVICE Device)
{
	TraceEnterFunc();
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("RawPdo started\n"));
	return status;
}
// 创建虚拟设备, 并初始化(不只是虚拟用的)设备上下文
NTSTATUS VhfSourceCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit)
{
	TraceEnterFunc();


	NTSTATUS status;

	WDF_PNPPOWER_EVENT_CALLBACKS    wdfPnpPowerCallbacks;

	PAGED_CODE();

	WdfFdoInitSetFilter(DeviceInit);

	//创建虚拟设备的管理回调
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&wdfPnpPowerCallbacks);
	wdfPnpPowerCallbacks.EvtDeviceSelfManagedIoInit = EvtDeviceSelfManagedIoInit;
	wdfPnpPowerCallbacks.EvtDeviceSelfManagedIoCleanup = EvtDeviceSelfManagedIoCleanup;
	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &wdfPnpPowerCallbacks);

	WDF_OBJECT_ATTRIBUTES vhfDeviceAttributes;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&vhfDeviceAttributes, DEVICE_CONTEXT);

	// 创建虚拟设备
	status = WdfDeviceCreate(&DeviceInit, &vhfDeviceAttributes, &vhfDevice);

	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create vhf device"));
		return status;
	}

	// 虚拟设备配置初始化(绑定到 HID 报告描述符)
	VHF_CONFIG vhfConfig;
	VHF_CONFIG_INIT(&vhfConfig,
		WdfDeviceWdmGetDeviceObject(vhfDevice),
		sizeof(PenHIDReportDescriptor),// HID 报告描述符的大小
		PenHIDReportDescriptor);

	PDEVICE_CONTEXT deviceContext;
	deviceContext = DeviceGetContext(vhfDevice);

	vhfConfig.VendorID = 0x08;//厂家标识
	vhfConfig.ProductID = 0x09;//产品标识
	vhfConfig.VersionNumber = 1;
	vhfConfig.VhfClientContext = deviceContext;// bug
	//vhfConfig.EvtVhfAsyncOperationWriteReport = HidWriteInputReport;

	status = VhfCreate(&vhfConfig, &(deviceContext->VhfHandle));

	if (!NT_SUCCESS(status))
	{
		KdPrint(("VhfCreate failed"));
		return status;
	}
	//初始化自旋锁, 目前好像用不着
	//KeInitializeSpinLock(&deviceContext->MessageProcessLock);

	return status;

}

NTSTATUS PenEvtDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit) _Function_class_(EVT_WDF_DRIVER_DEVICE_ADD)

{
	TraceEnterFunc();

	return VhfSourceCreateDevice(DeviceInit);
}

// 驱动入口
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	KdPrint(("hello, world!\r\n"));

	NTSTATUS status;

	// 初始化属性
	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.EvtCleanupCallback = MyEvtDriverContextCleanup; //ok

	// 初始化驱动配置
	WDF_DRIVER_CONFIG config;
	WDF_DRIVER_CONFIG_INIT(
		&config,
		MyEvtDriverDeviceAdd
	);	
	config.EvtDriverUnload = MyEvtDriverUnload;	// ok

	// 创建框架驱动对象
	WDFDRIVER hDriver;
	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		&attributes,
		&config,
		&hDriver);
	if (!NT_SUCCESS(status)) {
		KdPrint(("NonPnp: WdfDriverCreate failed with status 0x%x\n", status));
		return status;
	}

	PWDFDEVICE_INIT pInit = NULL;
	pInit = WdfControlDeviceInitAllocate(hDriver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R);

	if (pInit == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}

	//
	// 创建一个设备对象表示我们的虚拟设备
	//
	status = MyDeviceAdd(hDriver, pInit);

	return status;

}
