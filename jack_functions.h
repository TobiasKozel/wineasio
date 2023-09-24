#ifndef _JACK_FUNCTIONS
#define _JACK_FUNCTIONS

#include <jack/types.h>
#include <limits.h>
#include <pthread.h>

#include "./interface.h"
#include "./debug.h"

/**
 * Called from jack when the buffer size changes
 * called from an arbitray thread, this causes issues with wine.
 */
static inline int jack_buffer_size_callback(jack_nframes_t nframes, void* arg) {
  IWineASIOImpl* This = (IWineASIOImpl*)arg;
  if (This->asio_driver_state != Running) return 0;
  TRACE("Request Buffer Size change to %u", nframes);
  This->reset_requested = 1;
  return JackFailure;
}

/**
 * Called from jack when the latency changes
 * called from an arbitray thread, this causes issues with wine.
 */
static inline void jack_latency_callback(jack_latency_callback_mode_t mode,
                                         void* arg) {
  IWineASIOImpl* This = (IWineASIOImpl*)arg;

  if (This->asio_driver_state != Running) return;

  if (mode == JackCaptureLatency) {
    TRACE("Request Latency change for JackCaptureLatency");
  } else {
    TRACE("Request Latency change for JackPlaybackLatency");
  }

  This->reset_requested = 1;
  return;
}

/**
 * Called from jack when the sample rate changes
 * called from an arbitray thread, this causes issues with wine.
 */
static inline int jack_sample_rate_callback(jack_nframes_t nframes, void* arg) {
  IWineASIOImpl* This = (IWineASIOImpl*)arg;

  if (This->asio_driver_state != Running) return 0;

  TRACE("Request Sample rate change to %i", nframes);

  This->reset_requested = 1;
  return 0;
}

#ifdef DEBUG
int jack_process_callback_started = 0;
#endif  // DEBUG

/**
 * This is guaranteed to be called from the thread created in @see
 * jack_thread_creator setup via @see jack_set_thread_creator
 */
static inline int jack_process_callback(jack_nframes_t nframes, void* arg) {
  IWineASIOImpl* This = (IWineASIOImpl*)arg;

  int i;
  jack_transport_state_t jack_transport_state;
  jack_position_t jack_position;
  DWORD time;

  if (This->reset_requested) {
    This->reset_requested = 0;
    if (This->asio_callbacks->asioMessage(kAsioSelectorSupported,
                                          kAsioResetRequest, 0, 0)) {
      This->asio_callbacks->asioMessage(kAsioResetRequest, 0, 0, 0);
    }
    return 0;
  }

  /* output silence if the ASIO callback isn't running yet */
  if (This->asio_driver_state != Running) {
    for (i = 0; i < This->asio_active_outputs; i++)
      bzero(jack_port_get_buffer(This->output_channel[i].port, nframes),
            sizeof(jack_default_audio_sample_t) * nframes);
    return 0;
  }

#ifdef DEBUG
  if (jack_process_callback_started == 0) {
    TRACE("First block processed");
    jack_process_callback_started = 1;
  }
#endif

  /* copy jack to asio buffers */
  for (i = 0; i < This->wineasio_number_inputs; i++)
    if (This->input_channel[i].active == ASIOTrue)
      memcpy(&This->input_channel[i]
                  .audio_buffer[nframes * This->asio_buffer_index],
             jack_port_get_buffer(This->input_channel[i].port, nframes),
             sizeof(jack_default_audio_sample_t) * nframes);

  if (This->asio_sample_position.lo > ULONG_MAX - nframes)
    This->asio_sample_position.hi++;
  This->asio_sample_position.lo += nframes;

  time = timeGetTime();
  This->asio_time_stamp.lo = time * 1000000;
  This->asio_time_stamp.hi = ((unsigned long long)time * 1000000) >> 32;

  if (This->asio_time_info_mode) /* use the newer bufferSwitchTimeInfo method
                                    if supported */
  {
    This->asio_time.timeInfo.samplePosition.lo = This->asio_sample_position.lo;
    This->asio_time.timeInfo.samplePosition.hi = This->asio_sample_position.hi;
    This->asio_time.timeInfo.systemTime.lo = This->asio_time_stamp.lo;
    This->asio_time.timeInfo.systemTime.hi = This->asio_time_stamp.hi;
    This->asio_time.timeInfo.sampleRate = This->asio_sample_rate;
    This->asio_time.timeInfo.flags =
        kSystemTimeValid | kSamplePositionValid | kSampleRateValid;

    if (This->asio_can_time_code) /* FIXME addionally use time code if
                                   * supported
                                   */
    {
      jack_transport_state =
          jack_transport_query(This->jack_client, &jack_position);
      This->asio_time.timeCode.flags = kTcValid;
      if (jack_transport_state == JackTransportRolling)
        This->asio_time.timeCode.flags |= kTcRunning;
    }
    This->asio_callbacks->bufferSwitchTimeInfo(
        &This->asio_time, This->asio_buffer_index, ASIOTrue);
  } else { /* use the old bufferSwitch method */
    This->asio_callbacks->bufferSwitch(This->asio_buffer_index, ASIOTrue);
  }

  /* copy asio to jack buffers */
  for (i = 0; i < This->wineasio_number_outputs; i++)
    if (This->output_channel[i].active == ASIOTrue)
      memcpy(jack_port_get_buffer(This->output_channel[i].port, nframes),
             &This->output_channel[i]
                  .audio_buffer[nframes * This->asio_buffer_index],
             sizeof(jack_default_audio_sample_t) * nframes);

  /* swith asio buffer */
  This->asio_buffer_index = This->asio_buffer_index ? 0 : 1;
  return 0;
}

