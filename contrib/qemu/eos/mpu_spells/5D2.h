/**
 * 5D2 MPU spells
 */
static struct mpu_init_spell mpu_init_spells_5D2[] = { {
    { 0x06, 0x04, 0x02, 0x00, 0x00 }, {                         /* spell #1 */
        { 0x08, 0x07, 0x01, 0x33, 0x00, 0x03, 0x00, 0x00 },     /* reply #1.1 */
        { 0x06, 0x05, 0x01, 0x20, 0x01, 0x00 },                 /* reply #1.2 */
        { 0x06, 0x05, 0x01, 0x21, 0x00, 0x00 },                 /* reply #1.3 */
        { 0x06, 0x05, 0x01, 0x22, 0x00, 0x00 },                 /* reply #1.4 */
        { 0x06, 0x05, 0x03, 0x0c, 0x01, 0x00 },                 /* reply #1.5 */
        { 0x06, 0x05, 0x03, 0x0d, 0x00, 0x00 },                 /* reply #1.6 */
        { 0x06, 0x05, 0x03, 0x0e, 0x00, 0x00 },                 /* reply #1.7 */
        { 0x08, 0x06, 0x01, 0x23, 0x00, 0x00, 0x00 },           /* reply #1.8 */
        { 0x08, 0x06, 0x01, 0x24, 0x00, 0x00, 0x00 },           /* reply #1.9, PROP_CARD2_STATUS(0) */
        { 0x08, 0x06, 0x01, 0x25, 0x00, 0x00, 0x00 },           /* reply #1.10 */
        { 0x06, 0x05, 0x01, 0x2e, 0x01, 0x00 },                 /* reply #1.11 */
        { 0x06, 0x05, 0x01, 0x2c, 0x01, 0x00 },                 /* reply #1.12 */
        { 0x06, 0x05, 0x03, 0x20, 0x04, 0x00 },                 /* reply #1.13 */
        { 0x06, 0x05, 0x01, 0x3d, 0x00, 0x00 },                 /* reply #1.14 */
        { 0x06, 0x05, 0x01, 0x42, 0x00, 0x00 },                 /* reply #1.15 */
        { 0x06, 0x05, 0x01, 0x43, 0x00, 0x00 },                 /* reply #1.16 */
        { 0x06, 0x05, 0x01, 0x46, 0x00, 0x00 },                 /* reply #1.17 */
        { 0x06, 0x05, 0x01, 0x44, 0x00, 0x00 },                 /* reply #1.18 */
        { 0x06, 0x05, 0x01, 0x00, 0x03, 0x00 },                 /* reply #1.19, PROP_SHOOTING_MODE(3) */
        { 0x2c, 0x2a, 0x02, 0x00, 0x03, 0x03, 0x05, 0x00, 0x03, 0x03, 0x00, 0x78, 0x00, 0x00, 0x03, 0x1b, 0x58, 0x00, 0x00, 0x00, 0x00, 0x83, 0x06, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x03, 0x06, 0x00, 0x00, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x55, 0x15, 0x01 },/* reply #1.20 */
        { 0x06, 0x05, 0x01, 0x37, 0x00, 0x00 },                 /* reply #1.21 */
        { 0x06, 0x05, 0x01, 0x49, 0x01, 0x00 },                 /* reply #1.22 */
        { 0x06, 0x05, 0x01, 0x3e, 0x01, 0x00 },                 /* reply #1.23 */
        { 0x08, 0x06, 0x01, 0x45, 0x00, 0x10, 0x00 },           /* reply #1.24 */
        { 0x06, 0x05, 0x01, 0x48, 0x01, 0x00 },                 /* reply #1.25 */
        { 0x06, 0x05, 0x01, 0x4b, 0x01, 0x00 },                 /* reply #1.26 */
        { 0x06, 0x05, 0x01, 0x40, 0x00, 0x00 },                 /* reply #1.27 */
        { 0x06, 0x05, 0x01, 0x41, 0x00, 0x00 },                 /* reply #1.28 */
        { 0x06, 0x05, 0x01, 0x3f, 0x00, 0x00 },                 /* reply #1.29 */
        { 0x16, 0x14, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x0f, 0x00 },/* reply #1.30 */
        { 0x0e, 0x0c, 0x02, 0x05, 0x08, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.31 */
        { 0x0c, 0x0a, 0x02, 0x06, 0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00 },/* reply #1.32 */
        { 0x14, 0x13, 0x02, 0x07, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x06, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00 },/* reply #1.33 */
        { 0x0e, 0x0c, 0x02, 0x08, 0x08, 0x03, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.34 */
        { 0x0a, 0x08, 0x03, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.35 */
        { 0x06, 0x05, 0x03, 0x05, 0x01, 0x00 },                 /* reply #1.36 */
        { 0x1e, 0x1c, 0x03, 0x30, 0x65, 0x65, 0x65, 0x53, 0x53, 0x53, 0x65, 0x65, 0x00, 0x1b, 0x00, 0x1b, 0x00, 0x1b, 0x00, 0x1b, 0x00, 0x1b, 0x00, 0x1b, 0x00, 0x1b, 0x00, 0x1b, 0x00 },/* reply #1.37 */
        { 0x0e, 0x0c, 0x03, 0x2e, 0x00, 0x00, 0xbb, 0xfd, 0x00, 0x00, 0xf4, 0x14, 0x00 },/* reply #1.38 */
        { 0x06, 0x05, 0x03, 0x35, 0x01, 0x00 },                 /* reply #1.39 */
        { 0x1c, 0x1b, 0x03, 0x1d, 0x19, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x50, 0x2d, 0x45, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0xc3, 0x83, 0xb6, 0x00 },/* reply #1.40 */
        { 0x06, 0x04, 0x03, 0x36, 0x00 },                       /* reply #1.41 */
        { 0 } } }, {
    { 0x08, 0x06, 0x00, 0x00, 0x02, 0x00, 0x00 }, {             /* spell #2, Complete WaitID = 0x80000001 */
        { 0 } } }, {
    { 0x06, 0x05, 0x03, 0x0e, 0x00, 0x00 }, {                   /* spell #3 */
        { 0 } } }, {
    { 0x06, 0x05, 0x01, 0x22, 0x00, 0x00 }, {                   /* spell #4 */
        { 0x06, 0x05, 0x03, 0x0e, 0x00, 0x00 },                 /* reply #4.1 */
        { 0 } } }, {
    { 0x08, 0x06, 0x01, 0x25, 0x00, 0x00, 0x00 }, {             /* spell #5 */
        { 0x06, 0x05, 0x01, 0x22, 0x00, 0x00 },                 /* reply #5.1 */
        { 0x08, 0x06, 0x01, 0x25, 0x00, 0x00, 0x00 },           /* reply #5.2 */
        { 0 } } }, {
    { 0x06, 0x05, 0x01, 0x37, 0x00, 0x00 }, {                   /* spell #6 */
        { 0x06, 0x05, 0x01, 0x37, 0x00, 0x00 },                 /* reply #6.1 */
        { 0x0a, 0x08, 0x01, 0x34, 0x00, 0x00, 0x06, 0x03, 0x01 },/* reply #6.2 */
        { 0 } } }, {
    { 0x08, 0x06, 0x00, 0x00, 0x01, 0x34, 0x00 }, {             /* spell #7, Complete WaitID = 0x8000002F */
        { 0x0a, 0x08, 0x01, 0x35, 0x00, 0x00, 0x01, 0x03, 0x01 },/* reply #7.1 */
        { 0 } } }, {
    { 0x08, 0x06, 0x00, 0x00, 0x01, 0x35, 0x00 }, {             /* spell #8, Complete WaitID = 0x80000030 */
        { 0x0a, 0x08, 0x01, 0x36, 0x00, 0x00, 0x06, 0x03, 0x01 },/* reply #8.1 */
        { 0 } } }, {
    { 0x08, 0x06, 0x00, 0x00, 0x01, 0x36, 0x00 }, {             /* spell #9, Complete WaitID = 0x80000031 */
        { 0 } } }, {
    { 0x0a, 0x08, 0x03, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00 }, { /* spell #10 */
        { 0 } } }, {
    { 0x06, 0x04, 0x03, 0x10, 0x00 }, {                         /* spell #11 */
        { 0 } } }, {
    { 0x06, 0x05, 0x03, 0x07, 0xff, 0x00 }, {                   /* spell #12, PROP_BURST_COUNT(255) */
        { 0 } } }, {
    { 0x06, 0x05, 0x01, 0x2e, 0x01, 0x00 }, {                   /* spell #13 */
        { 0x06, 0x05, 0x01, 0x2e, 0x01, 0x00 },                 /* reply #13.1 */
        { 0 } } }, {
    { 0x0a, 0x08, 0x03, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00 }, { /* spell #14 */
        { 0 } } }, {
    { 0x06, 0x05, 0x04, 0x0e, 0x01, 0x00 }, {                   /* spell #15 */
        { 0 } } }, {
    { 0x08, 0x06, 0x01, 0x25, 0x00, 0x00, 0x00 }, {             /* spell #16 */
        { 0x08, 0x06, 0x01, 0x25, 0x00, 0x00, 0x00 },           /* reply #16.1 */
        { 0 } } }, {
    { 0x08, 0x06, 0x01, 0x23, 0x00, 0x01, 0x00 }, {             /* spell #17 */
        { 0x08, 0x06, 0x01, 0x23, 0x00, 0x01, 0x00 },           /* reply #17.1 */
        { 0 } } }, {
    { 0x08, 0x06, 0x01, 0x26, 0x00, 0x64, 0x00 }, {             /* spell #18 */
        { 0 } } }, {
    { 0x06, 0x05, 0x03, 0x07, 0x0b, 0x00 }, {                   /* spell #19, PROP_BURST_COUNT(11) */
        { 0 } } }, {
    { 0x0a, 0x08, 0x03, 0x06, 0x00, 0x00, 0x00, 0x4a, 0x00 }, { /* spell #20 */
        { 0 } } }, {
    { 0x08, 0x07, 0x01, 0x29, 0x0f, 0x07, 0x00, 0x00 }, {       /* spell #21 */
        { 0 } } }, {
    { 0x06, 0x05, 0x03, 0x11, 0x01, 0x00 }, {                   /* spell #22 */
        { 0 } } }, {
    { 0x06, 0x05, 0x02, 0x0a, 0x00, 0x00 }, {                   /* spell #23 */
        { 0x06, 0x05, 0x01, 0x2c, 0x01, 0x00 },                 /* reply #23.1 */
        { 0x0a, 0x08, 0x03, 0x00, 0xa1, 0x00, 0x00, 0x00, 0x00 },/* reply #23.2 */
        { 0x06, 0x05, 0x03, 0x04, 0x00, 0x00 },                 /* reply #23.3 */
        { 0x14, 0x12, 0x03, 0x15, 0x01, 0x16, 0x16, 0x00, 0x1b, 0x00, 0x87, 0x00, 0x87, 0x90, 0x04, 0x00, 0x00, 0x00, 0x00 },/* reply #23.4 */
        { 0x06, 0x05, 0x03, 0x17, 0x9c, 0x00 },                 /* reply #23.5 */
        { 0x08, 0x06, 0x01, 0x0a, 0x00, 0x01, 0x00 },           /* reply #23.6 */
        { 0x06, 0x05, 0x01, 0x38, 0x00, 0x00 },                 /* reply #23.7 */
        { 0x06, 0x05, 0x01, 0x39, 0x00, 0x00 },                 /* reply #23.8 */
        { 0x06, 0x05, 0x01, 0x0f, 0x00, 0x00 },                 /* reply #23.9 */
        { 0x06, 0x05, 0x03, 0x23, 0x0a, 0x00 },                 /* reply #23.10 */
        { 0x10, 0x0e, 0x03, 0x24, 0x31, 0x33, 0x35, 0x2d, 0x31, 0x33, 0x35, 0x6d, 0x6d, 0x00, 0x00 },/* reply #23.11 */
        { 0x06, 0x04, 0x03, 0x25, 0x00 },                       /* reply #23.12 */
        { 0x06, 0x05, 0x01, 0x3d, 0x00, 0x00 },                 /* reply #23.13 */
        { 0x14, 0x12, 0x03, 0x15, 0x01, 0x16, 0x16, 0x00, 0x1b, 0x00, 0x87, 0x00, 0x87, 0x90, 0x04, 0x00, 0x00, 0x00, 0x00 },/* reply #23.14 */
        { 0 } } }, {
    { 0x06, 0x05, 0x03, 0x19, 0x01, 0x00 }, {                   /* spell #24 */
        { 0 } } }, {
    { 0x06, 0x05, 0x09, 0x11, 0x01, 0x00 }, {                   /* spell #25 */
        { 0 } } }, {
    { 0x12, 0x11, 0x09, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, {/* spell #26 */
        { 0 } } }, {
    { 0x26, 0x24, 0x09, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, {/* spell #27 */
        { 0 } } }, {
    { 0x08, 0x06, 0x03, 0x18, 0x00, 0x00, 0x00 }, {             /* spell #28 */
        { 0 } } }
};
