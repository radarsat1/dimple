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

#ifndef _OSC_BASE_H_
#define _OSC_BASE_H_

#include <lo/lo.h>
#include <string>
#include <vector>
#include <map>

//! The OscBase class handles basic OSC functions for dealing with LibLo.
//! It keeps a record of the object's name and classname which becomes
//! part of all OSC methods for this object.
class OscBase
{
public:
    OscBase(const char *name, const char *classname);
    virtual ~OscBase();

    const char* name() { return m_name.c_str(); }
    const char* classname() { return m_classname.c_str(); }
    const std::string strname() { return m_name; }
    const std::string strclassname() { return m_classname; }

protected:
    virtual void addHandler(const char *methodname, const char* type, lo_method_handler h);
    std::string m_name;
    std::string m_classname;

    struct method_t {
        std::string name;
        std::string type;
    };
    std::vector <method_t> m_methods;
};

#endif // _OSC_BASE_H_
