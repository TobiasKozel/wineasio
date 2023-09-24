#ifndef _INTERFACE
#define _INTERFACE

#include "./constants.h"
#include "objbase.h"
#include <jack/jack.h>

#define IEEE754_64FLOAT 1
#undef NATIVE_INT64
#include "asio.h"
#define NATIVE_INT64

/* Hide ELF symbols for the COM members - No need to to export them */
#define HIDDEN __attribute__ ((visibility("hidden")))


typedef struct IWineASIO *LPWINEASIO;

typedef struct IOChannel
{
	ASIOBool                    active;
	jack_default_audio_sample_t *audio_buffer;
	char                        port_name[ASIO_MAX_NAME_LENGTH];
	jack_port_t                 *port;
} IOChannel;

/* structure needed to create the JACK callback thread in the wine process context */
struct {
	void        *(*jack_callback_thread) (void*);
	void        *arg;
	pthread_t   jack_callback_pthread_id;
	HANDLE      jack_callback_thread_created;
} jack_thread_creator_privates;

enum { Loaded, Initialized, Prepared, Running };


/*****************************************************************************
 * IWineAsio interface
 */

#define INTERFACE IWineASIO
DECLARE_INTERFACE_(IWineASIO,IUnknown)
{
	STDMETHOD_(HRESULT, QueryInterface)         (THIS_ IID riid, void** ppvObject) PURE;
	STDMETHOD_(ULONG, AddRef)                   (THIS) PURE;
	STDMETHOD_(ULONG, Release)                  (THIS) PURE;
	STDMETHOD_(ASIOBool, Init)                  (THIS_ void *sysRef) PURE;
	STDMETHOD_(void, GetDriverName)             (THIS_ char *name) PURE;
	STDMETHOD_(LONG, GetDriverVersion)          (THIS) PURE;
	STDMETHOD_(void, GetErrorMessage)           (THIS_ char *string) PURE;
	STDMETHOD_(ASIOError, Start)                (THIS) PURE;
	STDMETHOD_(ASIOError, Stop)                 (THIS) PURE;
	STDMETHOD_(ASIOError, GetChannels)          (THIS_ LONG *numInputChannels, LONG *numOutputChannels) PURE;
	STDMETHOD_(ASIOError, GetLatencies)         (THIS_ LONG *inputLatency, LONG *outputLatency) PURE;
	STDMETHOD_(ASIOError, GetBufferSize)        (THIS_ LONG *minSize, LONG *maxSize, LONG *preferredSize, LONG *granularity) PURE;
	STDMETHOD_(ASIOError, CanSampleRate)        (THIS_ ASIOSampleRate sampleRate) PURE;
	STDMETHOD_(ASIOError, GetSampleRate)        (THIS_ ASIOSampleRate *sampleRate) PURE;
	STDMETHOD_(ASIOError, SetSampleRate)        (THIS_ ASIOSampleRate sampleRate) PURE;
	STDMETHOD_(ASIOError, GetClockSources)      (THIS_ ASIOClockSource *clocks, LONG *numSources) PURE;
	STDMETHOD_(ASIOError, SetClockSource)       (THIS_ LONG index) PURE;
	STDMETHOD_(ASIOError, GetSamplePosition)    (THIS_ ASIOSamples *sPos, ASIOTimeStamp *tStamp) PURE;
	STDMETHOD_(ASIOError, GetChannelInfo)       (THIS_ ASIOChannelInfo *info) PURE;
	STDMETHOD_(ASIOError, CreateBuffers)        (THIS_ ASIOBufferInfo *bufferInfo, LONG numChannels, LONG bufferSize, ASIOCallbacks *asioCallbacks) PURE;
	STDMETHOD_(ASIOError, DisposeBuffers)       (THIS) PURE;
	STDMETHOD_(ASIOError, ControlPanel)         (THIS) PURE;
	STDMETHOD_(ASIOError, Future)               (THIS_ LONG selector,void *opt) PURE;
	STDMETHOD_(ASIOError, OutputReady)          (THIS) PURE;
};
#undef INTERFACE

typedef struct IWineASIOImpl
{
	/* COM stuff */
	const IWineASIOVtbl         *lpVtbl;
	LONG                        ref;

	/* The app's main window handle on windows, 0 on OS/X */
	HWND                        sys_ref;

	/* ASIO stuff */
	LONG                        asio_active_inputs;
	LONG                        asio_active_outputs;
	BOOL                        asio_buffer_index;
	ASIOCallbacks               *asio_callbacks;
	BOOL                        asio_can_time_code;
	LONG                        asio_current_buffersize;
	INT                         asio_driver_state;
	ASIOSamples                 asio_sample_position;
	ASIOSampleRate              asio_sample_rate;
	ASIOTime                    asio_time;
	BOOL                        asio_time_info_mode;
	ASIOTimeStamp               asio_time_stamp;
	LONG                        asio_version;

	/* WineASIO configuration options */
	LONG                        wineasio_number_inputs;
	LONG                        wineasio_number_outputs;
	BOOL                        wineasio_autostart_server;
	BOOL                        wineasio_connect_to_hardware;
	LONG                        wineasio_fixed_buffersize;
	LONG                        wineasio_preferred_buffersize;

	/* JACK stuff */
	jack_client_t               *jack_client;
	char                        jack_client_name[ASIO_MAX_NAME_LENGTH];
	int                         jack_num_input_ports;
	int                         jack_num_output_ports;
	const char                  **jack_input_ports;
	const char                  **jack_output_ports;

	/* jack process callback buffers */
	jack_default_audio_sample_t *callback_audio_buffer;
	IOChannel                   *input_channel;
	IOChannel                   *output_channel;
} IWineASIOImpl;


