 char ReportDescriptor[118] = {
     0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
     0x09, 0x02,        // USAGE (Mouse)
     0xa1, 0x01,        // COLLECTION (Application)
     0x09, 0x02,        //   USAGE (Mouse)
     0xa1, 0x02,        //   COLLECTION (Logical)
     0x09, 0x01,        //     USAGE (Pointer)
     0xa1, 0x00,        //     COLLECTION (Physical)
                        // ------------------------------  Buttons
     0x05, 0x09,        //       USAGE_PAGE (Button)      
     0x19, 0x01,        //       USAGE_MINIMUM (Button 1)
     0x29, 0x05,        //       USAGE_MAXIMUM (Button 5)
     0x15, 0x00,        //       LOGICAL_MINIMUM (0)
     0x25, 0x01,        //       LOGICAL_MAXIMUM (1)
     0x75, 0x01,        //       REPORT_SIZE (1)
     0x95, 0x05,        //       REPORT_COUNT (5)
     0x81, 0x02,        //       INPUT (Data,Var,Abs)
                        // ------------------------------  Padding
     0x75, 0x03,        //       REPORT_SIZE (3)
     0x95, 0x01,        //       REPORT_COUNT (1)
     0x81, 0x03,        //       INPUT (Cnst,Var,Abs)
                        // ------------------------------  X,Y position
     0x05, 0x01,        //       USAGE_PAGE (Generic Desktop)
     0x09, 0x30,        //       USAGE (X)
     0x09, 0x31,        //       USAGE (Y)
     0x15, 0x81,        //       LOGICAL_MINIMUM (-127)
     0x25, 0x7f,        //       LOGICAL_MAXIMUM (127)
     0x75, 0x08,        //       REPORT_SIZE (8)
     0x95, 0x02,        //       REPORT_COUNT (2)
     0x81, 0x06,        //       INPUT (Data,Var,Rel)
     0xa1, 0x02,        //       COLLECTION (Logical)
                        // ------------------------------  Vertical wheel res multiplier
     0x09, 0x48,        //         USAGE (Resolution Multiplier)
     0x15, 0x00,        //         LOGICAL_MINIMUM (0)
     0x25, 0x01,        //         LOGICAL_MAXIMUM (1)
     0x35, 0x01,        //         PHYSICAL_MINIMUM (1)
     0x45, 0x04,        //         PHYSICAL_MAXIMUM (4)
     0x75, 0x02,        //         REPORT_SIZE (2)
     0x95, 0x01,        //         REPORT_COUNT (1)
     0xa4,              //         PUSH
     0xb1, 0x02,        //         FEATURE (Data,Var,Abs)
                        // ------------------------------  Vertical wheel
     0x09, 0x38,        //         USAGE (Wheel)
     0x15, 0x81,        //         LOGICAL_MINIMUM (-127)
     0x25, 0x7f,        //         LOGICAL_MAXIMUM (127)
     0x35, 0x00,        //         PHYSICAL_MINIMUM (0)        - reset physical
     0x45, 0x00,        //         PHYSICAL_MAXIMUM (0)
     0x75, 0x08,        //         REPORT_SIZE (8)
     0x81, 0x06,        //         INPUT (Data,Var,Rel)
     0xc0,              //       END_COLLECTION
     0xa1, 0x02,        //       COLLECTION (Logical)
                        // ------------------------------  Horizontal wheel res multiplier
     0x09, 0x48,        //         USAGE (Resolution Multiplier)
     0xb4,              //         POP
     0xb1, 0x02,        //         FEATURE (Data,Var,Abs)
                        // ------------------------------  Padding for Feature report
     0x35, 0x00,        //         PHYSICAL_MINIMUM (0)        - reset physical
     0x45, 0x00,        //         PHYSICAL_MAXIMUM (0)
     0x75, 0x04,        //         REPORT_SIZE (4)
     0xb1, 0x03,        //         FEATURE (Cnst,Var,Abs)
                        // ------------------------------  Horizontal wheel
     0x05, 0x0c,        //         USAGE_PAGE (Consumer Devices)
     0x0a, 0x38, 0x02,  //         USAGE (AC Pan)
     0x15, 0x81,        //         LOGICAL_MINIMUM (-127)
     0x25, 0x7f,        //         LOGICAL_MAXIMUM (127)
     0x75, 0x08,        //         REPORT_SIZE (8)
     0x81, 0x06,        //         INPUT (Data,Var,Rel)
     0xc0,              //       END_COLLECTION
     0xc0,              //     END_COLLECTION
     0xc0,              //   END_COLLECTION
     0xc0               // END_COLLECTION
 };