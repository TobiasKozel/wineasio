/*
 * Copyright (C) 2006 Robert Reif
 * Portions copyright (C) 2007 Ralf Beck
 * Portions copyright (C) 2007 Johnny Petrantoni
 * Portions copyright (C) 2007 Stephane Letz
 * Portions copyright (C) 2008 William Steidtmann
 * Portions copyright (C) 2010 Peter L Jones
 * Portions copyright (C) 2010 Torben Hohn
 * Portions copyright (C) 2010 Nedko Arnaudov
 * Portions copyright (C) 2013 Joakim Hernberg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <jack/jack.h>
#include <jack/thread.h>



#include "objbase.h"
#include "mmsystem.h"
#include "winreg.h"

#include "./debug.h"
#include "./constants.h"
#include "./interface.h"
#include "./driver_configuration.h"
#include "./jack_functions.h"


HIDDEN HRESULT STDMETHODCALLTYPE QueryInterface(LPWINEASIO iface, REFIID riid, void **ppvObject)
{
	IWineASIOImpl   *This = (IWineASIOImpl *)iface;

	TRACE("iface: %p, riid: %s, ppvObject: %p)\n", iface, debugstr_guid(riid), ppvObject);

	if (ppvObject == NULL)
		return E_INVALIDARG;

	if (IsEqualIID(&CLSID_WineASIO, riid))
	{
		AddRef(iface);
		*ppvObject = This;
		return S_OK;
	}

	return E_NOINTERFACE;
}

/*
 * ULONG STDMETHODCALLTYPE AddRef(LPWINEASIO iface);
 * Function: Increment the reference count on the object
 * Returns:  Ref count
 */

HIDDEN ULONG STDMETHODCALLTYPE AddRef(LPWINEASIO iface)
{
	IWineASIOImpl   *This = (IWineASIOImpl *)iface;
	ULONG           ref = InterlockedIncrement(&(This->ref));

	TRACE("iface: %p, ref count is %d\n", iface, ref);
	return ref;
}

/*
 * ULONG Release (LPWINEASIO iface);
 *  Function:   Destroy the interface
 *  Returns:    Ref count
 *  Implies:    ASIOStop() and ASIODisposeBuffers()
 */

HIDDEN ULONG STDMETHODCALLTYPE Release(LPWINEASIO iface)
{
	IWineASIOImpl   *This = (IWineASIOImpl *)iface;
	ULONG            ref = InterlockedDecrement(&This->ref);
	int             i;

	TRACE("iface: %p, ref count is %d\n", iface, ref);

	if (This->asio_driver_state == Running)
		Stop(iface);
	if (This->asio_driver_state == Prepared)
		DisposeBuffers(iface);

	if (This->asio_driver_state == Initialized)
	{
		/* just for good measure we deinitialize IOChannel structures and unregister JACK ports */
		for (i = 0; i < This->wineasio_number_inputs; i++)
		{
			jack_port_unregister (This->jack_client, This->input_channel[i].port);
			This->input_channel[i].active = ASIOFalse;
			This->input_channel[i].port = NULL;
		}
		for (i = 0; i < This->wineasio_number_outputs; i++)
		{
			jack_port_unregister (This->jack_client, This->output_channel[i].port);
			This->output_channel[i].active = ASIOFalse;
			This->output_channel[i].port = NULL;
		}
		This->asio_active_inputs = This->asio_active_outputs = 0;
		TRACE("%i IOChannel structures released\n", This->wineasio_number_inputs + This->wineasio_number_outputs);

		jack_free (This->jack_output_ports);
		jack_free (This->jack_input_ports);
		jack_client_close(This->jack_client);
		if (This->input_channel)
			HeapFree(GetProcessHeap(), 0, This->input_channel);
	}
	TRACE("WineASIO terminated\n\n");
	if (ref == 0)
		HeapFree(GetProcessHeap(), 0, This);
	return ref;
}

/*
 * ASIOBool Init (void *sysRef);
 *  Function:   Initialize the driver
 *  Parameters: Pointer to "This"
 *              sysHanle is 0 on OS/X and on windows it contains the applications main window handle
 *  Returns:    ASIOFalse on error, and ASIOTrue on success
 */
