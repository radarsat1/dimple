// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-
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

//! This class is used to override behaviour of OscObject's values
//! that can be generalized across different types of objets.  We
//! "assign a specialization" instead of using multiple inheritance.
class OscObjectSpecial {public:virtual ~OscObjectSpecial(){}};

//! The OscObject class keeps track of an object in the world. The object
//! is some cGenericObject and some cODEPrimitve -- in other words, an
//! OscObject consists of an object in the CHAI world and an object in the
//! ODE world which are kept synchronized.
class OscObject : public OscBase
{
  public:
	OscObject(cGenericObject* p, const char *name, OscBase *parent=NULL);
    virtual ~OscObject();

    bool collidedWith(OscObject *o, int count);

    const OscVector3& getPosition() { return m_position; }
    const OscVector3& getVelocity() { return m_velocity; }
    const OscVector3& getAccel() { return m_accel; }

    OSCVECTOR3(OscObject, position) {};
    OSCMATRIX3(OscObject, rotation) {};
    OSCVECTOR3(OscObject, velocity) {};
    OSCVECTOR3(OscObject, accel) {};
    OSCVECTOR3(OscObject, force) {};
    OSCVECTOR3(OscObject, color) {};
    OSCSCALAR(OscObject, mass) {};
    OSCSCALAR(OscObject, density) {};
    OSCSCALAR(OscObject, collide) {};
    OSCSCALAR(OscObject, friction_static) {};
    OSCSCALAR(OscObject, friction_dynamic) {};
    OSCSCALAR(OscObject, stiffness) {};
    OSCSTRING(OscObject, texture_image) {};
    OSCSCALAR(OscObject, texture_level) {};

    OSCMETHOD0(OscObject, grab) {};
    OSCMETHOD0(OscObject, destroy);

    OSCBOOLEAN(OscObject, visible) {};

    std::list<OscConstraint*> m_constraintList;
    typedef std::list<OscConstraint*>::iterator
      constraint_list_iterator;

    OscObjectSpecial *special() { return m_pSpecial; }

  protected:
    /* This is used for any specialized behaviours defined for
     * OscValue members. See OscObjectSpecial for more information. */
    OscObjectSpecial *m_pSpecial;

    std::map<OscObject*,int> m_collisions;

    static void setVelocity(OscObject *me, const OscVector3& vel);

    static int mass_handler(const char *path, const char *types, lo_arg **argv,
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

  protected:
    OSCVECTOR3(OscPrism, size) {};
};

class OscSphere : public OscObject
{
  public:
	OscSphere(cGenericObject* p, const char *name, OscBase *parent=NULL);

    const OscScalar& getRadius();

  protected:
    OSCSCALAR(OscSphere, radius) {};

    static int radius_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);
};

class OscMesh : public OscObject
{
  public:
	OscMesh(cGenericObject *p, const char *name,
            const char *filename, OscBase *parent=NULL);

  protected:
    OSCVECTOR3(OscMesh, size) {};

    static int size_handler(const char *path, const char *types, lo_arg **argv,
							int argc, void *data, void *user_data);
	static void size_physics_callback(void *self);
	cVector3d m_vLastScaled;
};

class OscCamera : public OscBase
{
  public:
	OscCamera(const char *name, OscBase *parent=NULL);

    OscVector3& getPosition() { return m_position; }
    OscVector3& getLookat() { return m_lookat; }
    OscVector3& getUp() { return m_up; }

  protected:
    OSCVECTOR3(OscCamera, position) {};
    OSCVECTOR3(OscCamera, lookat) {};
    OSCVECTOR3(OscCamera, up) {};
};

//! The OscResponse class is used by the free axes of each OscConstraint
//! subclass to dictate how they will respond to external forces.
class OscResponse : public OscBase
{
public:
    OscResponse(const char* name, OscBase *parent);

    double response(double position, double velocity);

    OSCSCALAR(OscResponse, stiffness) {};
    OSCSCALAR(OscResponse, damping) {};
    OSCSCALAR(OscResponse, offset) {};

    OSCMETHOD2(OscResponse, spring)
        { m_stiffness.setValue(arg1); m_damping.setValue(arg2); }
};

//! This class is used to override behaviour of OscConstraint's values
//! that can be generalized across different types of objets.  We
//! "assign a specialization" instead of using multiple inheritance.
class OscConstraintSpecial {public:virtual ~OscConstraintSpecial(){}};

