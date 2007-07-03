
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

#ifndef _OSC_OBJECT_H_
#define _OSC_OBJECT_H_

#include <lo/lo.h>
#include <string>
#include <vector>
#include <map>

#include "CODEPrism.h"
#include "CODESphere.h"

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

//! The OscValue class is the base class for all OSC-accessible values,
//! including vectors and scalars.
class OscValue : public OscBase
{
  public:
    OscValue(const char *name, const char *classname);
    virtual ~OscValue();
	virtual void setChanged() {}
    virtual void send() = 0;

    typedef void set_callback(void*, const OscValue&);
    void setCallback(set_callback*c, void*d, int thread)
      { m_callback = c; m_callback_data = d; m_callback_thread = thread; }

  protected:
    set_callback *m_callback;
    void *m_callback_data;
    int m_callback_thread;
    static int get_handler(const char *path, const char *types, lo_arg **argv,
                           int argc, void *data, void *user_data);
};

//! The OscScalar class is used to maintain information about scalar values
//! used throughout the OSC interface.
class OscScalar : public OscValue
{
  public:
    OscScalar(const char *name, const char *classname);
	void set(double value);
    void send();

    double m_value;

  protected:
    static int _handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data);
};

//! The OscVector3 class is used to maintain information about 3-vector values
//! used throughout the OSC interface.
class OscVector3 : public OscValue, public cVector3d
{
  public:
    OscVector3(const char *name, const char *classname);
	void setChanged();
	void set(double x, double y, double z);
    void send();

	OscScalar m_magnitude;

  protected:
    static void setMagnitude(OscVector3*, const OscScalar&);
    static int _handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data);
};

class OscString : public OscValue, public std::string
{
  public:
    OscString(const char *name, const char *classname);
    void send();

  protected:
    static int _handler(const char *path, const char *types, lo_arg **argv,
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
	virtual cGenericObject* chaiObject()   { return dynamic_cast<cGenericObject*>(m_objChai); }

	void linkConstraint(std::string &name);
	void unlinkConstraint(std::string &name);

    void updateDynamicVelocity(const dReal* vel);
    void updateDynamicPosition(const dReal* pos);

    bool collidedWith(OscObject *o);

    const OscVector3& getPosition() { return m_position; }
    const OscVector3& getVelocity() { return m_velocity; }
    const OscVector3& getAccel() { return m_accel; }

    void ungrab(int thread);

  protected:
	cGenericObject* m_objChai;
	std::vector<std::string> m_constraintLinks;
    std::map<OscObject*,int> m_collisions;

    bool m_getCollide;

    OscVector3 m_velocity;
    OscVector3 m_accel;
    OscVector3 m_position;
    static void setPosition(OscObject *me, const OscVector3& pos);
    static void setVelocity(OscObject *me, const OscVector3& vel);

    OscScalar m_friction_static;
    OscScalar m_friction_dynamic;
    static void setFrictionStatic(OscObject *me, const OscScalar& value);
    static void setFrictionDynamic(OscObject *me, const OscScalar& value);

    OscVector3 m_color;
    static void setColor(OscObject *me, const OscVector3& color);

    OscString m_texture_image;
    static void setTextureImage(OscObject *me, const OscString& filename);

    static int destroy_handler(const char *path, const char *types, lo_arg **argv,
                               int argc, void *data, void *user_data);
    static int mass_handler(const char *path, const char *types, lo_arg **argv,
                            int argc, void *data, void *user_data);
    static int force_handler(const char *path, const char *types, lo_arg **argv,
                             int argc, void *data, void *user_data);
    static int collideGet_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data);
    static int grab_handler(const char *path, const char *types, lo_arg **argv,
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

    static int forceMagnitudeGet_handler(const char *path, const char *types, lo_arg **argv,
                                         int argc, void *data, void *user_data);

protected:
    float m_torque;
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

