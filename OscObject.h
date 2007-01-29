
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

    const char* name() { return m_name.c_str(); }

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
	virtual cGenericObject* chaiObject()   { return dynamic_cast<cGenericObject*>(m_objChai); }

	void linkConstraint(std::string &name);
	void unlinkConstraint(std::string &name);

  protected:
	cGenericObject* m_objChai;
	std::vector<std::string> m_constraintLinks;

    static int destroy_handler(const char *path, const char *types, lo_arg **argv,
                               int argc, void *data, void *user_data);
    static int mass_handler(const char *path, const char *types, lo_arg **argv,
                            int argc, void *data, void *user_data);
    static int force_handler(const char *path, const char *types, lo_arg **argv,
                             int argc, void *data, void *user_data);
};

class OscComposite : public OscObject
{
  public:
    OscComposite(const char *name);

    void addChild(OscObject *o);
    
  protected:
    std::vector<OscObject*> m_children;
    dBodyID m_odeBody;
};

class OscPrism : public OscObject
{
  public:
	OscPrism(cGenericObject* p, const char *name);

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

	virtual cODESphere*   odePrimitive() { return dynamic_cast<cODESphere*>(m_objChai); }
	virtual cShapeSphere* chaiObject()   { return dynamic_cast<cShapeSphere*>(m_objChai); }

  protected:
    static int radius_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);
};

//! The OscConstraint class keeps track of ODE constraints between two
//! objects in the world, or between one object and some point in the
//! coordinate space.
class OscConstraint : public OscBase
{
public:
    OscConstraint(const char *name, OscObject *object1, OscObject *object2);
    ~OscConstraint();

    OscObject *object1() { return m_object1; }
    OscObject *object2() { return m_object2; }

    //! This function is called once per simulation step, allowing the
    //! constraint to be "motorized" according to some response.
    virtual void simulationCallback() {};

  protected:
      OscObject *m_object1;
      OscObject *m_object2;

      // hooke's law response coefficients
      double m_stiffness;
      double m_damping;

	  static int destroy_handler(const char *path, const char *types, lo_arg **argv,
								 int argc, void *data, void *user_data);

	  static int responseLinear_handler(const char *path, const char *types, lo_arg **argv,
								        int argc, void *data, void *user_data);

      static int responseSpring_handler(const char *path, const char *types, lo_arg **argv,
								        int argc, void *data, void *user_data);
};

class OscBallJoint : public OscConstraint
{
public:
    OscBallJoint(const char *name, OscObject *object1, OscObject *object2,
                 double x, double y, double z);
};

class OscHinge : public OscConstraint
{
public:
    OscHinge(const char *name, OscObject *object1, OscObject *object2,
             double x, double y, double z, double ax, double ay, double az);

    virtual void simulationCallback();
};

class OscHinge2 : public OscConstraint
{
public:
    OscHinge2(const char *name, OscObject *object1, OscObject *object2,
              double x, double y, double z,
              double ax, double ay, double az,
              double bx, double by, double bz);

    virtual void simulationCallback();
};

class OscUniversal : public OscConstraint
{
public:
    OscUniversal(const char *name, OscObject *object1, OscObject *object2,
                 double x, double y, double z,
                 double ax, double ay, double az,
                 double bx, double by, double bz);

    virtual void simulationCallback();
};

class OscFixed : public OscConstraint
{
public:
    OscFixed(const char *name, OscObject *object1, OscObject *object2);
};

#endif // _OSC_OBJECT_H_
