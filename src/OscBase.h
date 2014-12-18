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

#ifndef _OSC_BASE_H_
#define _OSC_BASE_H_

#include <lo/lo.h>
#include <string>
#include <vector>
#include <map>

class Simulation;

//! The OscBase class handles basic OSC functions for dealing with LibLo.
//! It keeps a record of the object's name and classname which becomes
//! part of all OSC methods for this object.
class OscBase
{
public:
    OscBase(const char *name, OscBase *parent, lo_server server=NULL);
    virtual ~OscBase();

    const char* c_name() const { return m_name.c_str(); }
    const std::string name() const { return m_name; }

    const char* c_path() { return path().c_str(); }
    const std::string path() { if (m_path.empty()) m_path=(m_parent?m_parent->path():std::string())+"/"+name(); return m_path; }

    OscBase *parent() { return m_parent; }

    Simulation *simulation();

#ifdef DEBUG
    void traceOn() { m_bTrace = true; }
    void traceOff() { m_bTrace = false; }
    bool tracing() { return m_bTrace; }
#else
    void traceOn() {}
    void traceOff() {}
    bool tracing() { return false; }
#endif

    //! Add an OSC handler to this class.
    //!
    //! This should be protected, but since C++ doesn't allow friend
    //! class inheritence, there is no other way to allow
    //! specialization classes to access this method.
    virtual void addHandler(const char *methodname, const char* type, lo_method_handler h);

protected:
    std::string m_name;
    std::string m_path; // generated on demand, but we cache it here
    OscBase *m_parent;
    lo_server m_server;

    /*! True if this object should output trace messages when compiled
     *  for debug. */
#ifdef DEBUG
    bool m_bTrace;
#endif

    struct method_t {
        std::string name;
        std::string type;
    };
    std::vector <method_t> m_methods;
};

#endif // _OSC_BASE_H_