HIDDEN ASIOBool STDMETHODCALLTYPE Init(LPWINEASIO iface, void *sysRef)
{
	IWineASIOImpl   *This = (IWineASIOImpl *)iface;
	jack_status_t   jack_status;
	jack_options_t  jack_options = This->wineasio_autostart_server ? JackNullOption : JackNoStartServer;
	int             i;

	This->sys_ref = sysRef;
	mlockall(MCL_FUTURE);
	configure_driver(This);

	if (!(This->jack_client = jack_client_open(This->jack_client_name, jack_options, &jack_status)))
	{
		WARN("Unable to open a JACK client as: %s\n", This->jack_client_name);
		return ASIOFalse;
	}
	TRACE("JACK client opened as: '%s'\n", jack_get_client_name(This->jack_client));

	This->asio_sample_rate = jack_get_sample_rate(This->jack_client);
	This->asio_current_buffersize = jack_get_buffer_size(This->jack_client);

	/* Allocate IOChannel structures */
	This->input_channel = HeapAlloc(GetProcessHeap(), 0, (This->wineasio_number_inputs + This->wineasio_number_outputs) * sizeof(IOChannel));
	if (!This->input_channel)
	{
		jack_client_close(This->jack_client);
		ERR("Unable to allocate IOChannel structures for %i channels\n", This->wineasio_number_inputs);
		return ASIOFalse;
	}
	This->output_channel = This->input_channel + This->wineasio_number_inputs;
	TRACE("%i IOChannel structures allocated\n", This->wineasio_number_inputs + This->wineasio_number_outputs);

	/* Get and count physical JACK ports */
	This->jack_input_ports = jack_get_ports(This->jack_client, NULL, NULL, JackPortIsPhysical | JackPortIsOutput);
	for (This->jack_num_input_ports = 0; This->jack_input_ports && This->jack_input_ports[This->jack_num_input_ports]; This->jack_num_input_ports++)
		;
	This->jack_output_ports = jack_get_ports(This->jack_client, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
	for (This->jack_num_output_ports = 0; This->jack_output_ports && This->jack_output_ports[This->jack_num_output_ports]; This->jack_num_output_ports++)
		;

	/* Initialize IOChannel structures */
	for (i = 0; i < This->wineasio_number_inputs; i++)
	{
		This->input_channel[i].active = ASIOFalse;
		This->input_channel[i].port = NULL;
		snprintf(This->input_channel[i].port_name, ASIO_MAX_NAME_LENGTH, "in_%i", i + 1);
		This->input_channel[i].port = jack_port_register(This->jack_client,
			This->input_channel[i].port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, i);
		/* TRACE("IOChannel structure initialized for input %d: '%s'\n", i, This->input_channel[i].port_name); */
	}
	for (i = 0; i < This->wineasio_number_outputs; i++)
	{
		This->output_channel[i].active = ASIOFalse;
		This->output_channel[i].port = NULL;
		snprintf(This->output_channel[i].port_name, ASIO_MAX_NAME_LENGTH, "out_%i", i + 1);
		This->output_channel[i].port = jack_port_register(This->jack_client,
			This->output_channel[i].port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, i);
		/* TRACE("IOChannel structure initialized for output %d: '%s'\n", i, This->output_channel[i].port_name); */
	}
	TRACE("%i IOChannel structures initialized\n", This->wineasio_number_inputs + This->wineasio_number_outputs);

	jack_set_thread_creator(jack_thread_creator);

	if (jack_set_buffer_size_callback(This->jack_client, jack_buffer_size_callback, This))
	{
		jack_client_close(This->jack_client);
		HeapFree(GetProcessHeap(), 0, This->input_channel);
		ERR("Unable to register JACK buffer size change callback\n");
		return ASIOFalse;
	}
	
	if (jack_set_latency_callback(This->jack_client, jack_latency_callback, This))
	{
		jack_client_close(This->jack_client);
		HeapFree(GetProcessHeap(), 0, This->input_channel);
		ERR("Unable to register JACK latency callback\n");
		return ASIOFalse;
	}


	if (jack_set_process_callback(This->jack_client, jack_process_callback, This))
	{
		jack_client_close(This->jack_client);
		HeapFree(GetProcessHeap(), 0, This->input_channel);
		ERR("Unable to register JACK process callback\n");
		return ASIOFalse;
	}

	if (jack_set_sample_rate_callback (This->jack_client, jack_sample_rate_callback, This))
	{
		jack_client_close(This->jack_client);
		HeapFree(GetProcessHeap(), 0, This->input_channel);
		ERR("Unable to register JACK sample rate change callback\n");
		return ASIOFalse;
	}

	This->asio_driver_state = Initialized;
	TRACE("WineASIO 0.%.1f initialized\n",(float) This->asio_version / 10);
	return ASIOTrue;
}

/*
 * void GetDriverName(char *name);
 *  Function:    Returns the driver name in name
 */
HIDDEN void STDMETHODCALLTYPE GetDriverName(LPWINEASIO iface, char *name)
{
	TRACE("iface: %p, name: %p\n", iface, name);
	strcpy(name, DRIVER_NAME);
	return;
}

/*
 * LONG GetDriverVersion (void);
 *  Function:    Returns the driver version number
 */
HIDDEN LONG STDMETHODCALLTYPE GetDriverVersion(LPWINEASIO iface)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)iface;

	TRACE("iface: %p\n", iface);
	return This->asio_version;
}

