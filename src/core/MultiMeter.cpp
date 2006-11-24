/*
    Copyright (C) 2006 Nicola Doebelin

    This file is part of Traverso

    Traverso is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.

    $Id: MultiMeter.cpp,v 1.3 2006/11/24 18:44:30 n_doebelin Exp $
*/

#include "MultiMeter.h"
#include <AudioBus.h>
#include <AudioDevice.h>

#include <math.h>

// Always put me below _all_ includes, this is needed
// in case we run with memory leak detection enabled!
#include "Debugger.h"
		
#define SMOOTH_FACTOR	1

MultiMeter::MultiMeter()
{
	// constructs a ringbuffer that can hold 150 MultiMeterData structs
	m_databuffer = new RingBufferNPT<MultiMeterData>(150);
	
	// Initialize member variables, that need to be initialized
	calculate_fract();
	// With memset, we're able to very efficiently set all bytes of an array
	// or struct to zero
	memset(&m_history, 0, sizeof(MultiMeterData));
	
	connect(&audiodevice(), SIGNAL(driverParamsChanged()), this, SLOT(calculate_fract()));
}

MultiMeter::~MultiMeter()
{
	delete m_databuffer;
}


void MultiMeter::process(AudioBus* bus, nframes_t nframes)
{
	// check if audiobus is stereo (2 channels)
	// if not, do nothing
	if (bus->get_channel_count() != 2)
		return;

	// The nframes is the amount of samples there are in the buffers
	// we have to process. No need to get the buffersize, we _have_ to
	// use the nframes variable !
	audio_sample_t* bufferLeft = bus->get_buffer(0, nframes);
	audio_sample_t* bufferRight = bus->get_buffer(1, nframes);

	
	// Variables we need to calculate the correlation and avarages/levels
	float a1, a2, a1a2 = 0, a1sq = 0, a2sq = 0, r, levelLeft = 0, levelRight = 0;
	
	// calculate coefficient
	for (uint i = 0; i < nframes; ++i) {
		a1 = bufferLeft[i];
		a2 = bufferRight[i];

		a1a2 += a1 * a2;
		a1sq += a1 * a1;
		a2sq += a2 * a2;

		levelLeft += a1sq;
		levelRight += a2sq;
	}

	// We have all data to calculate the correlation coefficient
	// for the processed buffer (but check for division by 0 first)
	if ((a1sq == 0.0) || (a2sq == 0.0)) {
		r = 1.0;
	} else {
		r = a1a2 / (sqrtf(a1sq) * sqrtf(a2sq));
	}

	// calculate RMS of the levels
	levelLeft = sqrtf(levelLeft / nframes);
	levelRight = sqrtf(levelRight / nframes);

	// And we store this in a MultiMeterData struct
	// and write this struct into the data ringbuffer,
	// to be processed later in get_data()
	// levelLeft and levelRight are also needed to calculate the
	// correct direction in get_data(), so we store that too!
	MultiMeterData data;
	data.r = r;
	data.levelLeft = levelLeft;
	data.levelRight = levelRight;

	// The ringbuffer::write function acts like it's appending the data
	// to the end of the buffer.
	// The amount of MultiMeterData structs we want to write is 1, and 
	// we have to provide a pointer to the data we want to write, which
	// is done by dereferencing (the & in front of) data.
	//
	// This would also have worked (since it's essentially the same):
	// MultiMeterData* datatowrite = &data;
	// m_databuffer->write(datatowrite, 1);
	// 
	// If we want to write more then 1 MultiMeterData struct, we have to 
	// place them into an array, but well, there's only one now :-)
	m_databuffer->write(&data, 1);
}

/**
 * Compute the correlation coefficient of the stereo master output data
 * of the active song, and the direction. When there is new data, the new
 * data will be assigned to the \a r and \a direction variables, else no 
 * data will be assigned to those variables
 * 
 * r: linear correlation coefficient (1.0: complete positive correlation,
 * 0.0: uncorrelated, -1.0: complete negative correlation).
 *
 * @returns 0 if no new data was available, > 0 when new data was available
 *	The new data will be assigned to \a r and \a direction
 **/
int MultiMeter::get_data(float& r, float& direction)
{
	// RingBuffer::read_space() tells us how many data
	// of type T (MultiMeterData in this case) has been written 
	// to the buffer since last time we checked.
	int readcount = m_databuffer->read_space();
	
	// If there is no new data in the ringbuffer, leave it alone and return zero
	if (readcount == 0) {
		return 0;
	}
	
	// We need to know the 'history' of these variables to get a smooth
	// and consistent (independend of buffersizes) stereometer behaviour.
	// So we get it from our history struct.
	r = m_history.r;
	float levelLeft = m_history.levelLeft;
	float levelRight = m_history.levelRight;
	
	// Create an empty MultiMeterData struct data,
	MultiMeterData data;
	
	for (int i=0; i<readcount; ++i) {
		// which we fill by reading from the databuffer.
		m_databuffer->read(&data, 1);
		
		// Calculate the new correlation variable, and merge the old one.
		// Assign it to r itself, this spares a temp. variable for r ;-)
		r = data.r * m_fract + r * (1.0 - m_fract);
	
		// Same for levelLeft/Right
		levelLeft = data.levelLeft * m_fract + levelLeft * (1.0 - m_fract);
		levelRight = data.levelRight * m_fract + levelRight * (1.0 - m_fract);
	}
	
	// Now that we truely have taken into account all the levelLeft/Right data
	// for all buffers that have been processed since last call to get_data()
	// we now can calculate the direction variable.
	if ( ! ((levelLeft + levelRight) == 0.0) ) {
		float vl = levelLeft / (levelLeft + levelRight);
		float vr = levelRight / (levelLeft + levelRight);
	
		direction = vr - vl;
	} else {
		direction = 0;
	}

	// Store the calculated variables in the history struct, to be used on
	// next call of this function
	m_history.r = r;
	m_history.levelLeft = levelLeft;
	m_history.levelRight = levelRight;

	return readcount;
}

void MultiMeter::calculate_fract( )
{
	m_fract = ((float) audiodevice().get_buffer_size()) / (audiodevice().get_sample_rate() * SMOOTH_FACTOR);
}


// EOF
