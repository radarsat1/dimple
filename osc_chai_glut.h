
#ifndef _OSC_CHAI_GLUT_H_
#define _OSC_CHAI_GLUT_H_

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
#define LOCK_WORLD() printf("Locking.\n");(lock_world=1)
#define UNLOCK_WORLD() printf("Unlocking.\n");(lock_world=0)

// Messaging system, operation requests on ODE and CHAI spaces
typedef enum {
    ODE_OBJECT_ADD,
    ODE_OBJECT_REMOVE
} ode_request_t;

typedef enum {
    CHAI_OBJECT_ADD,
    CHAI_OBJECT_REMOVE
} chai_request_t;

// Request structure
class request {
  public:
    request() { handled = false; }
    bool handled;
};

// Request structures for each queue
class ode_request_class : public request {
  public:
    ode_request_class() : request() {}
    ode_request_t req;
    cODEPrimitive *ob;
};

class chai_request_class : public request {
  public:
    chai_request_class() : request() {}
    chai_request_t req;
    cGenericObject *ob;
};

// called from OSC thread, non-blocking
ode_request_class*  post_ode_request(ode_request_t req, cODEPrimitive *ob);
chai_request_class* post_chai_request(chai_request_t req, cGenericObject *ob);

// called from OSC thread
void wait_ode_request(ode_request_t req, cODEPrimitive *ob);
void wait_chai_request(chai_request_t req, cGenericObject *ob);

// called from respective ODE or CHAI thread
// return non-zero if requests remain in the queue
int ode_poll_requests();
int chai_poll_requests();

#endif // _OSC_CHAI_GLUT_H_
