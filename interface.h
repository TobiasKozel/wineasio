#ifndef _INTERFACE
#define _INTERFACE

#include "./constants.h"
#include "objbase.h"
#include <jack/jack.h>

#define IEEE754_64FLOAT 1
#undef NATIVE_INT64
#include "asio.h"
#define NATIVE_INT64

#include "./win32_hack.h"

/* Hide ELF symbols for the COM members - No need to to export them */
#define HIDDEN __attribute__((visibility("hidden")))

typedef struct IWineASIO* LPWINEASIO;

typedef struct IOChannel {
  ASIOBool active;
  jack_default_audio_sample_t* audio_buffer;
  char port_name[ASIO_MAX_NAME_LENGTH];
  jack_port_t* port;
} IOChannel;

/* structure needed to create the JACK callback thread in the wine process
 * context */
struct {
  void* (*jack_callback_thread)(void*);
  void* arg;
  pthread_t jack_callback_pthread_id;
  HANDLE jack_callback_thread_created;
} jack_thread_creator_privates;

enum { Loaded, Initialized, Prepared, Running };

/**
 * TODO
 */
enum DispatcherTypes {
  DispatcherTypes_none = 0,
  DispatcherTypes_buffer_size,
  DispatcherTypes_latency,
  DispatcherTypes_sample_rate
};

struct DispatcherAction {
  enum DispatcherTypes type;
  jack_nframes_t parameter;
};

/*****************************************************************************
 * IWineAsio interface
 */

#define INTERFACE IWineASIO
DECLARE_INTERFACE_(IWineASIO, IUnknown) {
  STDMETHOD_(HRESULT, QueryInterface)
  (THIS_ IID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)
  (THIS) PURE;
  STDMETHOD_(ULONG, Release)
  (THIS) PURE;
  STDMETHOD_(ASIOBool, Init)
  (THIS_ void* sysRef) PURE;
  STDMETHOD_(void, GetDriverName)
  (THIS_ char* name) PURE;
  STDMETHOD_(LONG, GetDriverVersion)
  (THIS) PURE;
  STDMETHOD_(void, GetErrorMessage)
  (THIS_ char* string) PURE;
  STDMETHOD_(ASIOError, Start)
  (THIS) PURE;
  STDMETHOD_(ASIOError, Stop)
  (THIS) PURE;
  STDMETHOD_(ASIOError, GetChannels)
  (THIS_ LONG * numInputChannels, LONG * numOutputChannels) PURE;
  STDMETHOD_(ASIOError, GetLatencies)
  (THIS_ LONG * inputLatency, LONG * outputLatency) PURE;
  STDMETHOD_(ASIOError, GetBufferSize)
  (THIS_ LONG * minSize, LONG * maxSize, LONG * preferredSize,
   LONG * granularity) PURE;
  STDMETHOD_(ASIOError, CanSampleRate)
  (THIS_ ASIOSampleRate sampleRate) PURE;
  STDMETHOD_(ASIOError, GetSampleRate)
  (THIS_ ASIOSampleRate * sampleRate) PURE;
  STDMETHOD_(ASIOError, SetSampleRate)
  (THIS_ ASIOSampleRate sampleRate) PURE;
  STDMETHOD_(ASIOError, GetClockSources)
  (THIS_ ASIOClockSource * clocks, LONG * numSources) PURE;
  STDMETHOD_(ASIOError, SetClockSource)
  (THIS_ LONG index) PURE;
  STDMETHOD_(ASIOError, GetSamplePosition)
  (THIS_ ASIOSamples * sPos, ASIOTimeStamp * tStamp) PURE;
  STDMETHOD_(ASIOError, GetChannelInfo)
  (THIS_ ASIOChannelInfo * info) PURE;
  STDMETHOD_(ASIOError, CreateBuffers)
  (THIS_ ASIOBufferInfo * bufferInfo, LONG numChannels, LONG bufferSize,
   ASIOCallbacks * asioCallbacks) PURE;
  STDMETHOD_(ASIOError, DisposeBuffers)
  (THIS) PURE;
  STDMETHOD_(ASIOError, ControlPanel)
  (THIS) PURE;
  STDMETHOD_(ASIOError, Future)
  (THIS_ LONG selector, void* opt) PURE;
  STDMETHOD_(ASIOError, OutputReady)
  (THIS) PURE;
};
#undef INTERFACE

