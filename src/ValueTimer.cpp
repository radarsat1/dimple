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

#include "ValueTimer.h"
#include "OscObject.h"

typedef std::map<OscValue*, struct ValueTimerPair*>::iterator value_iter;
typedef std::pair<OscValue*, struct ValueTimerPair*> value_pair;

void ValueTimer::addValue(OscValue* oscval, int interval_ms)
{
    value_iter it;
    it = m_values.find(oscval);
    if (it==m_values.end()) {
        struct ValueTimerPair *v = new struct ValueTimerPair;
        v->current_ms = v->interval_ms = interval_ms;
        m_values.insert(value_pair(oscval, v));
    }
    else {
        it->second->current_ms = it->second->interval_ms = interval_ms;
    }
}

void ValueTimer::removeValue(OscValue* oscval)
{
    value_iter it;
    it = m_values.find(oscval);
    if (it!=m_values.end()) {
        delete it->second;
        m_values.erase(it);
    }
}
    
void ValueTimer::onTimer(int interval_ms)
{
    value_iter it;
    for (it=m_values.begin(); it!=m_values.end(); it++)
    {
        struct ValueTimerPair *v;
        v = it->second;
        v->current_ms -= interval_ms;
        if (v->current_ms <= 0) {
            v->current_ms = v->interval_ms;
            it->first->send();
        }
    }
}