/*
 * void GetErrorMessage(char *string);
 *  Function:    Returns an error message for the last occured error in string
 */
HIDDEN void STDMETHODCALLTYPE GetErrorMessage(LPWINEASIO iface, char *string)
{
	TRACE("iface: %p, string: %p)\n", iface, string);
	strcpy(string, "WineASIO does not return error messages\n");
	return;
}

/*
 * ASIOError Start(void);
 *  Function:    Start JACK IO processing and reset the sample counter to zero
 *  Returns:     ASE_NotPresent if IO is missing
 *               ASE_HWMalfunction if JACK fails to start
 */
HIDDEN ASIOError STDMETHODCALLTYPE Start(LPWINEASIO iface)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)iface;
	int             i;
	DWORD           time;

	TRACE("iface: %p\n", iface);

	if (This->asio_driver_state != Prepared)
		return ASE_NotPresent;

	/* Zero the audio buffer */
	for (i = 0; i < (This->wineasio_number_inputs + This->wineasio_number_outputs) * 2 * This->asio_current_buffersize; i++)
		This->callback_audio_buffer[i] = 0;

	/* prime the callback by preprocessing one outbound ASIO bufffer */
	This->asio_buffer_index =  0;
	This->asio_sample_position.hi = This->asio_sample_position.lo = 0;

	time = timeGetTime();
	This->asio_time_stamp.lo = time * 1000000;
	This->asio_time_stamp.hi = ((unsigned long long) time * 1000000) >> 32;

	if (This->asio_time_info_mode) /* use the newer bufferSwitchTimeInfo method if supported */
	{
		This->asio_time.timeInfo.samplePosition.lo = This->asio_time.timeInfo.samplePosition.hi = 0;
		This->asio_time.timeInfo.systemTime.lo = This->asio_time_stamp.lo;
		This->asio_time.timeInfo.systemTime.hi = This->asio_time_stamp.hi;
		This->asio_time.timeInfo.sampleRate = This->asio_sample_rate;
		This->asio_time.timeInfo.flags = kSystemTimeValid | kSamplePositionValid | kSampleRateValid;

		if (This->asio_can_time_code) /* addionally use time code if supported */
		{
			This->asio_time.timeCode.speed = 1; /* FIXME */
			This->asio_time.timeCode.timeCodeSamples.lo = This->asio_time_stamp.lo;
			This->asio_time.timeCode.timeCodeSamples.hi = This->asio_time_stamp.hi;
			This->asio_time.timeCode.flags = ~(kTcValid | kTcRunning);
		}
		This->asio_callbacks->bufferSwitchTimeInfo(&This->asio_time, This->asio_buffer_index, ASIOTrue);
	} 
	else
	{ /* use the old bufferSwitch method */
		This->asio_callbacks->bufferSwitch(This->asio_buffer_index, ASIOTrue);
	}

	/* swith asio buffer */
	This->asio_buffer_index = This->asio_buffer_index ? 0 : 1;

	This->asio_driver_state = Running;
	TRACE("WineASIO successfully loaded\n");
	return ASE_OK;
}

/*
 * ASIOError Stop(void);
 *  Function:   Stop JACK IO processing
 *  Returns:    ASE_NotPresent on missing IO
 *  Note:       BufferSwitch() must not called after returning
 */
HIDDEN ASIOError STDMETHODCALLTYPE Stop(LPWINEASIO iface)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)iface;

	TRACE("iface: %p\n", iface);

	if (This->asio_driver_state != Running)
		return ASE_NotPresent;

	This->asio_driver_state = Prepared;

	return ASE_OK;
}

/*
 * ASIOError GetChannels(LONG *numInputChannels, LONG *numOutputChannels);
 *  Function:   Report number of IO channels
 *  Parameters: numInputChannels and numOutputChannels will hold number of channels on returning
 *  Returns:    ASE_NotPresent if no channels are available, otherwise AES_OK
 */
HIDDEN ASIOError STDMETHODCALLTYPE GetChannels (LPWINEASIO iface, LONG *numInputChannels, LONG *numOutputChannels)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)iface;

	if (!numInputChannels || !numOutputChannels)
		return ASE_InvalidParameter;

	*numInputChannels = This->wineasio_number_inputs;
	*numOutputChannels = This->wineasio_number_outputs;
	TRACE("iface: %p, inputs: %i, outputs: %i\n", iface, This->wineasio_number_inputs, This->wineasio_number_outputs);
	return ASE_OK;
}

/*
 * ASIOError GetLatencies(LONG *inputLatency, LONG *outputLatency);
 *  Function:   Return latency in frames
 *  Returns:    ASE_NotPresent if no IO is available, otherwise AES_OK
 */
