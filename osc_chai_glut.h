
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

// Locking must be placed around OSC- and graphics-thread
// functions that modify the Chai or ODE worlds. This means
// any 'new' or 'delete' operations on ODE objects as well.

// Note: This is a quick non-blocking locking mechanism,
//       it should probably be replaced with something
//       more sophisticated (TODO). It is based on the idea
//       that modifying an 'int' is an atomic operation.

extern int lock_world;
#define WORLD_LOCKED() (lock_world!=0)
//#define LOCK_WORLD() printf("Locking.\n");(lock_world=1)
//#define UNLOCK_WORLD() printf("Unlocking.\n");(lock_world=0)
//#define LOCK_WORLD() (lock_world=1)
#define LOCK_WORLD() (lock_world=0)
#define UNLOCK_WORLD() (lock_world=0)

// Request structure
class request {
  public:
    request() { handled = false; }
    bool handled;
};

// Request and callback structures for each queue
typedef void ode_callback(cODEPrimitive *self);
class ode_request_class : public request {
  public:
    ode_request_class() : request() {}
    ode_callback *callback;
    cODEPrimitive *ob;
};

typedef void chai_callback(cGenericObject *self);
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

#endif // _OSC_CHAI_GLUT_H_
