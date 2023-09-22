#ifndef _JACK_FUNCTIONS
#define _JACK_FUNCTIONS

#include <limits.h>

#include "./interface.h"
#include "./debug.h"

static inline int jack_buffer_size_callback(jack_nframes_t nframes, void *arg)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)arg;

	if(This->asio_driver_state != Running)
		return 0;

	if (This->asio_callbacks->asioMessage(kAsioSelectorSupported, kAsioResetRequest, 0 , 0))
		This->asio_callbacks->asioMessage(kAsioResetRequest, 0, 0, 0);
	return 0;
}

static inline void jack_latency_callback(jack_latency_callback_mode_t mode, void *arg)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)arg;

	if(This->asio_driver_state != Running)
		return;

	if (This->asio_callbacks->asioMessage(kAsioSelectorSupported, kAsioLatenciesChanged, 0 , 0))
		This->asio_callbacks->asioMessage(kAsioLatenciesChanged, 0, 0, 0);

	return;
}

static inline int jack_process_callback(jack_nframes_t nframes, void *arg)
{
	IWineASIOImpl               *This = (IWineASIOImpl*)arg;

	int                         i;
	jack_transport_state_t      jack_transport_state;
	jack_position_t             jack_position;
	DWORD                       time;

	/* output silence if the ASIO callback isn't running yet */
	if (This->asio_driver_state != Running)
	{
		for (i = 0; i < This->asio_active_outputs; i++)
			bzero(jack_port_get_buffer(This->output_channel[i].port, nframes), sizeof (jack_default_audio_sample_t) * nframes);
		return 0;
	}

	/* copy jack to asio buffers */
	for (i = 0; i < This->wineasio_number_inputs; i++)
		if (This->input_channel[i].active == ASIOTrue)
			memcpy (&This->input_channel[i].audio_buffer[nframes * This->asio_buffer_index],
					jack_port_get_buffer(This->input_channel[i].port, nframes),
					sizeof (jack_default_audio_sample_t) * nframes);

	if (This->asio_sample_position.lo > ULONG_MAX - nframes)
		This->asio_sample_position.hi++;
	This->asio_sample_position.lo += nframes;

	time = timeGetTime();
	This->asio_time_stamp.lo = time * 1000000;
	This->asio_time_stamp.hi = ((unsigned long long) time * 1000000) >> 32;

	if (This->asio_time_info_mode) /* use the newer bufferSwitchTimeInfo method if supported */
	{
		This->asio_time.timeInfo.samplePosition.lo = This->asio_sample_position.lo;
		This->asio_time.timeInfo.samplePosition.hi = This->asio_sample_position.hi;
		This->asio_time.timeInfo.systemTime.lo = This->asio_time_stamp.lo;
		This->asio_time.timeInfo.systemTime.hi = This->asio_time_stamp.hi;
		This->asio_time.timeInfo.sampleRate = This->asio_sample_rate;
		This->asio_time.timeInfo.flags = kSystemTimeValid | kSamplePositionValid | kSampleRateValid;

		if (This->asio_can_time_code) /* FIXME addionally use time code if supported */
		{
			jack_transport_state = jack_transport_query(This->jack_client, &jack_position);
			This->asio_time.timeCode.flags = kTcValid;
			if (jack_transport_state == JackTransportRolling)
				This->asio_time.timeCode.flags |= kTcRunning;
		}
		This->asio_callbacks->bufferSwitchTimeInfo(&This->asio_time, This->asio_buffer_index, ASIOTrue);
	}
	else
	{ /* use the old bufferSwitch method */
		This->asio_callbacks->bufferSwitch(This->asio_buffer_index, ASIOTrue);
	}

	/* copy asio to jack buffers */
	for (i = 0; i < This->wineasio_number_outputs; i++)
		if (This->output_channel[i].active == ASIOTrue)
			memcpy(jack_port_get_buffer(This->output_channel[i].port, nframes),
					&This->output_channel[i].audio_buffer[nframes * This->asio_buffer_index],
					sizeof (jack_default_audio_sample_t) * nframes);

	/* swith asio buffer */
	This->asio_buffer_index = This->asio_buffer_index ? 0 : 1;
	return 0;
}

static inline int jack_sample_rate_callback(jack_nframes_t nframes, void *arg)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)arg;

	if(This->asio_driver_state != Running)
		return 0;

	This->asio_sample_rate = nframes;
	This->asio_callbacks->sampleRateDidChange(nframes);
	return 0;
}

/*****************************************************************************
 *  Support functions
 */

/*
 *  Jack callbacks
 */


/* internal helper function for returning the posix thread_id of the newly created callback thread */
static DWORD WINAPI jack_thread_creator_helper(LPVOID arg)
{
	TRACE("arg: %p\n", arg);

	jack_thread_creator_privates.jack_callback_pthread_id = pthread_self();
	SetEvent(jack_thread_creator_privates.jack_callback_thread_created);
	jack_thread_creator_privates.jack_callback_thread(jack_thread_creator_privates.arg);
	return 0;
}


/* Function called by JACK to create a thread in the wine process context,
 *  uses the global structure jack_thread_creator_privates to communicate with jack_thread_creator_helper() */
static int jack_thread_creator(pthread_t* thread_id, const pthread_attr_t* attr, void *(*function)(void*), void* arg)
{
	TRACE("arg: %p, thread_id: %p, attr: %p, function: %p\n", arg, thread_id, attr, function);

	jack_thread_creator_privates.jack_callback_thread = function;
	jack_thread_creator_privates.arg = arg;
	jack_thread_creator_privates.jack_callback_thread_created = CreateEventW(NULL, FALSE, FALSE, NULL);
	CreateThread( NULL, 0, jack_thread_creator_helper, arg, 0,0 );
	WaitForSingleObject(jack_thread_creator_privates.jack_callback_thread_created, INFINITE);
	*thread_id = jack_thread_creator_privates.jack_callback_pthread_id;
	return 0;
}

#endif // _JACK_FUNCTIONS