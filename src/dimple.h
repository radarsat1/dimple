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
#endif
#define _WINSOCK2API_
extern "C" {
#include "lo/lo.h"
}
#include "OscObject.h"

// LibLo server
extern lo_server_thread loserver;
extern lo_address address_send;

// world objects
extern std::map<std::string,OscObject*> world_objects;
typedef std::map<std::string,OscObject*>::iterator objects_iter;

// world constraints
extern std::map<std::string,OscConstraint*> world_constraints;
typedef std::map<std::string,OscConstraint*>::iterator constraints_iter;

OscObject *findObject(const char *name);

extern cWorld* world;
extern dWorldID ode_world;
extern dSpaceID ode_space;
extern OscObject *proxyObject;

// Locking must be placed around OSC- and graphics-thread
// functions that modify the Chai or ODE worlds. This means
// any 'new' or 'delete' operations on ODE objects as well.

// Note: This is a quick non-blocking locking mechanism,
//       it should probably be replaced with something
//       more sophisticated (TODO). It is based on the idea
//       that modifying an 'int' is an atomic operation.

extern int lock_ode;
extern int lock_chai;
//#define LOCK_ODE() (lock_ode++)
//#define UNLOCK_ODE() (lock_ode--)
#define LOCK_ODE() 
#define UNLOCK_ODE() 
#define ODE_LOCKED() (lock_ode>0)
//#define LOCK_CHAI() (lock_chai++)
//#define UNLOCK_CHAI() (lock_chai--)
#define LOCK_CHAI()
#define UNLOCK_CHAI()
#define CHAI_LOCKED() (lock_chai>0)
//#define LOCK_WORLD() (lock_chai++ + lock_ode++)
//#define UNLOCK_WORLD() (lock_chai-- + lock_ode--)
#define LOCK_WORLD()
#define UNLOCK_WORLD()
#define WORLD_LOCKED() (lock_ode>0 || lock_chai>0)
#define WAIT_WORLD_LOCK() {while (WORLD_LOCKED()) Sleep(1);}

// Request structure
class request {
  public:
    request() { handled = false; }
    bool handled;
};

// Request and callback structures for each queue
typedef void ode_callback(void *self);
class ode_request_class : public request {
  public:
    ode_request_class() : request() {}
    ode_callback *callback;
    cODEPrimitive *ob;
};

typedef void chai_callback(void *self);
class chai_request_class : public request {
  public:
    chai_request_class() : request() {}
    chai_callback *callback;
    cGenericObject *ob;
};

// called from OSC thread, non-blocking
ode_request_class*  post_ode_request(ode_callback *callback, cODEPrimitive *ob);
chai_request_class* post_chai_request(chai_callback *callback, cGenericObject *ob);

// called from OSC thread, blocking
void wait_ode_request(ode_callback *callback, cODEPrimitive *ob);
void wait_chai_request(chai_callback *callback, cGenericObject *ob);

// called from respective ODE or CHAI thread
// these will not block.
// return non-zero if requests remain in the queue
int poll_ode_requests();
int poll_chai_requests();

// ODE simulation step count
extern int ode_counter;

// Program thread identifiers
typedef enum {
    DIMPLE_THREAD_MAIN,
    DIMPLE_THREAD_CONTROL,
    DIMPLE_THREAD_PHYSICS,
    DIMPLE_THREAD_HAPTICS,
    DIMPLE_THREAD_GRAPHICS
} dimple_thread_t;

class handler_data
{
public:
    lo_method_handler handler;
    std::string path;
    std::string types;
    lo_arg **argv;
    int argc;
    void *user_data;
    dimple_thread_t thread;
    handler_data(lo_method_handler _handler,
				 const char *_path, const char *_types,
				 lo_arg **_argv, int _argc, void *_user_data,
				 dimple_thread_t _thread);
    ~handler_data();
};


#endif // _OSC_CHAI_GLUT_H_