/*****************************************************************************
 *  Support functions
 */

/**
 * internal helper function for returning the posix thread_id of the newly
 * created callback thread
 */
static DWORD WINAPI jack_wine_callback_thread(LPVOID arg) {
  jack_thread_creator_privates.jack_callback_pthread_id = pthread_self();
  TRACE("wine jack callback thread started");
  SetEvent(jack_thread_creator_privates.jack_callback_thread_created);

  // This function will block until the asio driver is closed.
  jack_thread_creator_privates.jack_callback_thread(
      jack_thread_creator_privates.arg);

  TRACE("wine jack thread stopped");
  return 0;
}

static DWORD WINAPI jack_wine_control_thread(LPVOID arg) {
  TRACE("wine jack control thread started");
  SetEvent(arg);

  // This function will block until the asio driver is closed.
  // jack_thread_creator_privates.jack_callback_thread(
  //     jack_thread_creator_privates.arg);

  TRACE("wine jack thread stopped");
  return 0;
}

/**
 * Function called by JACK to create a thread in the wine process context,
 * uses the global structure jack_thread_creator_privates to communicate with
 * jack_thread_creator_helper()
 */
static int jack_thread_creator(pthread_t* thread_id, const pthread_attr_t* attr,
                               void* (*function)(void*), void* arg) {
  HANDLE control_thread_created;
  TRACE("arg: %p, thread: %lu, attr: %p, function: %p", arg, *thread_id, attr,
        function);

  jack_thread_creator_privates.jack_callback_thread = function;
  jack_thread_creator_privates.arg = arg;
  jack_thread_creator_privates.jack_callback_thread_created =
      CreateEventW(NULL, FALSE, FALSE, NULL);
  control_thread_created = CreateEventW(NULL, FALSE, FALSE, NULL);
  CreateThread(NULL, 0, jack_wine_callback_thread, arg, 0, 0);
  CreateThread(NULL, 0, jack_wine_control_thread, control_thread_created, 0, 0);
  WaitForSingleObject(jack_thread_creator_privates.jack_callback_thread_created,
                      INFINITE);
  WaitForSingleObject(control_thread_created, INFINITE);
  *thread_id = jack_thread_creator_privates.jack_callback_pthread_id;
  return 0;
}

#endif  // _JACK_FUNCTIONS
