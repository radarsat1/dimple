
#ifndef _OSC_CHAI_GLUT_H_
#define _OSC_CHAI_GLUT_H_

extern "C" {
#include "lo/lo.h"
}
#include "OscObject.h"


// LibLo server
extern lo_server_thread loserver;

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

#endif // _OSC_CHAI_GLUT_H_
