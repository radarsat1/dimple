
#include "AudioStreamer.h"
#include <stdio.h>

AudioStreamer::AudioStreamer(unsigned int input_samplerate_hz,
                             unsigned int output_samplerate_hz,
                             unsigned int size_ms,
                             unsigned int channels)
  : m_fifo((output_samplerate_hz * size_ms / 1000) * channels)
{
    m_input_samplerate_hz = input_samplerate_hz;
    m_output_samplerate_hz = output_samplerate_hz;
    m_samplerate_ratio = (double)output_samplerate_hz / (double)input_samplerate_hz;
    m_channels = channels;
    m_upsampling = m_input_samplerate_hz < m_output_samplerate_hz;

    /* libsamplerate initialization */
    m_samplerate_state = src_new(SRC_SINC_FASTEST, m_channels, NULL);
    if (!m_samplerate_state)
        printf("Error in libsamplerate src_new()\n");

    m_tmpbuffer.resize(m_fifo.getSize());
}

AudioStreamer::~AudioStreamer()
{
    if (m_samplerate_state)
        src_delete(m_samplerate_state);
}

/*! Write samples to the FIFO at the input sample rate. */
bool AudioStreamer::writeSamples(float *samples, unsigned int len)
{
    // When downsampling, resampling happens on write.
    if (!m_upsampling) {
        m_samplerate_data.data_in = samples;
        m_samplerate_data.data_out = m_tmpbuffer.data();
        m_samplerate_data.input_frames = len;
        m_samplerate_data.output_frames = m_fifo.getSize();
        m_samplerate_data.src_ratio = m_samplerate_ratio;
        m_samplerate_data.end_of_input = 0;

        src_process(m_samplerate_state, &m_samplerate_data);

        return
        m_fifo.writeBuffer((const unsigned char*)m_tmpbuffer.data(),
                           (int)(len * m_samplerate_ratio) * sizeof(float) * m_channels);
    }
    else {
        return
        m_fifo.writeBuffer((const unsigned char*)samples,
                           len * sizeof(float) * m_channels);
    }
}

/*! Read samples from the FIFO at the output sample rate. */
bool AudioStreamer::readSamples(float *samples, unsigned int len)
{
    // When upsampling, resampling happens on read.
    if (m_upsampling) {
        if (!m_fifo.readBuffer((unsigned char*)m_tmpbuffer.data(),
                               len * sizeof(float) * m_channels))
            return false;

        m_samplerate_data.data_in = m_tmpbuffer.data();
        m_samplerate_data.data_out = samples;
        m_samplerate_data.input_frames = (unsigned int)(len * m_samplerate_ratio);
        m_samplerate_data.output_frames = len;
        m_samplerate_data.src_ratio = m_samplerate_ratio;
        m_samplerate_data.end_of_input = 0;

        src_process(m_samplerate_state, &m_samplerate_data);
    }
    else {
        return
        m_fifo.readBuffer((unsigned char*)samples,
                          len * sizeof(float) * m_channels);
    }
}

