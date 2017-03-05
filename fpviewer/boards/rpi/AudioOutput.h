/*
 * BCFlight
 * Copyright (C) 2017 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <Thread.h>
#include <Link.h>
#include <alsa/asoundlib.h>

class AudioOutput : public Thread
{
public:
	AudioOutput( Link* link, uint8_t channels = 1, uint32_t samplerate = 44100, std::string device = "default" );
	~AudioOutput();

protected:
	virtual bool run();
	Link* mLink;
	snd_pcm_t* mPCM;
	uint32_t mChannels;
};

#endif // AUDIOOUTPUT_H
