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

#ifndef _VALUETIMER_H_
#define _VALUETIMER_H_

#include "dimple.h"

class OscValue;

struct ValueTimerPair
{
    int current_ms;
    int interval_ms;
};

class ValueTimer
{
  public: 
    ValueTimer() {};
    virtual ~ValueTimer() {};

    void addValue(OscValue* oscval, int interval_ms);
    void removeValue(OscValue* oscval);

    void onTimer(int interval_ms);

  protected:
    std::map<OscValue*, struct ValueTimerPair*> m_values;
};

#endif // _VALUETIMER_H_
