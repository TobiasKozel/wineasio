#ifndef _CONSTANTS
#define _CONSTANTS

#include "objbase.h"

#define MAX_ENVIRONMENT_SIZE        6
#define ASIO_MAX_NAME_LENGTH        32
#define ASIO_MINIMUM_BUFFERSIZE     16
#define ASIO_MAXIMUM_BUFFERSIZE     8192
#define ASIO_PREFERRED_BUFFERSIZE   1024

/* {48D0C522-BFCC-45cc-8B84-17F25F33E6E9} */
static GUID const CLSID_WineASIO = {
0x48d0c522, 0xbfcc, 0x45cc, { 0x8b, 0x84, 0x17, 0xf2, 0x5f, 0x33, 0xe6, 0xe9 } };

#define CLISID_WineASIOString "{48D0C522-BFCC-45CC-8B84-17F25F33E6E9}"
#define DRIVER_NAME "WineASIOPW"
#define DRIVER_REG_PATH "Software\\ASIO\\WineASIOPW"

#endif // _CONSTANTS