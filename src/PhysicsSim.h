// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-

#ifndef _PHYSICS_SIM_H_
#define _PHYSICS_SIM_H_

#include "Simulation.h"
#include "OscObject.h"
#include <ode/ode.h>

class ODEObject;

class PhysicsSim : public Simulation
{
  public:
    PhysicsSim(const char *port);
    virtual ~PhysicsSim();

    dWorldID odeWorld() { return m_odeWorld; }
    dSpaceID odeSpace() { return m_odeSpace; }
    dJointGroupID odeContactGroup() { return m_odeContactGroup; }

    static const int MAX_CONTACTS;

    virtual void on_gravity()
        { dWorldSetGravity(m_odeWorld, m_gravity.x,
                           m_gravity.y, m_gravity.z); }

    //! Set the grabbed object or ungrab by setting to NULL.
    virtual void set_grabbed(OscObject *pGrabbed);

  protected:
    dWorldID m_odeWorld;
    dSpaceID m_odeSpace;
    dJointGroupID m_odeContactGroup;

    ODEObject* m_pGrabbedODEObject;
    OscObject* m_pCursor;

    bool m_bGetCollide;
    int m_counter;

    virtual void initialize();
    virtual void step();

    static void ode_errorhandler(int errnum, const char *msg, va_list ap)
        { printf("ODE error %d: %s\n", errnum, msg); }
    static void ode_nearCallback (void *data, dGeomID o1, dGeomID o2);
};

class PhysicsPrismFactory : public PrismFactory
{
public:
    PhysicsPrismFactory(Simulation *parent) : PrismFactory(parent) {}
    virtual ~PhysicsPrismFactory() {}

