// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-
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

#ifndef _LOQUEUE_H_
#define _LOQUEUE_H_

#include <cassert>

#include "CircBuffer.h"
#include "lo/lo.h"

/*! Class to use CircBufferNoLock for transmitting liblo messages. */
class LoQueue
{
public:
    LoQueue(int size) : m_fifo(size), m_readsize(0) {};

    /*! Write a lo_message to the FIFO queue. */
    bool write_lo_message(const char *path, lo_message m)
    {
        unsigned char msgbuf[1024];
        size_t msgbufsize = 1024-sizeof(size_t);

        lo_message_serialise(m, path, msgbuf+sizeof(size_t), &msgbufsize);
        if (msgbufsize <= 0)
            return false;

        *((size_t*)msgbuf) = msgbufsize;

        return m_fifo.writeBuffer((unsigned char*)msgbuf,
                                  msgbufsize+sizeof(size_t));
    }

    /*! Check for messages in raw queue memory and dispatch them if
     * any are found. */
    bool read_and_dispatch(lo_server s)
    {
        if (m_readsize == 0) {
            if (!m_fifo.readBuffer((unsigned char*)&m_readsize,
                                   sizeof(size_t)))
                return false;
        }

        assert(m_readsize < 1024);

        if (m_readsize > 0) {
            unsigned char buffer[1024];
            if (!m_fifo.readBuffer(buffer, m_readsize))
                return false;

            lo_server_dispatch_data(s, buffer, m_readsize);
            m_readsize = 0;
            return true;
        }

        return false;
    }

    size_t size() { return m_fifo.getSize(); }

protected:
    CircBufferNoLock m_fifo;
    size_t m_readsize;
};

#endif // _LOQUEUE_H_
