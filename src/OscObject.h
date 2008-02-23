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

#ifndef _OSC_OBJECT_H_
#define _OSC_OBJECT_H_

#include <lo/lo.h>
#include <string>
#include <vector>
#include <map>

#include "OscBase.h"
#include "Simulation.h"
#include "CODEPrism.h"
#include "CODESphere.h"

// Macro for conditionally including code when compiling for DEBUG
// (TODO: move this to dimple.h or somewhere else more general.)
#ifdef DEBUG
#define ptrace(c,x) if (c) {printf x;}
#else
#define ptrace(c,x)
#endif

// Macros for easily adding member functions with associated callbacks
#define __dimple_str(x) #x
#define _dimple_str(x) #x
#define OSCVALUE(c, t, x, p) t m_##x;           \
    static void set_##x(void *data, t& s) \
    { p; ((c*)data)->on_##x(); }                \
    virtual void on_##x()
#define OSCSCALAR(c, o) OSCVALUE(c, OscScalar, o, ptrace(((c*)data)->m_bTrace, ("[%s] %s." _dimple_str(o) " -> %f\n", ((c*)data)->simulation()->type_str(), ((c*)data)->c_path(), s.m_value)))
#define OSCVECTOR3(c, o) OSCVALUE(c, OscVector3, o, ptrace(((c*)data)->m_bTrace, ("[%s] %s." _dimple_str(o) " -> (%f, %f, %f)\n", ((c*)data)->simulation()->type_str(), ((c*)data)->c_path(), s.x, s.y, s.z)))
#define OSCSTRING(c, o) OSCVALUE(c, OscStrings, o, ptrace(((c*)data)->m_bTrace, ("[%s] %s." _dimple_str(o) " -> '%s'\n", ((c*)data)->simulation()->type_str(), ((c*)data)->c_path(), s.c_str())))

//! The OscValue class is the base class for all OSC-accessible values,
//! including vectors and scalars.
class OscValue : public OscBase
{
  public:
    OscValue(const char *name, OscBase *owner);
    virtual ~OscValue();
    virtual void send() = 0;

    typedef void SetCallback(void*, OscValue&);
    void setSetCallback(SetCallback*c, void*d, int thread)
      { m_set_callback = c; m_set_callback_data = d; m_set_callback_thread = thread; }
    typedef void GetCallback(void*, OscValue&, int interval);
    void setGetCallback(GetCallback*c, void*d, int thread)
      { m_get_callback = c; m_get_callback_data = d; m_get_callback_thread = thread; }

  protected:
    SetCallback *m_set_callback;
    void *m_set_callback_data;
    int m_set_callback_thread;
    GetCallback *m_get_callback;
    void *m_get_callback_data;
    int m_get_callback_thread;
    static int get_handler(const char *path, const char *types, lo_arg **argv,
                           int argc, void *data, void *user_data);
};

//! The OscScalar class is used to maintain information about scalar values
//! used throughout the OSC interface.
class OscScalar : public OscValue
{
  public:
    OscScalar(const char *name, OscBase *owner);
	void set(double value);
    void send();

    double m_value;

    typedef void SetCallback(void*, OscScalar&);
    void setSetCallback(SetCallback *c, void *d, int thread)
        { OscValue::setSetCallback((OscValue::SetCallback*)c, d, thread); }
    typedef void GetCallback(void*, OscScalar&, int interval);
    void setGetCallback(GetCallback *c, void *d, int thread)
        { OscValue::setGetCallback((OscValue::GetCallback*)c, d, thread); }

  protected:
    static int _handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data);
};

//! The OscVector3 class is used to maintain information about 3-vector values
//! used throughout the OSC interface.
class OscVector3 : public OscValue, public cVector3d
{
  public:
    OscVector3(const char *name, OscBase *owner);
	void setChanged();
	void set(double x, double y, double z);
    void send();

	OscScalar m_magnitude;

    typedef void SetCallback(void*, OscVector3&);
    void setSetCallback(SetCallback *c, void *d, int thread)
        { OscValue::setSetCallback((OscValue::SetCallback*)c, d, thread); }
    typedef void GetCallback(void*, OscVector3&, int interval);
    void setGetCallback(GetCallback *c, void *d, int thread)
        { OscValue::setGetCallback((OscValue::GetCallback*)c, d, thread); }

  protected:
    static void set_magnitude_callback(OscVector3*, OscScalar&);
    static int _handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data);
};

class OscString : public OscValue, public std::string
{
  public:
    OscString(const char *name, OscBase *owner);
    void send();

    void set(const std::string& s);
    void set(const char* s);

