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

//! OscBase objects always have a name. Class name defaults to "".
OscBase::OscBase(const char *name, OscBase *parent, lo_server server)
    : m_name(name), m_parent(parent), m_server(server?server:parent->m_server)
{
}

//! Add a handler for some OSC method
void OscBase::addHandler(const char *methodname, const char* type, lo_method_handler h)
{
    // build OSC method name
    std::string n("/");
    if (m_parent && m_parent->name().length()>0)
        n += m_parent->name() + "/";
    n += m_name;
    if (strlen(methodname)>0)
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

// ----------------------------------------------------------------------------------