HIDDEN ASIOError STDMETHODCALLTYPE GetLatencies(LPWINEASIO iface, LONG *inputLatency, LONG *outputLatency)
{
	IWineASIOImpl           *This = (IWineASIOImpl*)iface;
	jack_latency_range_t    range;

	if (!inputLatency || !outputLatency)
		return ASE_InvalidParameter;

	if (This->asio_driver_state == Loaded)
		return ASE_NotPresent;

	jack_port_get_latency_range(This->input_channel[0].port, JackCaptureLatency, &range);
	*inputLatency = range.max;
	jack_port_get_latency_range(This->output_channel[0].port, JackPlaybackLatency, &range);
	*outputLatency = range.max;
	TRACE("iface: %p, input latency: %d, output latency: %d\n", iface, *inputLatency, *outputLatency);

	return ASE_OK;
}

/*
 * ASIOError GetBufferSize(LONG *minSize, LONG *maxSize, LONG *preferredSize, LONG *granularity);
 *  Function:    Return minimum, maximum, preferred buffer sizes, and granularity
 *               At the moment return all the same, and granularity 0
 *  Returns:    ASE_NotPresent on missing IO
 */
HIDDEN ASIOError STDMETHODCALLTYPE GetBufferSize(LPWINEASIO iface, LONG *minSize, LONG *maxSize, LONG *preferredSize, LONG *granularity)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)iface;

	TRACE("iface: %p, minSize: %p, maxSize: %p, preferredSize: %p, granularity: %p\n", iface, minSize, maxSize, preferredSize, granularity);

	if (!minSize || !maxSize || !preferredSize || !granularity)
		return ASE_InvalidParameter;

	if (This->wineasio_fixed_buffersize)
	{
		*minSize = *maxSize = *preferredSize = This->asio_current_buffersize;
		*granularity = 0;
		TRACE("Buffersize fixed at %i\n", This->asio_current_buffersize);
		return ASE_OK;
	}

	*minSize = ASIO_MINIMUM_BUFFERSIZE;
	*maxSize = ASIO_MAXIMUM_BUFFERSIZE;
	*preferredSize = This->wineasio_preferred_buffersize;
	*granularity = -1;
	TRACE("The ASIO host can control buffersize\nMinimum: %i, maximum: %i, preferred: %i, granularity: %i, current: %i\n",
		  *minSize, *maxSize, *preferredSize, *granularity, This->asio_current_buffersize);
	return ASE_OK;
}

/*
 * ASIOError CanSampleRate(ASIOSampleRate sampleRate);
 *  Function:   Ask if specific SR is available
 *  Returns:    ASE_NoClock if SR isn't available, ASE_NotPresent on missing IO
 */
HIDDEN ASIOError STDMETHODCALLTYPE CanSampleRate(LPWINEASIO iface, ASIOSampleRate sampleRate)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)iface;

	TRACE("iface: %p, Samplerate = %li, requested samplerate = %li\n", iface, (long) This->asio_sample_rate, (long) sampleRate);

	if (sampleRate != This->asio_sample_rate)
		return ASE_NoClock;
	return ASE_OK;
}

/*
 * ASIOError GetSampleRate(ASIOSampleRate *currentRate);
 *  Function:   Return current SR
 *  Parameters: currentRate will hold SR on return, 0 if unknown
 *  Returns:    ASE_NoClock if SR is unknown, ASE_NotPresent on missing IO
 */
HIDDEN ASIOError STDMETHODCALLTYPE GetSampleRate(LPWINEASIO iface, ASIOSampleRate *sampleRate)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)iface;

	TRACE("iface: %p, Sample rate is %i\n", iface, (int) This->asio_sample_rate);

	if (!sampleRate)
		return ASE_InvalidParameter;

	*sampleRate = This->asio_sample_rate;
	return ASE_OK;
}

/*
 * ASIOError SetSampleRate(ASIOSampleRate sampleRate);
 *  Function:   Set requested SR, enable external sync if SR == 0
 *  Returns:    ASE_NoClock if unknown SR
 *              ASE_InvalidMode if current clock is external and SR != 0
 *              ASE_NotPresent on missing IO
 */
HIDDEN ASIOError STDMETHODCALLTYPE SetSampleRate(LPWINEASIO iface, ASIOSampleRate sampleRate)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)iface;

	TRACE("iface: %p, Sample rate %f requested\n", iface, sampleRate);

	if (sampleRate != This->asio_sample_rate)
		return ASE_NoClock;
	return ASE_OK;
}