/****************************************************************************
 *  Interface Methods
 */

/*
 *  as seen from the WineASIO source
 */

HIDDEN HRESULT   STDMETHODCALLTYPE      QueryInterface(LPWINEASIO iface, REFIID riid, void **ppvObject);
HIDDEN ULONG     STDMETHODCALLTYPE      AddRef(LPWINEASIO iface);
HIDDEN ULONG     STDMETHODCALLTYPE      Release(LPWINEASIO iface);
HIDDEN ASIOBool  STDMETHODCALLTYPE      Init(LPWINEASIO iface, void *sysRef);
HIDDEN void      STDMETHODCALLTYPE      GetDriverName(LPWINEASIO iface, char *name);
HIDDEN LONG      STDMETHODCALLTYPE      GetDriverVersion(LPWINEASIO iface);
HIDDEN void      STDMETHODCALLTYPE      GetErrorMessage(LPWINEASIO iface, char *string);
HIDDEN ASIOError STDMETHODCALLTYPE      Start(LPWINEASIO iface);
HIDDEN ASIOError STDMETHODCALLTYPE      Stop(LPWINEASIO iface);
HIDDEN ASIOError STDMETHODCALLTYPE      GetChannels (LPWINEASIO iface, LONG *numInputChannels, LONG *numOutputChannels);
HIDDEN ASIOError STDMETHODCALLTYPE      GetLatencies(LPWINEASIO iface, LONG *inputLatency, LONG *outputLatency);
HIDDEN ASIOError STDMETHODCALLTYPE      GetBufferSize(LPWINEASIO iface, LONG *minSize, LONG *maxSize, LONG *preferredSize, LONG *granularity);
HIDDEN ASIOError STDMETHODCALLTYPE      CanSampleRate(LPWINEASIO iface, ASIOSampleRate sampleRate);
HIDDEN ASIOError STDMETHODCALLTYPE      GetSampleRate(LPWINEASIO iface, ASIOSampleRate *sampleRate);
HIDDEN ASIOError STDMETHODCALLTYPE      SetSampleRate(LPWINEASIO iface, ASIOSampleRate sampleRate);
HIDDEN ASIOError STDMETHODCALLTYPE      GetClockSources(LPWINEASIO iface, ASIOClockSource *clocks, LONG *numSources);
HIDDEN ASIOError STDMETHODCALLTYPE      SetClockSource(LPWINEASIO iface, LONG index);
HIDDEN ASIOError STDMETHODCALLTYPE      GetSamplePosition(LPWINEASIO iface, ASIOSamples *sPos, ASIOTimeStamp *tStamp);
HIDDEN ASIOError STDMETHODCALLTYPE      GetChannelInfo(LPWINEASIO iface, ASIOChannelInfo *info);
HIDDEN ASIOError STDMETHODCALLTYPE      CreateBuffers(LPWINEASIO iface, ASIOBufferInfo *bufferInfo, LONG numChannels, LONG bufferSize, ASIOCallbacks *asioCallbacks);
HIDDEN ASIOError STDMETHODCALLTYPE      DisposeBuffers(LPWINEASIO iface);
HIDDEN ASIOError STDMETHODCALLTYPE      ControlPanel(LPWINEASIO iface);
HIDDEN ASIOError STDMETHODCALLTYPE      Future(LPWINEASIO iface, LONG selector, void *opt);
HIDDEN ASIOError STDMETHODCALLTYPE      OutputReady(LPWINEASIO iface);

static const IWineASIOVtbl WineASIO_Vtbl =
{
	(void *) QueryInterface,
	(void *) AddRef,
	(void *) Release,

	(void *) (Init),
	(void *) (GetDriverName),
	(void *) (GetDriverVersion),
	(void *) (GetErrorMessage),
	(void *) (Start),
	(void *) (Stop),
	(void *) (GetChannels),
	(void *) (GetLatencies),
	(void *) (GetBufferSize),
	(void *) (CanSampleRate),
	(void *) (GetSampleRate),
	(void *) (SetSampleRate),
	(void *) (GetClockSources),
	(void *) (SetClockSource),
	(void *) (GetSamplePosition),
	(void *) (GetChannelInfo),
	(void *) (CreateBuffers),
	(void *) (DisposeBuffers),
	(void *) (ControlPanel),
	(void *) (Future),
	(void *) (OutputReady)
};


#endif // _INTERFACE