#ifndef _CONSTANTS
#define _CONSTANTS

#include "objbase.h"

#define MAX_ENVIRONMENT_SIZE 6
#define ASIO_MAX_NAME_LENGTH 32
#define ASIO_MINIMUM_BUFFERSIZE 16
#define ASIO_MAXIMUM_BUFFERSIZE 8192
#define ASIO_PREFERRED_BUFFERSIZE 256
#define DISPATCHER_QUEUE_SIZE 32

/* {48D0C522-BFCC-45cc-8B84-17F25F33E6A0} */
static GUID const CLSID_WineASIO = {
    0x48d0c522,
    0xbfcc,
    0x45cc,
    {0x8b, 0x84, 0x17, 0xf2, 0x5f, 0x33, 0xe6, 0xa1}};
#define CLISID_WineASIOString "{48D0C522-BFCC-45CC-8B84-17F25F33E6A1}"

#define DRIVER_NAME "WineASIOPW"
#define DRIVER_REG_PATH "Software\\ASIO\\WineASIOPW"

/***********************************************************************
 *		Unicode strings used for the registry
 */
static const WCHAR key_software_wine_wineasio[] = {
    'S',  'o', 'f', 't', 'w', 'a', 'r', 'e', '\\', 'W', 'i', 'n', 'e',
    '\\', 'W', 'i', 'n', 'e', 'A', 'S', 'I', 'O',  'P', 'W', 0};
static const WCHAR value_wineasio_number_inputs[] = {
    'N', 'u', 'm', 'b', 'e', 'r', ' ', 'o', 'f',
    ' ', 'i', 'n', 'p', 'u', 't', 's', 0};
static const WCHAR value_wineasio_number_outputs[] = {
    'N', 'u', 'm', 'b', 'e', 'r', ' ', 'o', 'f',
    ' ', 'o', 'u', 't', 'p', 'u', 't', 's', 0};
static const WCHAR value_wineasio_fixed_buffersize[] = {
    'F', 'i', 'x', 'e', 'd', ' ', 'b', 'u', 'f',
    'f', 'e', 'r', 's', 'i', 'z', 'e', 0};
static const WCHAR value_wineasio_preferred_buffersize[] = {
    'P', 'r', 'e', 'f', 'e', 'r', 'r', 'e', 'd', ' ', 'b',
    'u', 'f', 'f', 'e', 'r', 's', 'i', 'z', 'e', 0};
static const WCHAR wineasio_autostart_server[] = {'A', 'u', 't', 'o', 's', 't',
                                                  'a', 'r', 't', ' ', 's', 'e',
                                                  'r', 'v', 'e', 'r', 0};
static const WCHAR value_wineasio_connect_to_hardware[] = {
    'C', 'o', 'n', 'n', 'e', 'c', 't', ' ', 't', 'o',
    ' ', 'h', 'a', 'r', 'd', 'w', 'a', 'r', 'e', 0};

/***********************************************************************
 *		static string constants for regsvr
 */
static const WCHAR interface_keyname[] = {'I', 'n', 't', 'e', 'r',
                                          'f', 'a', 'c', 'e', 0};
static const WCHAR base_ifa_keyname[] = {'B', 'a', 's', 'e', 'I', 'n', 't',
                                         'e', 'r', 'f', 'a', 'c', 'e', 0};
static const WCHAR num_methods_keyname[] = {'N', 'u', 'm', 'M', 'e', 't',
                                            'h', 'o', 'd', 's', 0};
static const WCHAR ps_clsid_keyname[] = {'P', 'r', 'o', 'x', 'y', 'S', 't', 'u',
                                         'b', 'C', 'l', 's', 'i', 'd', 0};
static const WCHAR ps_clsid32_keyname[] = {'P', 'r', 'o', 'x', 'y', 'S',
                                           't', 'u', 'b', 'C', 'l', 's',
                                           'i', 'd', '3', '2', 0};
static const WCHAR clsid_keyname[] = {'C', 'L', 'S', 'I', 'D', 0};
static const WCHAR curver_keyname[] = {'C', 'u', 'r', 'V', 'e', 'r', 0};
static const WCHAR ips_keyname[] = {'I', 'n', 'P', 'r', 'o', 'c', 'S',
                                    'e', 'r', 'v', 'e', 'r', 0};
static const WCHAR ips32_keyname[] = {'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e',
                                      'r', 'v', 'e', 'r', '3', '2', 0};
static const WCHAR progid_keyname[] = {'P', 'r', 'o', 'g', 'I', 'D', 0};
static const WCHAR viprogid_keyname[] = {
    'V', 'e', 'r', 's', 'i', 'o', 'n', 'I', 'n', 'd', 'e', 'p', 'e',
    'n', 'd', 'e', 'n', 't', 'P', 'r', 'o', 'g', 'I', 'D', 0};

#endif  // _CONSTANTS