/*
 * ASIOError GetClockSources(ASIOClockSource *clocks, LONG *numSources);
 *  Function:   Return available clock sources
 *  Parameters: clocks - a pointer to an array of ASIOClockSource structures.
 *              numSources - when called: number of allocated members
 *                         - on return: number of clock sources, the minimum is 1 - the internal clock
 *  Returns:    ASE_NotPresent on missing IO
 */
HIDDEN ASIOError STDMETHODCALLTYPE GetClockSources(LPWINEASIO iface, ASIOClockSource *clocks, LONG *numSources)
{
	TRACE("iface: %p, clocks: %p, numSources: %p\n", iface, clocks, numSources);

	if (!clocks || !numSources)
		return ASE_InvalidParameter;

	clocks->index = 0;
	clocks->associatedChannel = -1;
	clocks->associatedGroup = -1;
	clocks->isCurrentSource = ASIOTrue;
	strcpy(clocks->name, "Internal");
	*numSources = 1;
	return ASE_OK;
}

/*
 * ASIOError SetClockSource(LONG index);
 *  Function:   Set clock source
 *  Parameters: index returned by ASIOGetClockSources() - See asio.h for more details
 *  Returns:    ASE_NotPresent on missing IO
 *              ASE_InvalidMode may be returned if a clock can't be selected
 *              ASE_NoClock should not be returned
 */
HIDDEN ASIOError STDMETHODCALLTYPE SetClockSource(LPWINEASIO iface, LONG index)
{
	TRACE("iface: %p, index: %i\n", iface, index);

	if (index != 0)
		return ASE_NotPresent;
	return ASE_OK;
}

/*
 * ASIOError GetSamplePosition (ASIOSamples *sPos, ASIOTimeStamp *tStamp);
 *  Function:   Return sample position and timestamp
 *  Parameters: sPos holds the position on return, reset to 0 on ASIOStart()
 *              tStamp holds the system time of sPos
 *  Return:     ASE_NotPresent on missing IO
 *              ASE_SPNotAdvancing on missing clock
 */
HIDDEN ASIOError STDMETHODCALLTYPE GetSamplePosition(LPWINEASIO iface, ASIOSamples *sPos, ASIOTimeStamp *tStamp)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)iface;

	TRACE("iface: %p, sPos: %p, tStamp: %p\n", iface, sPos, tStamp);

	if (!sPos || !tStamp)
		return ASE_InvalidParameter;

	tStamp->lo = This->asio_time_stamp.lo;
	tStamp->hi = This->asio_time_stamp.hi;
	sPos->lo = This->asio_sample_position.lo;
	sPos->hi = 0; /* FIXME */

	return ASE_OK;
}

/*
 * ASIOError GetChannelInfo (ASIOChannelInfo *info);
 *  Function:   Retrive channel info. - See asio.h for more detail
 *  Returns:    ASE_NotPresent on missing IO
 */
HIDDEN ASIOError STDMETHODCALLTYPE GetChannelInfo(LPWINEASIO iface, ASIOChannelInfo *info)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)iface;

	/* TRACE("(iface: %p, info: %p\n", iface, info); */

	if (info->channel < 0 || (info->isInput ? info->channel >= This->wineasio_number_inputs : info->channel >= This->wineasio_number_outputs))
		return ASE_InvalidParameter;

	info->channelGroup = 0;
	info->type = ASIOSTFloat32LSB;

	if (info->isInput)
	{
		info->isActive = This->input_channel[info->channel].active;
		memcpy(info->name, This->input_channel[info->channel].port_name, ASIO_MAX_NAME_LENGTH);
	}
	else
	{
		info->isActive = This->output_channel[info->channel].active;
		memcpy(info->name, This->output_channel[info->channel].port_name, ASIO_MAX_NAME_LENGTH);
	}
	return ASE_OK;
}

/*
 * ASIOError CreateBuffers(ASIOBufferInfo *bufferInfo, LONG numChannels, LONG bufferSize, ASIOCallbacks *asioCallbacks);
 *  Function:   Allocate buffers for IO channels
 *  Parameters: bufferInfo      - pointer to an array of ASIOBufferInfo structures
 *              numChannels     - the total number of IO channels to be allocated
 *              bufferSize      - one of the buffer sizes retrieved with ASIOGetBufferSize()
 *              asioCallbacks   - pointer to an ASIOCallbacks structure
 *              See asio.h for more detail
 *  Returns:    ASE_NoMemory if impossible to allocate enough memory
 *              ASE_InvalidMode on unsupported bufferSize or invalid bufferInfo data
 *              ASE_NotPresent on missing IO
 */
