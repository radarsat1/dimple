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

#ifndef _CIRC_BUFFER_H_
#define _CIRC_BUFFER_H_

#include <string.h>
#include <math.h>
#ifdef WIN32
#include <float.h>
#define ilogb(a) ((int)_logb((double)a))
#endif

/*! Class for writing to a circular buffer from one thread and reading
 *  from another without requiring locking. This code is adapted from
 *  the Linux kernel's kfifo.c.  It may only be called from one reader
 *  and one writer. */

class CircBufferNoLock
{
  public:
    /*! This constructor allocates memory for the circular buffer. If
     *  requested size is not a power of two, it may allocate more
     *  memory than requested. */
    CircBufferNoLock(unsigned int size) {
        m_size = 1 << (ilogb(size-1)+1); /* Ensure size is a power of two */
        m_readpos = 0;
        m_writepos = 0;
        m_buffer = new unsigned char[size];
        memset(m_buffer, 0, size);
    }
    ~CircBufferNoLock() {
        if (m_buffer)
            delete m_buffer;
    }

    /*! Write bytes to the buffer. Return true if successful. */
    bool writeBuffer(const unsigned char *data,
                     unsigned int len)
    {
        unsigned int left, rightside;
        
        left = m_size - m_writepos + m_readpos;
        if (left < len)
            return false;

        /* first put the data starting from m_writepos to buffer end */
        rightside = m_size - (m_writepos & (m_size - 1));
        rightside = (len<rightside) ? len : rightside;
        memcpy(m_buffer + (m_writepos & (m_size - 1)), data, rightside);
        
        /* then put the rest (if any) at the beginning of the buffer */
        memcpy(m_buffer, data + rightside, len - rightside);
        
        m_writepos += len;
        
        return true;
    }

    /*! Read bytes from the buffer. Return true if successful. */
    bool readBuffer(unsigned char *data,
                    unsigned int len)
    {
        unsigned int left, rightside;

        left = m_writepos - m_readpos;
        if (left < len)
            return false;
        
        /* first get the data from m_readpos until the end of the data */
        rightside = m_size - (m_readpos & (m_size - 1));
        rightside = (len<rightside) ? len : rightside;
        memcpy(data, m_buffer + (m_readpos & (m_size - 1)), rightside);
        
        /* then get the rest (if any) from the beginning of the data */
        memcpy(data + rightside, m_buffer, len - rightside);
        
        m_readpos += len;
        
        return true;
    }

    /*! Return the allocated size of the buffer. */
    unsigned int getSize() { return m_size; }

  protected:
    unsigned int m_size;
    unsigned int m_readpos;
    unsigned int m_writepos;
    unsigned char* m_buffer;
};

#endif // _CIRC_BUFFER_H_