    typedef void SetCallback(void*, OscString&);
    void setSetCallback(SetCallback *c, void *d, int thread)
        { OscValue::setSetCallback((OscValue::SetCallback*)c, d, thread); }
    typedef void GetCallback(void*, OscString&, int interval);
    void setGetCallback(GetCallback *c, void *d, int thread)
        { OscValue::setGetCallback((OscValue::GetCallback*)c, d, thread); }

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
	OscObject(cGenericObject* p, const char *name, OscBase *parent=NULL);
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

    OSCVECTOR3(OscObject, position);

  protected:
	cGenericObject* m_objChai;
	std::vector<std::string> m_constraintLinks;
    std::map<OscObject*,int> m_collisions;

    bool m_getCollide;

    OscVector3 m_velocity;
    OscVector3 m_accel;

    OSCVECTOR3(OscObject, force) {};
    OSCVECTOR3(OscObject, color) {};

    static void setVelocity(OscObject *me, const OscVector3& vel);

    OscScalar m_friction_static;
    OscScalar m_friction_dynamic;
    static void setFrictionStatic(OscObject *me, const OscScalar& value);
    static void setFrictionDynamic(OscObject *me, const OscScalar& value);

    OscString m_texture_image;
    static void setTextureImage(OscObject *me, const OscString& filename);

    static int destroy_handler(const char *path, const char *types, lo_arg **argv,
                               int argc, void *data, void *user_data);
    static int mass_handler(const char *path, const char *types, lo_arg **argv,
                            int argc, void *data, void *user_data);
    static int collideGet_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data);
    static int grab_handler(const char *path, const char *types, lo_arg **argv,
                            int argc, void *data, void *user_data);
    static int oscillate_handler(const char *path, const char *types, lo_arg **argv,
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
	OscPrism(cGenericObject* p, const char *name, OscBase *parent=NULL);

	virtual cODEPrism* odePrimitive() { return dynamic_cast<cODEPrism*>(m_objChai); }
	virtual cMesh*     chaiObject()   { return dynamic_cast<cMesh*>(m_objChai); }

  protected:
    OSCVECTOR3(OscPrism, size) {};
};

class OscSphere : public OscObject
{
  public:
	OscSphere(cGenericObject* p, const char *name, OscBase *parent=NULL);

	virtual cODESphere*   odePrimitive() { return dynamic_cast<cODESphere*>(m_objChai); }
	virtual cShapeSphere* chaiObject()   { return dynamic_cast<cShapeSphere*>(m_objChai); }

    const OscScalar& getRadius();

  protected:
    OSCSCALAR(OscSphere, radius) {};
    /*
    OscScalar m_radius;
    static void setRadius(void *data, const OscScalar&) {
    printf ("OscSphere::setRadius()\n");
    OscSphere *me = (OscSphere*)data;
    me->onSetRadius();
    }
    virtual void onSetRadius() {}
    */

    static int radius_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);
};

class OscMesh : public OscObject
{
  public:
    OscMesh(cGenericObject* p, const char *name);

	virtual cODEMesh* odePrimitive() { return dynamic_cast<cODEMesh*>(m_objChai); }
	virtual cMesh*    chaiObject()   { return dynamic_cast<cMesh*>(m_objChai); }

  protected:
    static int size_handler(const char *path, const char *types, lo_arg **argv,
							int argc, void *data, void *user_data);
	static void size_physics_callback(void *self);
	cVector3d m_vLastScaled;
};

//! The OscConstraint class keeps track of ODE constraints between two
//! objects in the world, or between one object and some point in the
//! coordinate space.
class OscConstraint : public OscBase
{
public:
    OscConstraint(const char *name, OscBase *parent, OscObject *object1, OscObject *object2);
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

      static int responseCenter_handler(const char *path, const char *types, lo_arg **argv,
                                        int argc, void *data, void *user_data);

      static int responseConstant_handler(const char *path, const char *types, lo_arg **argv,
                                          int argc, void *data, void *user_data);

	  static int responseLinear_handler(const char *path, const char *types, lo_arg **argv,
								        int argc, void *data, void *user_data);

      static int responseSpring_handler(const char *path, const char *types, lo_arg **argv,
								        int argc, void *data, void *user_data);

      static int responseWall_handler(const char *path, const char *types, lo_arg **argv,
                                      int argc, void *data, void *user_data);

      static int responsePluck_handler(const char *path, const char *types, lo_arg **argv,
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
    OscHinge(const char *name, OscBase *parent, OscObject *object1, OscObject *object2,
             double x, double y, double z, double ax, double ay, double az);

    virtual void simulationCallback();

protected:
    OscScalar m_torque;
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

