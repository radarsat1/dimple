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

#ifndef _LOQUEUE_H_
#define _LOQUEUE_H_

#include "CircBuffer.h"
#include "lo/lo.h"

/*! Class to use CircBufferNoLock for transmitting liblo messages. */
class LoQueue
{
public:
    LoQueue(int size) : m_fifo(size) {};

    /*! Write a lo_message to the FIFO queue. */
    bool write_lo_message(const char *path, lo_message m)
        { return true; }

    /*! Check for messages in raw queue memory and dispatch them if
     * any are found. */
    bool read_and_dispatch()
        { return false; }

protected:
    CircBufferNoLock m_fifo;
};

#endif // _LOQUEUE_H_
