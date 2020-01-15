#pragma once

#define TraceEnterFunc() KdPrint(("DEBUG: Enter %s\n", __FUNCTION__))
#define TraceErrorStatus(funcName, status) KdPrint(("DEBUG: %s returned 0x%x in %s\n", funcName, status, __FUNCTION__))
void KdPrintBinary(PUCHAR bytes, UINT8 len) {
	TraceEnterFunc();
	for (int i = 0; i < len; i++)
	{
		KdPrint(("%#x ", *(bytes + i)));
	}
	KdPrint(("\r\n"));
}