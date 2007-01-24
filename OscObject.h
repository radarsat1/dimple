
#ifndef _OSC_OBJECT_H_
#define _OSC_OBJECT_H_

#include <lo/lo.h>
#include <string>
#include "CODEPrism.h"
#include "CODESphere.h"
#include <vector>

//! The OscBase class handles basic OSC functions for dealing with LibLo.
//! It keeps a record of the object's name and classname which becomes
//! part of all OSC methods for this object.
class OscBase
{
public:
    OscBase(const char *name, const char *classname);
    virtual ~OscBase();

protected:
    virtual void addHandler(const char *methodname, const char* type, lo_method_handler h);
    std::string m_name;
    std::string m_classname;

    struct method_t {
        std::string name;
        std::string type;
    };
    std::vector <method_t> m_methods;

    static int destroy_handler(const char *path, const char *types, lo_arg **argv,
                               int argc, void *data, void *user_data);
};

//! The OscObject class keeps track of an object in the world. The object
//! is some cGenericObject and some cODEPrimitve -- in other words, an
//! OscObject consists of an object in the CHAI world and an object in the
//! ODE world which are kept synchronized.
class OscObject : public OscBase
{
  public:
	OscObject(cGenericObject* p, const char *name);
    virtual ~OscObject();

	virtual cODEPrimitive*  odePrimitive() { return dynamic_cast<cODEPrimitive*>(m_objChai); }
	virtual cGenericObject* chaiObject()   { return m_objChai; }

  protected:
	cGenericObject* m_objChai;
};

//! The OscConstraint class keeps track of ODE constraints between two
//! objects in the world, or between one object and some point in the
//! coordinate space.
class OscConstraint : public OscBase
{
public:
    OscConstraint(const char *name, OscObject *object1, OscObject *object2);
    ~OscConstraint() {}

    OscObject *object1() { return m_object1; }
    OscObject *object2() { return m_object2; }

  protected:
      OscObject *m_object1;
      OscObject *m_object2;
};

class OscPrism : public OscObject
{
  public:
	OscPrism(cGenericObject* p, const char *name);
    virtual ~OscPrism();

	virtual cODEPrism* odePrimitive() { return dynamic_cast<cODEPrism*>(m_objChai); }
	virtual cMesh*     chaiObject()   { return dynamic_cast<cMesh*>(m_objChai); }

  protected:
    static int size_handler(const char *path, const char *types, lo_arg **argv,
                            int argc, void *data, void *user_data);
};

class OscSphere : public OscObject
{
  public:
	OscSphere(cGenericObject* p, const char *name);
    virtual ~OscSphere();

	virtual cODESphere*   odePrimitive() { return dynamic_cast<cODESphere*>(m_objChai); }
	virtual cShapeSphere* chaiObject()   { return dynamic_cast<cShapeSphere*>(m_objChai); }

  protected:
    static int radius_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);
};

#endif // _OSC_OBJECT_H_