    virtual PhysicsSim* simulation() { return static_cast<PhysicsSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

class PhysicsSphereFactory : public SphereFactory
{
public:
    PhysicsSphereFactory(Simulation *parent) : SphereFactory(parent) {}
    virtual ~PhysicsSphereFactory() {}

    virtual PhysicsSim* simulation() { return static_cast<PhysicsSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

class PhysicsHingeFactory : public HingeFactory
{
public:
    PhysicsHingeFactory(Simulation *parent) : HingeFactory(parent) {}
    virtual ~PhysicsHingeFactory() {}

    virtual PhysicsSim* simulation() { return static_cast<PhysicsSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2,
                double x, double y, double z, double ax, double ay, double az);
};

class PhysicsHinge2Factory : public Hinge2Factory
{
public:
    PhysicsHinge2Factory(Simulation *parent) : Hinge2Factory(parent) {}
    virtual ~PhysicsHinge2Factory() {}

    virtual PhysicsSim* simulation() { return static_cast<PhysicsSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2,
                double x, double y, double z, double a1x, double a1y,
                double a1z, double a2x, double a2y, double a2z);
};

class PhysicsFixedFactory : public FixedFactory
{
public:
    PhysicsFixedFactory(Simulation *parent) : FixedFactory(parent) {}
    virtual ~PhysicsFixedFactory() {}

    virtual PhysicsSim* simulation() { return static_cast<PhysicsSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2);
};

class PhysicsBallJointFactory : public BallJointFactory
{
public:
    PhysicsBallJointFactory(Simulation *parent) : BallJointFactory(parent) {}
    virtual ~PhysicsBallJointFactory() {}

    virtual PhysicsSim* simulation() { return static_cast<PhysicsSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2,
                double x, double y, double z);
};

class PhysicsSlideFactory : public SlideFactory
{
public:
    PhysicsSlideFactory(Simulation *parent) : SlideFactory(parent) {}
    virtual ~PhysicsSlideFactory() {}

    virtual PhysicsSim* simulation() { return static_cast<PhysicsSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2,
                double ax, double ay, double az);
};

class PhysicsPistonFactory : public PistonFactory
{
public:
    PhysicsPistonFactory(Simulation *parent) : PistonFactory(parent) {}
    virtual ~PhysicsPistonFactory() {}

    virtual PhysicsSim* simulation() { return static_cast<PhysicsSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2,
                double x, double y, double z, double ax, double ay, double az);
};

class PhysicsUniversalFactory : public UniversalFactory
{
public:
    PhysicsUniversalFactory(Simulation *parent) : UniversalFactory(parent) {}
    virtual ~PhysicsUniversalFactory() {}

    virtual PhysicsSim* simulation() { return static_cast<PhysicsSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2,
                double x, double y, double z, double a1x, double a1y,
                double a1z, double a2x, double a2y, double a2z);
};

class ODEObject : public OscObjectSpecial
{
public:
    ODEObject(OscObject *obj, dGeomID odeGeom, dWorldID odeWorld, dSpaceID odeSpace);
    virtual ~ODEObject();

    cVector3d getPosition() { return cVector3d(dBodyGetPosition(m_odeBody)); }
    cVector3d getVelocity() { return cVector3d(dBodyGetLinearVel(m_odeBody)); }
    cMatrix3d getRotation() {
      const dReal *r = dBodyGetRotation(m_odeBody); cMatrix3d m;
      m.set(r[0], r[1], r[2], r[4], r[5], r[6], r[8], r[9], r[10]);
      return m; }

    //! Update ODE dynamics information for this object.
    void update();
    
    //! Remove the association between the body and geom.
    void disconnectBody()
        { dGeomSetBody(m_odeGeom, 0); dBodyDisable(m_odeBody); }
    
    //! Create the association between the body and geom.
    void connectBody()
        { dGeomSetBody(m_odeGeom, m_odeBody); dBodyEnable(m_odeBody); }

    dBodyID  body()  { return m_odeBody;  } //! Return the dBodyID
    dGeomID  geom()  { return m_odeGeom;  } //! Return the dGeomID
    dMass&   mass()  { return m_odeMass;  } //! Return the dMass
    dWorldID world() { return m_odeWorld; } //! Return the dWorldID
    dSpaceID space() { return m_odeSpace; } //! Return the dSpaceID

    OscObject *object() { return m_object; }

protected:
    dBodyID  m_odeBody;
    dGeomID  m_odeGeom;
    dMass	 m_odeMass;
    dWorldID m_odeWorld;
    dSpaceID m_odeSpace;

    OscObject *m_object;

    static void on_set_force(void* me, OscVector3 &f);
    static void on_set_position(void* me, OscVector3 &p);
    static void on_set_rotation(void* me, OscMatrix3 &r);
    static void on_set_velocity(void* me, OscVector3 &v);
    static void on_set_accel(void* me, OscVector3 &a);

    static int push_handler(const char *path, const char *types, lo_arg **argv,
                            int argc, void *data, void *user_data);

    friend class ODEConstraint;
};

class ODEConstraint : public OscConstraintSpecial
{
public:
    ODEConstraint(OscConstraint *c, dJointID odeJoint,
                  dWorldID odeWorld, dSpaceID odeSpace,
                  OscObject *object1, OscObject *object2);
    virtual ~ODEConstraint();

    dJointID joint() { return m_odeJoint; }
    OscConstraint *constraint() { return m_constraint; }
    dBodyID body1() { return m_odeBody1; }
    dBodyID body2() { return m_odeBody2; }

protected:
    OscConstraint *m_constraint;
    dWorldID m_odeWorld;
    dSpaceID m_odeSpace;
    dJointID m_odeJoint;
    dBodyID  m_odeBody1;
    dBodyID  m_odeBody2;
};

class OscSphereODE : public OscSphere
{
public:
	OscSphereODE(dWorldID odeWorld, dSpaceID odeSpace, const char *name, OscBase *parent=NULL);
    virtual ~OscSphereODE() {}

protected:
    virtual void on_radius();
    virtual void on_mass();
    virtual void on_density();

    virtual void on_grab()
        { static_cast<PhysicsSim*>(simulation())->set_grabbed(this); }
};

class OscPrismODE : public OscPrism
{
public:
	OscPrismODE(dWorldID odeWorld, dSpaceID odeSpace, const char *name, OscBase *parent=NULL);
    virtual ~OscPrismODE() {}

protected:
    virtual void on_size();
    virtual void on_mass();
    virtual void on_density();

    virtual void on_grab()
        { static_cast<PhysicsSim*>(simulation())->set_grabbed(this); }
};

class OscHingeODE : public OscHinge
{
public:
    OscHingeODE(dWorldID odeWorld, dSpaceID odeSpace,
                const char *name, OscBase *parent, OscObject *object1, OscObject *object2,
                double x, double y, double z, double ax, double ay, double az);
    virtual ~OscHingeODE();

    virtual void simulationCallback();

protected:
    virtual void on_torque()
        { dJointAddHingeTorque(((ODEConstraint*)special())->joint(),
                               m_torque.m_value); }
};

class OscHinge2ODE : public OscHinge2
{
public:
    OscHinge2ODE(dWorldID odeWorld, dSpaceID odeSpace,
                 const char *name, OscBase *parent,
                 OscObject *object1, OscObject *object2,
                 double x, double y, double z,
                 double a1x, double a1y, double a1z,
                 double a2x, double a2y, double a2z);

    virtual ~OscHinge2ODE();

    virtual void simulationCallback();

protected:
    virtual void on_torque1()
        { dJointAddHinge2Torques(
                static_cast<ODEConstraint*>(special())->joint(),
                m_torque1.m_value, m_torque2.m_value); }
    virtual void on_torque2()
        { dJointAddHinge2Torques(
                static_cast<ODEConstraint*>(special())->joint(),
                m_torque1.m_value, m_torque2.m_value); }
};

class OscFixedODE : public OscFixed
{
public:
    OscFixedODE(dWorldID odeWorld, dSpaceID odeSpace,
                const char *name, OscBase *parent, OscObject *object1, OscObject *object2);

    virtual ~OscFixedODE();

protected:
};

class OscBallJointODE : public OscBallJoint
{
public:
    OscBallJointODE(dWorldID odeWorld, dSpaceID odeSpace,
                    const char *name, OscBase *parent,
                    OscObject *object1, OscObject *object2,
                    double x, double y, double z);
};

class OscSlideODE : public OscSlide
{
public:
    OscSlideODE(dWorldID odeWorld, dSpaceID odeSpace,
                const char *name, OscBase *parent,
                OscObject *object1, OscObject *object2,
                double ax, double ay, double az);

    virtual ~OscSlideODE();

    virtual void simulationCallback();

protected:
    virtual void on_force()
        { dJointAddSliderForce(
                static_cast<ODEConstraint*>(special())->joint(),
                m_force.m_value); }
};

class OscPistonODE : public OscPiston
{
public:
    OscPistonODE(dWorldID odeWorld, dSpaceID odeSpace,
                 const char *name, OscBase *parent, OscObject *object1,
                 OscObject *object2, double x, double y, double z,
                 double ax, double ay, double az);

    virtual ~OscPistonODE();

    virtual void simulationCallback();

protected:
    virtual void on_force()
        { dJointAddPistonForce(
                static_cast<ODEConstraint*>(special())->joint(),
                m_force.m_value); }
};

class OscUniversalODE : public OscUniversal
{
public:
    OscUniversalODE(dWorldID odeWorld, dSpaceID odeSpace,
                    const char *name, OscBase *parent,
                    OscObject *object1, OscObject *object2,
                    double x, double y, double z,
                    double a1x, double a1y, double a1z,
                    double a2x, double a2y, double a2z);

    virtual ~OscUniversalODE();

    virtual void simulationCallback();

protected:
    virtual void on_torque1()
        { dJointAddUniversalTorques(
                static_cast<ODEConstraint*>(special())->joint(),
                m_torque1.m_value, m_torque2.m_value); }
    virtual void on_torque2()
        { dJointAddUniversalTorques(
                static_cast<ODEConstraint*>(special())->joint(),
                m_torque1.m_value, m_torque2.m_value); }
};

#endif // _PHYSICS_SIM_H_
