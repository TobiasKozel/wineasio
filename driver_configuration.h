#ifndef _DRIVER_CONFIGURATION
#define _DRIVER_CONFIGURATION

#include "./interface.h"
#include <errno.h>
#ifdef WINE_WITH_UNICODE
#include "wine/unicode.h"
#endif

#ifndef WINE_WITH_UNICODE
/* Funtion required as unicode.h no longer in WINE */
static WCHAR *strrchrW(const WCHAR* str, WCHAR ch)
{
    WCHAR *ret = NULL;
    do { if (*str == ch) ret = (WCHAR *)(ULONG_PTR)str; } while (*str++);
    return ret;
}
#endif

static VOID configure_driver(IWineASIOImpl *This)
{
    HKEY    hkey;
    LONG    result, value;
    DWORD   type, size;
    WCHAR   application_path [MAX_PATH];
    WCHAR   *application_name;
    char    environment_variable[MAX_ENVIRONMENT_SIZE];

    /* Unicode strings used for the registry */
    static const WCHAR key_software_wine_wineasio[] =
        { 'S','o','f','t','w','a','r','e','\\',
          'W','i','n','e','\\',
          'W','i','n','e','A','S','I','O',0 };
    static const WCHAR value_wineasio_number_inputs[] =
        { 'N','u','m','b','e','r',' ','o','f',' ','i','n','p','u','t','s',0 };
    static const WCHAR value_wineasio_number_outputs[] =
        { 'N','u','m','b','e','r',' ','o','f',' ','o','u','t','p','u','t','s',0 };
    static const WCHAR value_wineasio_fixed_buffersize[] =
        { 'F','i','x','e','d',' ','b','u','f','f','e','r','s','i','z','e',0 };
    static const WCHAR value_wineasio_preferred_buffersize[] =
        { 'P','r','e','f','e','r','r','e','d',' ','b','u','f','f','e','r','s','i','z','e',0 };
    static const WCHAR wineasio_autostart_server[] =
        { 'A','u','t','o','s','t','a','r','t',' ','s','e','r','v','e','r',0 };
    static const WCHAR value_wineasio_connect_to_hardware[] =
        { 'C','o','n','n','e','c','t',' ','t','o',' ','h','a','r','d','w','a','r','e',0 };

    /* Initialise most member variables,
     * asio_sample_position, asio_time, & asio_time_stamp are initialized in Start()
     * jack_num_input_ports & jack_num_output_ports are initialized in Init() */
    This->asio_active_inputs = 0;
    This->asio_active_outputs = 0;
    This->asio_buffer_index = 0;
    This->asio_callbacks = NULL;
    This->asio_can_time_code = FALSE;
    This->asio_current_buffersize = 0;
    This->asio_driver_state = Loaded;
    This->asio_sample_rate = 0;
    This->asio_time_info_mode = FALSE;
    This->asio_version = 92;

    This->wineasio_number_inputs = 16;
    This->wineasio_number_outputs = 16;
    This->wineasio_autostart_server = FALSE;
    This->wineasio_connect_to_hardware = TRUE;
    This->wineasio_fixed_buffersize = TRUE;
    This->wineasio_preferred_buffersize = ASIO_PREFERRED_BUFFERSIZE;

    This->jack_client = NULL;
    This->jack_client_name[0] = 0;
    This->jack_input_ports = NULL;
    This->jack_output_ports = NULL;
    This->callback_audio_buffer = NULL;
    This->input_channel = NULL;
    This->output_channel = NULL;

    /* create registry entries with defaults if not present */
    result = RegCreateKeyExW(HKEY_CURRENT_USER, key_software_wine_wineasio, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL);

    /* get/set number of asio inputs */
    size = sizeof(DWORD);
    if (RegQueryValueExW(hkey, value_wineasio_number_inputs, NULL, &type, (LPBYTE) &value, &size) == ERROR_SUCCESS)
    {
        if (type == REG_DWORD)
            This->wineasio_number_inputs = value;
    }
    else
    {
        type = REG_DWORD;
        size = sizeof(DWORD);
        value = This->wineasio_number_inputs;
        result = RegSetValueExW(hkey, value_wineasio_number_inputs, 0, REG_DWORD, (LPBYTE) &value, size);
    }

    /* get/set number of asio outputs */
    size = sizeof(DWORD);
    if (RegQueryValueExW(hkey, value_wineasio_number_outputs, NULL, &type, (LPBYTE) &value, &size) == ERROR_SUCCESS)
    {
        if (type == REG_DWORD)
            This->wineasio_number_outputs = value;
    }
    else
    {
        type = REG_DWORD;
        size = sizeof(DWORD);
        value = This->wineasio_number_outputs;
        result = RegSetValueExW(hkey, value_wineasio_number_outputs, 0, REG_DWORD, (LPBYTE) &value, size);
    }

    /* allow changing of asio buffer sizes */
    size = sizeof(DWORD);
    if (RegQueryValueExW(hkey, value_wineasio_fixed_buffersize, NULL, &type, (LPBYTE) &value, &size) == ERROR_SUCCESS)
    {
        if (type == REG_DWORD)
            This->wineasio_fixed_buffersize = value;
    }
    else
    {
        type = REG_DWORD;
        size = sizeof(DWORD);
        value = This->wineasio_fixed_buffersize;
        result = RegSetValueExW(hkey, value_wineasio_fixed_buffersize, 0, REG_DWORD, (LPBYTE) &value, size);
    }

    /* preferred buffer size (if changing buffersize is allowed) */
    size = sizeof(DWORD);
    if (RegQueryValueExW(hkey, value_wineasio_preferred_buffersize, NULL, &type, (LPBYTE) &value, &size) == ERROR_SUCCESS)
    {
        if (type == REG_DWORD)
            This->wineasio_preferred_buffersize = value;
    }
    else
    {
        type = REG_DWORD;
        size = sizeof(DWORD);
        value = This->wineasio_preferred_buffersize;
        result = RegSetValueExW(hkey, value_wineasio_preferred_buffersize, 0, REG_DWORD, (LPBYTE) &value, size);
    }

    /* get/set JACK autostart */
    size = sizeof(DWORD);
    if (RegQueryValueExW(hkey, wineasio_autostart_server, NULL, &type, (LPBYTE) &value, &size) == ERROR_SUCCESS)
    {
        if (type == REG_DWORD)
            This->wineasio_autostart_server = value;
    }
    else
    {
        type = REG_DWORD;
        size = sizeof(DWORD);
        value = This->wineasio_autostart_server;
        result = RegSetValueExW(hkey, wineasio_autostart_server, 0, REG_DWORD, (LPBYTE) &value, size);
    }

    /* get/set JACK connect to physical io */
    size = sizeof(DWORD);
    if (RegQueryValueExW(hkey, value_wineasio_connect_to_hardware, NULL, &type, (LPBYTE) &value, &size) == ERROR_SUCCESS)
    {
        if (type == REG_DWORD)
            This->wineasio_connect_to_hardware = value;
    }
    else
    {
        type = REG_DWORD;
        size = sizeof(DWORD);
        value = This->wineasio_connect_to_hardware;
        result = RegSetValueExW(hkey, value_wineasio_connect_to_hardware, 0, REG_DWORD, (LPBYTE) &value, size);
    }

    /* get client name by stripping path and extension */
    GetModuleFileNameW(0, application_path, MAX_PATH);
    application_name = strrchrW(application_path, L'.');
    *application_name = 0;
    application_name = strrchrW(application_path, L'\\');
    application_name++;
    WideCharToMultiByte(CP_ACP, WC_SEPCHARS, application_name, -1, This->jack_client_name, ASIO_MAX_NAME_LENGTH, NULL, NULL);

    RegCloseKey(hkey);

    /* Look for environment variables to override registry config values */

    if (GetEnvironmentVariableA("WINEASIO_NUMBER_INPUTS", environment_variable, MAX_ENVIRONMENT_SIZE))
    {
        errno = 0;
        result = strtol(environment_variable, 0, 10);
        if (errno != ERANGE)
            This->wineasio_number_inputs = result;
    }

    if (GetEnvironmentVariableA("WINEASIO_NUMBER_OUTPUTS", environment_variable, MAX_ENVIRONMENT_SIZE))
    {
        errno = 0;
        result = strtol(environment_variable, 0, 10);
        if (errno != ERANGE)
            This->wineasio_number_outputs = result;
    }

    if (GetEnvironmentVariableA("WINEASIO_AUTOSTART_SERVER", environment_variable, MAX_ENVIRONMENT_SIZE))
    {
        if (!strcasecmp(environment_variable, "on"))
            This->wineasio_autostart_server = TRUE;
        else if (!strcasecmp(environment_variable, "off"))
            This->wineasio_autostart_server = FALSE;
    }

    if (GetEnvironmentVariableA("WINEASIO_CONNECT_TO_HARDWARE", environment_variable, MAX_ENVIRONMENT_SIZE))
    {
        if (!strcasecmp(environment_variable, "on"))
            This->wineasio_connect_to_hardware = TRUE;
        else if (!strcasecmp(environment_variable, "off"))
            This->wineasio_connect_to_hardware = FALSE;
    }

    if (GetEnvironmentVariableA("WINEASIO_FIXED_BUFFERSIZE", environment_variable, MAX_ENVIRONMENT_SIZE))
    {
        if (!strcasecmp(environment_variable, "on"))
            This->wineasio_fixed_buffersize = TRUE;
        else if (!strcasecmp(environment_variable, "off"))
            This->wineasio_fixed_buffersize = FALSE;
    }

    if (GetEnvironmentVariableA("WINEASIO_PREFERRED_BUFFERSIZE", environment_variable, MAX_ENVIRONMENT_SIZE))
    {
        errno = 0;
        result = strtol(environment_variable, 0, 10);
        if (errno != ERANGE)
            This->wineasio_preferred_buffersize = result;
    }

    /* over ride the JACK client name gotten from the application name */
    size = GetEnvironmentVariableA("WINEASIO_CLIENT_NAME", environment_variable, ASIO_MAX_NAME_LENGTH);
    if (size > 0 && size < ASIO_MAX_NAME_LENGTH)
        strcpy(This->jack_client_name, environment_variable);

    /* if wineasio_preferred_buffersize is not a power of two or if out of range, then set to ASIO_PREFERRED_BUFFERSIZE */
    if (!(This->wineasio_preferred_buffersize > 0 && !(This->wineasio_preferred_buffersize&(This->wineasio_preferred_buffersize-1))
            && This->wineasio_preferred_buffersize >= ASIO_MINIMUM_BUFFERSIZE
            && This->wineasio_preferred_buffersize <= ASIO_MAXIMUM_BUFFERSIZE))
        This->wineasio_preferred_buffersize = ASIO_PREFERRED_BUFFERSIZE;

    return;
}

#endif // _DRIVER_CONFIGURATION