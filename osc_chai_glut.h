
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

#endif // _OSC_CHAI_GLUT_H_
