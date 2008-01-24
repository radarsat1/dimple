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

#include <lo/lo.h>

#include "OscBase.h"
#include "dimple.h"
#include "Simulation.h"

//! OscBase objects always have a name. Class name defaults to "".
OscBase::OscBase(const char *name, OscBase *parent, lo_server server)
    : m_name(name), m_parent(parent), m_server(server?server:(parent?parent->m_server:NULL))
{
#ifdef DEBUG
    m_bTrace = false;
#endif
    if (!m_server)
        throw "Object created without valid lo_server.";
}

//! Add a handler for some OSC method
void OscBase::addHandler(const char *methodname, const char* type, lo_method_handler h)
{
    // build OSC method name
    std::string n("/" + m_name);
    OscBase *p = m_parent;
    while (p && p->name().length()>0) {
        n = "/" + p->name() + n;
        p = p->m_parent;
    }
    if (methodname && strlen(methodname)>0)
        n = n + "/" + methodname;

    // add it to liblo server and store it
    if (lo_server_add_method(m_server, n.c_str(), type, h, this))
    {
        method_t m;
        m.name = n;
        m.type = type;
        m_methods.push_back(m);
    }
}

OscBase::~OscBase()
{
    // remove all stored OSC methods from the liblo server
    while (m_methods.size()>0) {
        method_t m = m_methods.back();
        m_methods.pop_back();
        lo_server_del_method(m_server, m.name.c_str(), m.type.c_str());
    }
}

Simulation *OscBase::simulation()
{
    OscBase *p = parent();
    while (p) {
        Simulation *s = dynamic_cast<Simulation*>(p);
        if (s) return s;
        p = p->parent();
    }
    printf("Internal error, object %s not in a simulation.\n", c_path());
    exit(1);
    return NULL;
}

// ----------------------------------------------------------------------------------

