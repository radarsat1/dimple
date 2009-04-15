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

#ifndef _OSC_CHAI_GLUT_H_
#define _OSC_CHAI_GLUT_H_

#ifdef WIN32
#include <winsock2.h>
#ifdef interface
#undef interface
#endif
#endif
#define _WINSOCK2API_
#include "lo/lo.h"
#include "AudioStreamer.h"
#include <map>
#include <CWorld.h>
#include <ode/ode.h>

/** Global options **/

extern int visual_fps;
extern int visual_timestep_ms;
extern int physics_timestep_ms;
extern int haptics_timestep_ms;

/** Miscellaneous macros **/

#ifndef WIN32
#define Sleep(t) usleep(t*1000)
#endif

// Macro for conditionally including code when compiling for DEBUG
#ifdef DEBUG
#define ptrace(c,x) if (c) {printf x;}
#else
#define ptrace(c,x)
#endif

#define __dimple_str(x) #x
#define _dimple_str(x) #x

// LibLo server
extern lo_server        loserver;
extern lo_address       address_send;

#ifdef FLEXT_SYS
// Needed for initialization from Flext object
int dimple_main(int argc, char* argv[]);
// A bit of a hack for the Flext object to catch lo_send()
#define lo_send flext_dimple_send
void flext_dimple_send(lo_address, const char*, const char*, ...);
#endif
void dimple_cleanup();

#endif // _OSC_CHAI_GLUT_H_