typedef struct IWineASIOImpl {
  /* COM stuff */
  const IWineASIOVtbl* lpVtbl;
  LONG ref;

  /* The app's main window handle on windows, 0 on OS/X */
  HWND sys_ref;

  /* ASIO stuff */
  LONG asio_active_inputs;
  LONG asio_active_outputs;
  BOOL asio_buffer_index;
  ASIOCallbacks* asio_callbacks;
  BOOL asio_can_time_code;
  LONG asio_current_buffersize;
  INT asio_driver_state;
  ASIOSamples asio_sample_position;
  ASIOSampleRate asio_sample_rate;
  ASIOTime asio_time;
  BOOL asio_time_info_mode;
  ASIOTimeStamp asio_time_stamp;
  LONG asio_version;

  /* WineASIO configuration options */
  LONG wineasio_number_inputs;
  LONG wineasio_number_outputs;
  BOOL wineasio_autostart_server;
  BOOL wineasio_connect_to_hardware;
  LONG wineasio_fixed_buffersize;
  LONG wineasio_preferred_buffersize;

  /* JACK stuff */
  jack_client_t* jack_client;
  char jack_client_name[ASIO_MAX_NAME_LENGTH];
  int jack_num_input_ports;
  int jack_num_output_ports;
  const char** jack_input_ports;
  const char** jack_output_ports;

  /* jack process callback buffers */
  jack_default_audio_sample_t* callback_audio_buffer;
  IOChannel* input_channel;
  IOChannel* output_channel;

  struct DispatcherAction dispatcher_queue[DISPATCHER_QUEUE_SIZE];
} IWineASIOImpl;

/****************************************************************************
 *  Interface Methods
 */

/*
 *  as seen from the WineASIO source
 */

HIDDEN HRESULT __thiscall QueryInterface(LPWINEASIO iface, REFIID riid,
                                         void** ppvObject);
HIDDEN ULONG __thiscall AddRef(LPWINEASIO iface);
HIDDEN ULONG __thiscall Release(LPWINEASIO iface);
HIDDEN ASIOBool __thiscall Init(LPWINEASIO iface, void* sysRef);
HIDDEN void __thiscall GetDriverName(LPWINEASIO iface, char* name);
HIDDEN LONG __thiscall GetDriverVersion(LPWINEASIO iface);
HIDDEN void __thiscall GetErrorMessage(LPWINEASIO iface, char* string);
HIDDEN ASIOError __thiscall Start(LPWINEASIO iface);
HIDDEN ASIOError __thiscall Stop(LPWINEASIO iface);
HIDDEN ASIOError __thiscall GetChannels(LPWINEASIO iface,
                                        LONG* numInputChannels,
                                        LONG* numOutputChannels);
HIDDEN ASIOError __thiscall GetLatencies(LPWINEASIO iface, LONG* inputLatency,
                                         LONG* outputLatency);
HIDDEN ASIOError __thiscall GetBufferSize(LPWINEASIO iface, LONG* minSize,
                                          LONG* maxSize, LONG* preferredSize,
                                          LONG* granularity);
HIDDEN ASIOError __thiscall CanSampleRate(LPWINEASIO iface,
                                          ASIOSampleRate sampleRate);
HIDDEN ASIOError __thiscall GetSampleRate(LPWINEASIO iface,
                                          ASIOSampleRate* sampleRate);
HIDDEN ASIOError __thiscall SetSampleRate(LPWINEASIO iface,
                                          ASIOSampleRate sampleRate);
HIDDEN ASIOError __thiscall GetClockSources(LPWINEASIO iface,
                                            ASIOClockSource* clocks,
                                            LONG* numSources);
HIDDEN ASIOError __thiscall SetClockSource(LPWINEASIO iface, LONG index);
HIDDEN ASIOError __thiscall GetSamplePosition(LPWINEASIO iface,
                                              ASIOSamples* sPos,
                                              ASIOTimeStamp* tStamp);
HIDDEN ASIOError __thiscall GetChannelInfo(LPWINEASIO iface,
                                           ASIOChannelInfo* info);
HIDDEN ASIOError __thiscall CreateBuffers(LPWINEASIO iface,
                                          ASIOBufferInfo* bufferInfo,
                                          LONG numChannels, LONG bufferSize,
                                          ASIOCallbacks* asioCallbacks);
HIDDEN ASIOError __thiscall DisposeBuffers(LPWINEASIO iface);
HIDDEN ASIOError __thiscall ControlPanel(LPWINEASIO iface);
HIDDEN ASIOError __thiscall Future(LPWINEASIO iface, LONG selector, void* opt);
HIDDEN ASIOError __thiscall OutputReady(LPWINEASIO iface);

static const IWineASIOVtbl WineASIO_Vtbl = {(void*)QueryInterface,
                                            (void*)AddRef,
                                            (void*)Release,

                                            (void*)THISCALL(Init),
                                            (void*)THISCALL(GetDriverName),
                                            (void*)THISCALL(GetDriverVersion),
                                            (void*)THISCALL(GetErrorMessage),
                                            (void*)THISCALL(Start),
                                            (void*)THISCALL(Stop),
                                            (void*)THISCALL(GetChannels),
                                            (void*)THISCALL(GetLatencies),
                                            (void*)THISCALL(GetBufferSize),
                                            (void*)THISCALL(CanSampleRate),
                                            (void*)THISCALL(GetSampleRate),
                                            (void*)THISCALL(SetSampleRate),
                                            (void*)THISCALL(GetClockSources),
                                            (void*)THISCALL(SetClockSource),
                                            (void*)THISCALL(GetSamplePosition),
                                            (void*)THISCALL(GetChannelInfo),
                                            (void*)THISCALL(CreateBuffers),
                                            (void*)THISCALL(DisposeBuffers),
                                            (void*)THISCALL(ControlPanel),
                                            (void*)THISCALL(Future),
                                            (void*)THISCALL(OutputReady)};

#endif  // _INTERFACE