HIDDEN ASIOError STDMETHODCALLTYPE CreateBuffers(LPWINEASIO iface, ASIOBufferInfo *bufferInfo, LONG numChannels, LONG bufferSize, ASIOCallbacks *asioCallbacks)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)iface;
	ASIOBufferInfo  *buffer_info = bufferInfo;
	int             i, j, k;

	TRACE("iface: %p, bufferInfo: %p, numChannels: %i, bufferSize: %i, asioCallbacks: %p\n", iface, bufferInfo, numChannels, bufferSize, asioCallbacks);

	if (This->asio_driver_state != Initialized)
		return ASE_NotPresent;

	if (!bufferInfo || !asioCallbacks)
		return ASE_InvalidMode;

	/* Check for invalid channel numbers */
	for (i = j = k = 0; i < numChannels; i++, buffer_info++)
	{
		if (buffer_info->isInput)
		{
			if (j++ >= This->wineasio_number_inputs)
			{
				WARN("Invalid input channel requested\n");
				return ASE_InvalidMode;
			}
		}
		else
		{
			if (k++  >= This->wineasio_number_outputs)
			{
				WARN("Invalid output channel requested\n");
				return ASE_InvalidMode;
			}
		}
	}

	/* set buf_size */
	if (This->wineasio_fixed_buffersize)
	{
		if (This->asio_current_buffersize != bufferSize)
			return ASE_InvalidMode;
		TRACE("Buffersize fixed at %i\n", This->asio_current_buffersize);
	}
	else
	{ /* fail if not a power of two and if out of range */
		if (!(bufferSize > 0 && !(bufferSize&(bufferSize-1))
				&& bufferSize >= ASIO_MINIMUM_BUFFERSIZE
				&& bufferSize <= ASIO_MAXIMUM_BUFFERSIZE))
		{
			WARN("Invalid buffersize %i requested\n", bufferSize);
			return ASE_InvalidMode;
		}
		else
		{
			if (This->asio_current_buffersize == bufferSize)
			{
				TRACE("Buffer size already set to %i\n", This->asio_current_buffersize);
			}
			else
			{
				This->asio_current_buffersize = bufferSize;
				if (jack_set_buffer_size(This->jack_client, This->asio_current_buffersize))
				{
					WARN("JACK is unable to set buffersize to %i\n", This->asio_current_buffersize);
					return ASE_HWMalfunction;
				}
				TRACE("Buffer size changed to %i\n", This->asio_current_buffersize);
			}
		}
	}

	/* print/discover ASIO host capabilities */
	This->asio_callbacks = asioCallbacks;
	This->asio_time_info_mode = This->asio_can_time_code = FALSE;

	TRACE("The ASIO host supports ASIO v%i: ", This->asio_callbacks->asioMessage(kAsioEngineVersion, 0, 0, 0));
	if (This->asio_callbacks->asioMessage(kAsioSelectorSupported, kAsioBufferSizeChange, 0 , 0))
		TRACE("kAsioBufferSizeChange ");
	if (This->asio_callbacks->asioMessage(kAsioSelectorSupported, kAsioResetRequest, 0 , 0))
		TRACE("kAsioResetRequest ");
	if (This->asio_callbacks->asioMessage(kAsioSelectorSupported, kAsioResyncRequest, 0 , 0))
		TRACE("kAsioResyncRequest ");
	if (This->asio_callbacks->asioMessage(kAsioSelectorSupported, kAsioLatenciesChanged, 0 , 0))
		TRACE("kAsioLatenciesChanged ");

	if (This->asio_callbacks->asioMessage(kAsioSupportsTimeInfo, 0, 0, 0))
	{
		TRACE("bufferSwitchTimeInfo ");
		This->asio_time_info_mode = TRUE;
		if (This->asio_callbacks->asioMessage(kAsioSupportsTimeCode,  0, 0, 0))
		{
			TRACE("TimeCode");
			This->asio_can_time_code = TRUE;
		}
	}
	else
		TRACE("BufferSwitch");
	TRACE("\n");

	/* Allocate audio buffers */

	This->callback_audio_buffer = HeapAlloc(GetProcessHeap(), 0,
		(This->wineasio_number_inputs + This->wineasio_number_outputs) * 2 * This->asio_current_buffersize * sizeof(jack_default_audio_sample_t));
	if (!This->callback_audio_buffer)
	{
		ERR("Unable to allocate %i ASIO audio buffers\n", This->wineasio_number_inputs + This->wineasio_number_outputs);
		return ASE_NoMemory;
	}
	TRACE("%i ASIO audio buffers allocated (%i kB)\n", This->wineasio_number_inputs + This->wineasio_number_outputs,
		  (int) ((This->wineasio_number_inputs + This->wineasio_number_outputs) * 2 * This->asio_current_buffersize * sizeof(jack_default_audio_sample_t) / 1024));

	for (i = 0; i < This->wineasio_number_inputs; i++)
		This->input_channel[i].audio_buffer = This->callback_audio_buffer + (i * 2 * This->asio_current_buffersize);
	for (i = 0; i < This->wineasio_number_outputs; i++)
		This->output_channel[i].audio_buffer = This->callback_audio_buffer + ((This->wineasio_number_inputs + i) * 2 * This->asio_current_buffersize);

	/* initialize ASIOBufferInfo structures */
	buffer_info = bufferInfo;
	This->asio_active_inputs = This->asio_active_outputs = 0;

	for (i = 0; i < This->wineasio_number_inputs; i++) {
		This->input_channel[i].active = ASIOFalse;
	}
	for (i = 0; i < This->wineasio_number_outputs; i++) {
		This->output_channel[i].active = ASIOFalse;
	}

	for (i = 0; i < numChannels; i++, buffer_info++)
	{
		if (buffer_info->isInput)
		{
			buffer_info->buffers[0] = &This->input_channel[buffer_info->channelNum].audio_buffer[0];
			buffer_info->buffers[1] = &This->input_channel[buffer_info->channelNum].audio_buffer[This->asio_current_buffersize];
			This->input_channel[buffer_info->channelNum].active = ASIOTrue;
			This->asio_active_inputs++;
			/* TRACE("ASIO audio buffer for channel %i as input %li created\n", i, This->asio_active_inputs); */
		}
		else
		{
			buffer_info->buffers[0] = &This->output_channel[buffer_info->channelNum].audio_buffer[0];
			buffer_info->buffers[1] = &This->output_channel[buffer_info->channelNum].audio_buffer[This->asio_current_buffersize];
			This->output_channel[buffer_info->channelNum].active = ASIOTrue;
			This->asio_active_outputs++;
			/* TRACE("ASIO audio buffer for channel %i as output %li created\n", i, This->asio_active_outputs); */
		}
	}
	TRACE("%i audio channels initialized\n", This->asio_active_inputs + This->asio_active_outputs);

	if (jack_activate(This->jack_client))
		return ASE_NotPresent;

	/* connect to the hardware io */
	if (This->wineasio_connect_to_hardware)
	{
		for (i = 0; i < This->jack_num_input_ports && i < This->wineasio_number_inputs; i++)
			if (strstr(jack_port_type(jack_port_by_name(This->jack_client, This->jack_input_ports[i])), "audio"))
				jack_connect(This->jack_client, This->jack_input_ports[i], jack_port_name(This->input_channel[i].port));
		for (i = 0; i < This->jack_num_output_ports && i < This->wineasio_number_outputs; i++)
			if (strstr(jack_port_type(jack_port_by_name(This->jack_client, This->jack_output_ports[i])), "audio"))
				jack_connect(This->jack_client, jack_port_name(This->output_channel[i].port), This->jack_output_ports[i]);
	}

	/* at this point all the connections are made and the jack process callback is outputting silence */
	This->asio_driver_state = Prepared;
	return ASE_OK;
}

