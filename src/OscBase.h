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

/* === Macros for easily adding member functions with associated callbacks */

#define OSCVALUE(c, t, x, p) t m_##x;           \
    static void set_##x(void *data, t& s) \
    { p; ((c*)data)->on_##x(); }                \
    virtual void on_##x()
#define OSCSCALAR(c, o) OSCVALUE(c, OscScalar, o, ptrace(((c*)data)->m_bTrace, ("[%s] %s." _dimple_str(o) " -> %f\n", ((c*)data)->simulation()->type_str(), ((c*)data)->c_path(), s.m_value)))
#define OSCVECTOR3(c, o) OSCVALUE(c, OscVector3, o, ptrace(((c*)data)->m_bTrace, ("[%s] %s." _dimple_str(o) " -> (%f, %f, %f)\n", ((c*)data)->simulation()->type_str(), ((c*)data)->c_path(), s.x, s.y, s.z)))
#define OSCMATRIX3(c, o) OSCVALUE(c, OscMatrix3, o, ptrace(((c*)data)->m_bTrace, ("[%s] %s." _dimple_str(o) " -> (%f, %f, %f; %f, %f, %f; %f, %f, %f)\n", ((c*)data)->simulation()->type_str(), ((c*)data)->c_path(), s.m[0][0], s.m[0][1], s.m[0][2], s.m[1][0], s.m[1][1], s.m[1][2], s.m[2][0], s.m[2][1], s.m[2][2])))
#define OSCSTRING(c, o) OSCVALUE(c, OscStrings, o, ptrace(((c*)data)->m_bTrace, ("[%s] %s." _dimple_str(o) " -> '%s'\n", ((c*)data)->simulation()->type_str(), ((c*)data)->c_path(), s.c_str())))
#define OSCMETHOD0(t, x)                                                \
    static int x##_handler(const char *path, const char *types,         \
                           lo_arg **argv, int argc, void *data,         \
                           void *user_data) {((t*)user_data)->on_##x();}\
    virtual void on_##x()

/* === End of macro definitions. */


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

    lo_server get_lo_server();
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

protected:
    virtual void addHandler(const char *methodname, const char* type, lo_method_handler h);
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
