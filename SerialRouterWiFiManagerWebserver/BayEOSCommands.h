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