/*
 * ASIOError DisposeBuffers(void);
 *  Function:   Release allocated buffers
 *  Returns:    ASE_InvalidMode if no buffers were previously allocated
 *              ASE_NotPresent on missing IO
 *  Implies:    ASIOStop()
 */
HIDDEN ASIOError STDMETHODCALLTYPE DisposeBuffers(LPWINEASIO iface)
{
	IWineASIOImpl   *This = (IWineASIOImpl*)iface;
	int             i;

	TRACE("iface: %p\n", iface);

	if (This->asio_driver_state == Running)
		Stop (iface);
	if (This->asio_driver_state != Prepared)
		return ASE_NotPresent;

	if (jack_deactivate(This->jack_client))
		return ASE_NotPresent;

	This->asio_callbacks = NULL;

	for (i = 0; i < This->wineasio_number_inputs; i++)
	{
		This->input_channel[i].audio_buffer = NULL;
		This->input_channel[i].active = ASIOFalse;
	}
	for (i = 0; i < This->wineasio_number_outputs; i++)
	{
		This->output_channel[i].audio_buffer = NULL;
		This->output_channel[i].active = ASIOFalse;
	}
	This->asio_active_inputs = This->asio_active_outputs = 0;

	if (This->callback_audio_buffer)
		HeapFree(GetProcessHeap(), 0, This->callback_audio_buffer);

	This->asio_driver_state = Initialized;
	return ASE_OK;
}

/*
 * ASIOError ControlPanel(void);
 *  Function:   Open a control panel for driver settings
 *  Returns:    ASE_NotPresent if no control panel exists.  Actually return code should be ignored
 *  Note:       Call the asioMessage callback if something has changed
 */
HIDDEN ASIOError STDMETHODCALLTYPE ControlPanel(LPWINEASIO iface)
{
	static char arg0[] = "wineasio-settings\0";
	static char *arg_list[] = { arg0, NULL };

	TRACE("iface: %p\n", iface);

	if (vfork() == 0)
	{
		execvp (arg0, arg_list);
		_exit(1);
	}
	return ASE_OK;
}

