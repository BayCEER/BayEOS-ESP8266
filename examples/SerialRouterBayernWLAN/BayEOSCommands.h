/*
 * BayEOS-Commands
 *
 *
 */

// Router Commands: [0x2][0x31][command]
#define BayEOS_RouterCommand 0x31
#define ROUTER_IS_READY 0x1
// Returns [0x3][0x31][command][uint8_t]
#define ROUTER_GET_AVAILABLE 0x2
// Returns [0x3][0x31][command][unsigned long]
#define ROUTER_SEND 0x3
// Returns [0x3][0x31][command][uint8_t]
#define ROUTER_SET_NAME 0x4
// Returns [0x3][0x31][command][uint8_t]
#define ROUTER_SET_CONFIG 0x5
// Returns [0x3][0x31][command][uint8_t]

#define BaySerialESP_NAME 0x1
#define BaySerialESP_GATEWAY 0x2
#define BaySerialESP_USER 0x3
#define BaySerialESP_PW 0x4
