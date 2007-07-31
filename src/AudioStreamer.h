// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-
//======================================================================================
/*
    This file is part of DIMPLE, the Dynamic Interactive Musically PhysicaL Environment,

    This code is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License("GPL") version 2
    as published by the Free Software Foundation.  See the file LICENSE
    for more information.

    sinclair@music.mcgill.ca
    http://www.music.mcgill.ca/~sinclair/content/dimple
*/
//======================================================================================

#ifndef _AUDIOSTREAMER_H_
#define _AUDIOSTREAMER_H_

#include <samplerate.h>
#include "CircBuffer.h"

/*! Class to handle streaming of audio information from a fast thread
 *  to a slow thread. Performs resampling as necessary. */
class AudioStreamer
{
public:
    /*! Constructor must specify the input and output sample rates,
     *  the size of buffer to allocate in milliseconds, and the number
     *  of interleaved channels to expect. */
    AudioStreamer(unsigned int input_samplerate_hz,
                  unsigned int output_samplerate_hz,
                  unsigned int size_ms,
                  unsigned int channels);

    ~AudioStreamer();

    /*! Write samples to the FIFO at the input sample rate. */
    bool writeSamples(float *samples, unsigned int len);

    /*! Read samples from the FIFO at the output sample rate. */
    bool readSamples(float *samples, unsigned int len);

protected:
    unsigned int m_channels;
    unsigned int m_input_samplerate_hz;
    unsigned int m_output_samplerate_hz;
    double m_samplerate_ratio;
    CircBufferNoLock m_fifo;
    bool m_upsampling;
    SRC_DATA m_samplerate_data;
    SRC_STATE *m_samplerate_state;
    float *m_tmpbuffer;
};

#endif // _AUDIOSTREAMER_H_
