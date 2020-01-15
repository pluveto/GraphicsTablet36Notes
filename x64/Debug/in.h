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
	0x65, 0x11,							 //     Unit (System: SI Linear, Length: Centimeter)
	0x55, 0x0D,							 //     Unit Exponent (-3)
	0x35, 0x00,                         //     PHYSICAL_MINIMUM (0)         55
	0x46, 0xd0, 0x39,                   //     PHYSICAL_MAXIMUM (8250)      57
	0x26, 0xd0, 0x39,                    //     LOGICAL_MAXIMUM (21240)      60
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)         63
	0x09, 0x31,                         //     USAGE (Y)                    65
	0x46, 0x08, 0x52,                   //     PHYSICAL_MAXIMUM (6188)      67
	0x26, 0x08, 0x52,                   //     LOGICAL_MAXIMUM (15980)      70
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