/*
 * ASIOError Future(LONG selector, void *opt);
 *  Function:   Various, See asio.h for more detail
 *  Returns:    Depends on the selector but in general ASE_InvalidParameter on invalid selector
 *              ASE_InvalidParameter if function is unsupported to disable further calls
 *              ASE_SUCCESS on success, do not use AES_OK
 */
HIDDEN ASIOError STDMETHODCALLTYPE Future(LPWINEASIO iface, LONG selector, void *opt)
{
	IWineASIOImpl           *This = (IWineASIOImpl *) iface;

	TRACE("iface: %p, selector: %i, opt: %p\n", iface, selector, opt);

	switch (selector)
	{
		case kAsioEnableTimeCodeRead:
			This->asio_can_time_code = TRUE;
			TRACE("The ASIO host enabled TimeCode\n");
			return ASE_SUCCESS;
		case kAsioDisableTimeCodeRead:
			This->asio_can_time_code = FALSE;
			TRACE("The ASIO host disabled TimeCode\n");
			return ASE_SUCCESS;
		case kAsioSetInputMonitor:
			TRACE("The driver denied request to set input monitor\n");
			return ASE_NotPresent;
		case kAsioTransport:
			TRACE("The driver denied request for ASIO Transport control\n");
			return ASE_InvalidParameter;
		case kAsioSetInputGain:
			TRACE("The driver denied request to set input gain\n");
			return ASE_InvalidParameter;
		case kAsioGetInputMeter:
			TRACE("The driver denied request to get input meter \n");
			return ASE_InvalidParameter;
		case kAsioSetOutputGain:
			TRACE("The driver denied request to set output gain\n");
			return ASE_InvalidParameter;
		case kAsioGetOutputMeter:
			TRACE("The driver denied request to get output meter\n");
			return ASE_InvalidParameter;
		case kAsioCanInputMonitor:
			TRACE("The driver does not support input monitor\n");
			return ASE_InvalidParameter;
		case kAsioCanTimeInfo:
			TRACE("The driver supports TimeInfo\n");
			return ASE_SUCCESS;
		case kAsioCanTimeCode:
			TRACE("The driver supports TimeCode\n");
			return ASE_SUCCESS;
		case kAsioCanTransport:
			TRACE("The driver denied request for ASIO Transport\n");
			return ASE_InvalidParameter;
		case kAsioCanInputGain:
			TRACE("The driver does not support input gain\n");
			return ASE_InvalidParameter;
		case kAsioCanInputMeter:
			TRACE("The driver does not support input meter\n");
			return ASE_InvalidParameter;
		case kAsioCanOutputGain:
			TRACE("The driver does not support output gain\n");
			return ASE_InvalidParameter;
		case kAsioCanOutputMeter:
			TRACE("The driver does not support output meter\n");
			return ASE_InvalidParameter;
		case kAsioSetIoFormat:
			TRACE("The driver denied request to set DSD IO format\n");
			return ASE_NotPresent;
		case kAsioGetIoFormat:
			TRACE("The driver denied request to get DSD IO format\n");
			return ASE_NotPresent;
		case kAsioCanDoIoFormat:
			TRACE("The driver does not support DSD IO format\n");
			return ASE_NotPresent;
		default:
			TRACE("ASIOFuture() called with undocumented selector\n");
			return ASE_InvalidParameter;
	}
}

/*
 * ASIOError OutputReady(void);
 *  Function:   Tells the driver that output bufffers are ready
 *  Returns:    ASE_OK if supported
 *              ASE_NotPresent to disable
 */
HIDDEN ASIOError STDMETHODCALLTYPE OutputReady(LPWINEASIO iface)
{
	/* disabled to stop stand alone NI programs from spamming the console
	TRACE("iface: %p\n", iface); */
	return ASE_NotPresent;
}

/* Allocate the interface pointer and associate it with the vtbl/WineASIO object */
HRESULT WINAPI WineASIOCreateInstance(REFIID riid, LPVOID *ppobj)
{
	IWineASIOImpl   *pobj;

	/* TRACE("riid: %s, ppobj: %p\n", debugstr_guid(riid), ppobj); */

	pobj = HeapAlloc(GetProcessHeap(), 0, sizeof(*pobj));
	if (pobj == NULL)
	{
		WARN("out of memory\n");
		return E_OUTOFMEMORY;
	}

	pobj->lpVtbl = &WineASIO_Vtbl;
	pobj->ref = 1;
	TRACE("pobj = %p\n", pobj);
	*ppobj = pobj;
	/* TRACE("return %p\n", *ppobj); */
	return S_OK;
}