//! The OscConstraint class keeps track of ODE constraints between two
//! objects in the world, or between one object and some point in the
//! coordinate space.
class OscConstraint : public OscBase
{
public:
    OscConstraint(const char *name, OscBase *parent, OscObject *object1, OscObject *object2);
    virtual ~OscConstraint() {};

    OscObject *object1() { return m_object1; }
    OscObject *object2() { return m_object2; }

    //! This function is called once per simulation step, allowing the
    //! constraint to be "motorized" according to some response.
    virtual void simulationCallback() {};

    OSCMETHOD0(OscConstraint, destroy);

    OscConstraintSpecial *special() { return m_pSpecial; }

protected:
    /* This is used for any specialized behaviours defined for
     * OscValue members. See OscConstraintSpecial for more information. */
    OscConstraintSpecial *m_pSpecial;

      OscObject *m_object1;
      OscObject *m_object2;

      // hooke's law response coefficients
      double m_stiffness;
      double m_damping;

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
    OscBallJoint(const char *name, OscBase *parent,
                 OscObject *object1, OscObject *object2,
                 double x, double y, double z);
};

class OscHinge : public OscConstraint
{
public:
    OscHinge(const char *name, OscBase *parent, OscObject *object1, OscObject *object2,
             double x, double y, double z, double ax, double ay, double az);

    OscResponse* m_response;

    virtual void simulationCallback() {};

protected:
    OSCSCALAR(OscHinge, torque) {};
    OSCSCALAR(OscHinge, angle) {};
};

class OscHinge2 : public OscConstraint
{
public:
    OscHinge2(const char *name, OscBase *parent,
              OscObject *object1, OscObject *object2,
              double x, double y, double z,
              double a1x, double a1y, double a1z,
              double a2x, double a2y, double a2z);

    OscResponse* m_response;

protected:
    OSCSCALAR(OscHinge2, torque1) {};
    OSCSCALAR(OscHinge2, torque2) {};
    OSCSCALAR(OscHinge2, angle1) {};
    OSCSCALAR(OscHinge2, angle2) {};
};

class OscUniversal : public OscConstraint
{
public:
    OscUniversal(const char *name, OscBase *parent,
                 OscObject *object1, OscObject *object2,
                 double x, double y, double z,
                 double a1x, double a1y, double a1z,
                 double a2x, double a2y, double a2z);

    OscResponse* m_response;

protected:
    OSCSCALAR(OscUniversal, torque1) {};
    OSCSCALAR(OscUniversal, torque2) {};
    OSCSCALAR(OscUniversal, angle1) {};
    OSCSCALAR(OscUniversal, angle2) {};
};

class OscFixed : public OscConstraint
{
public:
    OscFixed(const char *name, OscBase *parent, OscObject *object1, OscObject *object2)
        : OscConstraint(name, parent, object1, object2) {}
};

/*! The "free" constraint has all axes free, and is used purely to add
 *  force and torque responses between two objects.  Can be used to
 *  put a spring between two objects for example. */
class OscFree : public OscConstraint
{
public:
    OscFree(const char *name, OscBase *parent,
            OscObject *object1, OscObject *object2);

    OscResponse* m_response;

protected:
    OSCVECTOR3(OscFree, force) {};
    OSCVECTOR3(OscFree, torque) {};
};

class OscSlide : public OscConstraint
{
public:
    OscSlide(const char *name, OscBase *parent,
             OscObject *object1, OscObject *object2,
             double ax, double ay, double az);

    OscResponse* m_response;

    virtual void simulationCallback() {};

protected:
    OSCSCALAR(OscSlide, force) {};
    OSCSCALAR(OscSlide, position) {};
};

class OscPiston : public OscConstraint
{
public:
    OscPiston(const char *name, OscBase *parent, OscObject *object1,
              OscObject *object2, double x, double y, double z,
              double ax, double ay, double az);

    OscResponse* m_response;

protected:
    OSCSCALAR(OscPiston, force) {};
    OSCSCALAR(OscPiston, position) {};

    // TODO
    // OSCSCALAR(OscPiston, torque) {};
    // OSCSCALAR(OscPiston, angle) {};
};

#endif // _OSC_OBJECT_H_

