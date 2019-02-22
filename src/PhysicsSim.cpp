// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-

#include "dimple.h"
#include "PhysicsSim.h"
#include <cassert>

bool PhysicsPrismFactory::create(const char *name, float x, float y, float z)
{
    OscPrismODE *obj = new OscPrismODE(simulation()->odeWorld(),
                                         simulation()->odeSpace(),
                                         name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.setValue(x, y, z);

    return true;
}

bool PhysicsSphereFactory::create(const char *name, float x, float y, float z)
{
    OscSphereODE *obj = new OscSphereODE(simulation()->odeWorld(),
                                         simulation()->odeSpace(),
                                         name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.setValue(x, y, z);

    return true;
}

bool PhysicsHingeFactory::create(const char *name, OscObject *object1, OscObject *object2,
                                 double x, double y, double z, double ax, double ay, double az)
{
    OscHinge *cons=NULL;
    cons = new OscHingeODE(simulation()->odeWorld(),
                           simulation()->odeSpace(),
                           name, m_parent, object1, object2,
                           x, y, z, ax, ay, az);

    cons->m_response->traceOn();

    if (cons)
        return simulation()->add_constraint(*cons);
}

bool PhysicsHinge2Factory::create(const char *name, OscObject *object1,
                                  OscObject *object2, double x,
                                  double y, double z, double a1x,
                                  double a1y, double a1z, double a2x,
                                  double a2y, double a2z)
{
    OscHinge2 *cons=NULL;
    cons = new OscHinge2ODE(simulation()->odeWorld(),
                            simulation()->odeSpace(),
                            name, m_parent, object1, object2,
                            x, y, z, a1x, a1y, a1z, a2x, a2y, a2z);

    if (cons)
        return simulation()->add_constraint(*cons);
}

bool PhysicsFixedFactory::create(const char *name, OscObject *object1, OscObject *object2)
{
    OscFixed *cons=NULL;
    cons = new OscFixedODE(simulation()->odeWorld(),
                           simulation()->odeSpace(),
                           name, m_parent, object1, object2);

    if (cons)
        return simulation()->add_constraint(*cons);
}

bool PhysicsFreeFactory::create(const char *name,
                                OscObject *object1, OscObject *object2)
{
    OscFree *cons=NULL;
    cons = new OscFreeODE(simulation()->odeWorld(),
                          simulation()->odeSpace(),
                          name, m_parent, object1, object2);

    cons->m_response->traceOn();

    if (cons)
        return simulation()->add_constraint(*cons);
}

bool PhysicsBallJointFactory::create(const char *name, OscObject *object1,
                                     OscObject *object2, double x, double y,
                                     double z)
{
    OscBallJoint *cons=NULL;
    cons = new OscBallJointODE(simulation()->odeWorld(),
                               simulation()->odeSpace(),
                               name, m_parent, object1, object2,
                               x, y, z);

    if (cons)
        return simulation()->add_constraint(*cons);
}

bool PhysicsSlideFactory::create(const char *name, OscObject *object1,
                                 OscObject *object2, double ax,
                                 double ay, double az)
{
    OscSlide *cons=NULL;
    cons = new OscSlideODE(simulation()->odeWorld(),
                           simulation()->odeSpace(),
                           name, m_parent, object1, object2,
                           ax, ay, az);

    if (cons)
        return simulation()->add_constraint(*cons);
}

bool PhysicsPistonFactory::create(const char *name, OscObject *object1,
                                  OscObject *object2, double x, double y,
                                  double z, double ax, double ay, double az)
{
    OscPiston *cons=NULL;
    cons = new OscPistonODE(simulation()->odeWorld(),
                            simulation()->odeSpace(),
                            name, m_parent, object1, object2,
                            x, y, z, ax, ay, az);

    if (cons)
        return simulation()->add_constraint(*cons);
}

bool PhysicsUniversalFactory::create(const char *name, OscObject *object1,
                                     OscObject *object2, double x,
                                     double y, double z, double a1x,
                                     double a1y, double a1z, double a2x,
                                     double a2y, double a2z)
{
    OscUniversal *cons=NULL;
    cons = new OscUniversalODE(simulation()->odeWorld(),
                               simulation()->odeSpace(),
                               name, m_parent, object1, object2,
                               x, y, z, a1x, a1y, a1z, a2x, a2y, a2z);

    if (cons)
        return simulation()->add_constraint(*cons);
}

/****** PhysicsSim ******/

const int PhysicsSim::MAX_CONTACTS = 30;

PhysicsSim::PhysicsSim(const char *port)
    : Simulation(port, ST_PHYSICS)
{
    m_pPrismFactory = new PhysicsPrismFactory(this);
    m_pSphereFactory = new PhysicsSphereFactory(this);
    m_pHingeFactory = new PhysicsHingeFactory(this);
    m_pHinge2Factory = new PhysicsHinge2Factory(this);
    m_pFixedFactory = new PhysicsFixedFactory(this);
    m_pFreeFactory = new PhysicsFreeFactory(this);
    m_pBallJointFactory = new PhysicsBallJointFactory(this);
    m_pSlideFactory = new PhysicsSlideFactory(this);
    m_pPistonFactory = new PhysicsPistonFactory(this);
    m_pUniversalFactory = new PhysicsUniversalFactory(this);

    m_pGrabbedObject = NULL;

    m_fTimestep = physics_timestep_ms/1000.0;
    m_counter = 0;
    printf("ODE timestep: %f\n", m_fTimestep);
}

PhysicsSim::~PhysicsSim()
{
    // Stop the simulation before deleting objects, otherwise thread
    // is still running and may dereference them.
    stop();
}

void PhysicsSim::initialize()
{
    dInitODE();

    dSetDebugHandler(ode_errorhandler);
    dSetErrorHandler(ode_errorhandler);
    dSetMessageHandler(ode_errorhandler);

    m_odeWorld = dWorldCreate();
    dWorldSetGravity (m_odeWorld,0,0,0);
    m_odeSpace = dSimpleSpaceCreate(0);
    m_odeContactGroup = dJointGroupCreate(0);

    /* This is just to track haptics cursor during "grab" state.
     * We only need its position, so just use a generic OscObject. */
    m_pCursor = new OscObject(NULL, "cursor", this);
    if (!m_pCursor)
        printf("Error creating PhysicsSim cursor.\n");

    Simulation::initialize();
}

void PhysicsSim::step()
{
    // Add extra forces to objects
    // Grabbed object attraction
    if (m_pGrabbedObject)
    {
        cVector3d grab_force(m_pGrabbedObject->getPosition()
                             - m_pCursor->m_position);

        grab_force.mul(-0.01);
        grab_force.add(m_pGrabbedODEObject->getVelocity()*(-0.0003));
        dBodyAddForce(m_pGrabbedODEObject->body(),
                      grab_force.x(), grab_force.y(), grab_force.z());
    }

    // Perform simulation step
	dSpaceCollide (m_odeSpace, this, &ode_nearCallback);
	dWorldQuickStep (m_odeWorld, m_fTimestep);
	dJointGroupEmpty (m_odeContactGroup);

    /* Update positions of each object in the other simulations */
    std::map<std::string,OscObject*>::iterator it;
    for (it=world_objects.begin(); it!=world_objects.end(); it++)
    {
        ODEObject *o = static_cast<ODEObject*>(it->second->special());

        if (o) {
            o->update();
            cVector3d pos(o->getPosition());
            cMatrix3d rot(o->getRotation());
            send(true, (it->second->path()+"/position").c_str(), "fff",
                 pos.x(),pos.y(),pos.z());
            send(true, (it->second->path()+"/rotation").c_str(), "fffffffff",
                 rot(0,0), rot(0,1), rot(0,2),
                 rot(1,0), rot(1,1), rot(1,2),
                 rot(2,0), rot(2,1), rot(2,2));
        }
    }

    /* Update the responses of each constraint. */
    std::map<std::string,OscConstraint*>::iterator cit;
    for (cit=world_constraints.begin(); cit!=world_constraints.end(); cit++)
    {
        cit->second->simulationCallback();
    }

    m_counter++;
}

void PhysicsSim::ode_nearCallback (void *data, dGeomID o1, dGeomID o2)
{
    PhysicsSim *me = static_cast<PhysicsSim*>(data);

    int i;

    // exit without doing anything if the two bodies are connected by a joint
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    if (b1 && b2 && dAreConnectedExcluding (b1,b2,dJointTypeContact)) return;

    dContact contact[MAX_CONTACTS];   // up to MAX_CONTACTS contacts per box-box
    for (i=0; i<MAX_CONTACTS; i++) {
        contact[i].surface.mode = dContactBounce | dContactSoftCFM;
        contact[i].surface.mu = dInfinity;
        contact[i].surface.mu2 = 0;
        contact[i].surface.bounce = 0.1;
        contact[i].surface.bounce_vel = 0.1;
        contact[i].surface.soft_cfm = 0.01;
    }

	if (int numc = dCollide (o1,o2,MAX_CONTACTS,&contact[0].geom,sizeof(dContact)))
	{
        OscObject *p1 = static_cast<OscObject*>(dGeomGetData(o1));
        OscObject *p2 = static_cast<OscObject*>(dGeomGetData(o2));
        if (p1 && p2) {
            bool co1 = p1->collidedWith(p2, me->m_counter);
            bool co2 = p2->collidedWith(p1, me->m_counter);
            if ( (co1 || co2) && me->m_collide.m_value ) {
                lo_send(address_send, "/world/collide", "ssf",
                        p1->c_name(), p2->c_name(),
                        (double)(p1->m_velocity - p2->m_velocity).length());
            }
            // TODO: this strategy will NOT work for multiple collisions between same objects!!
        }
		for (i=0; i<numc; i++) {
			dJointID c = dJointCreateContact (me->m_odeWorld, me->m_odeContactGroup, contact+i);
			dJointAttach (c,b1,b2);
		}
	}
}

void PhysicsSim::set_grabbed(OscObject *pGrabbed)
{
    Simulation::set_grabbed(pGrabbed);
    m_pGrabbedODEObject = NULL;
    if (pGrabbed)
        m_pGrabbedODEObject = dynamic_cast<ODEObject*>(pGrabbed->special());
}

/****** ODEObject ******/

ODEObject::ODEObject(OscObject *obj, dGeomID odeGeom, dWorldID odeWorld, dSpaceID odeSpace)
    : m_odeWorld(odeWorld), m_odeSpace(odeSpace)
{
    m_object = obj;

    m_odeGeom = odeGeom;
    m_odeBody = NULL;
    m_odeBody = dBodyCreate(m_odeWorld);

    assert(m_odeGeom!=NULL);

    dBodySetPosition(m_odeBody, 0, 0, 0);
    dGeomSetPosition(m_odeGeom, 0, 0, 0);

    // note: owners must override this by setting the density. can't
    //       do it here because obj->m_pSpecial is not yet
    //       initialized.
    dMassSetSphere(&m_odeMass, 1, 1);
    dBodySetMass(m_odeBody, &m_odeMass);

    dGeomSetBody(m_odeGeom, m_odeBody);
    dGeomSetData(m_odeGeom, obj);

    if (!obj) return;

    obj->m_rotation.setSetCallback(ODEObject::on_set_rotation, this);
    obj->m_position.setSetCallback(ODEObject::on_set_position, this);
    obj->m_velocity.setSetCallback(ODEObject::on_set_velocity, this);
    obj->m_accel.setSetCallback(ODEObject::on_set_accel, this);
    obj->m_force.setSetCallback(ODEObject::on_set_force, this);

    obj->addHandler("push", "ffffff", ODEObject::push_handler);
}

ODEObject::~ODEObject()
{
    if (m_odeBody)  dBodyDestroy(m_odeBody);
    if (m_odeGeom)  dGeomDestroy(m_odeGeom);
}

void ODEObject::update()
{
    OscObject *o = object();
    if (!o) return;

    cVector3d vel(o->m_velocity);
    float t = object()->simulation()->timestep();

    // Set position & velocity
    // (without feeding back effect to the simulation)
    o->m_position.setValue(getPosition(), false);
    o->m_velocity.setValue(getVelocity(), false);
    o->m_accel.setValue((vel - o->m_velocity) / t, false);
}

void ODEObject::on_set_rotation(void *me, OscMatrix3 &r)
{
    // Convert from a CHAI rotation matrix to an ODE rotation matrix
    dMatrix3 m;
    m[ 0] = r.getCol0().x();
    m[ 1] = r.getCol0().y();
    m[ 2] = r.getCol0().z();
    m[ 3] = 0;
    m[ 4] = r.getCol1().x();
    m[ 5] = r.getCol1().y();
    m[ 6] = r.getCol1().z();
    m[ 7] = 0;
    m[ 8] = r.getCol2().x();
    m[ 9] = r.getCol2().y();
    m[10] = r.getCol2().z();
    m[11] = 0;
    dGeomSetRotation(((ODEObject*)me)->m_odeGeom, m);
}

void ODEObject::on_set_position(void *me, OscVector3 &p)
{
    dGeomSetPosition(((ODEObject*)me)->m_odeGeom, p.x(), p.y(), p.z());
}

void ODEObject::on_set_velocity(void *me, OscVector3 &v)
{
    dBodySetLinearVel(((ODEObject*)me)->m_odeBody, v.x(), v.y(), v.z());
}

void ODEObject::on_set_accel(void *me, OscVector3 &a)
{
    dBodySetForce(((ODEObject*)me)->m_odeBody,
                  a.x() / ((ODEObject*)me)->m_odeMass.mass,
                  a.y() / ((ODEObject*)me)->m_odeMass.mass,
                  a.z() / ((ODEObject*)me)->m_odeMass.mass);
}

void ODEObject::on_set_force(void *me, OscVector3 &f)
{
    dBodyAddForce(((ODEObject*)me)->m_odeBody, f.x(), f.y(), f.z());
}


int ODEObject::push_handler(const char *path, const char *types,
                            lo_arg **argv, int argc,
                            void *data, void *user_data)
{
    OscObject *me = static_cast<OscObject*>(user_data);
    ODEObject *ode_object = static_cast<ODEObject*>(me->special());
    cVector3d(argv[0]->f, argv[1]->f, argv[2]->f).copyto(me->m_force);
    dBodyAddForceAtPos(ode_object->body(),
                       argv[0]->f, argv[1]->f, argv[2]->f,
                       argv[3]->f, argv[4]->f, argv[5]->f);
    return 0;
}

/****** ODEConstraint ******/

ODEConstraint::ODEConstraint(OscConstraint *c, dJointID odeJoint,
                             dWorldID odeWorld, dSpaceID odeSpace,
                             OscObject *object1, OscObject *object2)
{
    m_constraint = c;
    m_odeWorld = odeWorld;
    m_odeSpace = odeSpace;
    m_odeBody1 = 0;
    m_odeBody2 = 0;
    m_odeJoint = odeJoint;

    ODEObject *o = NULL;
    if (object1)
        o = dynamic_cast<ODEObject*>(object1->special());

    if (o)
        m_odeBody1 = o->m_odeBody;

    if (object2) {
        o = NULL;
        if (object2)
            o = dynamic_cast<ODEObject*>(object2->special());
        if (o)
            m_odeBody2 = o->m_odeBody;
    }
    else {
        printf("constraint created with bodies %#x and world.\n", m_odeBody1);
    }

    if (m_odeJoint)
        dJointAttach(m_odeJoint, m_odeBody1, m_odeBody2);

    printf("constraint created with bodies %#x and %#x.\n", m_odeBody1, m_odeBody2);
}

ODEConstraint::~ODEConstraint()
{
    if (m_odeJoint)
        dJointDestroy(m_odeJoint);
}

/****** OscSphereODE ******/

OscSphereODE::OscSphereODE(dWorldID odeWorld, dSpaceID odeSpace, const char *name, OscBase *parent)
    : OscSphere(NULL, name, parent)
{
    dGeomID odeGeom = dCreateSphere(odeSpace, m_radius.m_value);

    m_pSpecial = new ODEObject(this, odeGeom, odeWorld, odeSpace);
    m_density.setValue(m_density.m_value);
}

void OscSphereODE::on_radius()
{
    ODEObject *ode_object = static_cast<ODEObject*>(special());
    dGeomSphereSetRadius(ode_object->geom(), m_radius.m_value);

    // reset the mass to maintain same density
    dMassSetSphere(&ode_object->mass(), m_density.m_value, m_radius.m_value);

    m_mass.m_value = ode_object->mass().mass;
}

void OscSphereODE::on_mass()
{
    ODEObject *ode_object = static_cast<ODEObject*>(special());
    if (m_mass.m_value < 1e-9) {
        printf("[%s] Mass for %s is too small, setting to 1e-9.\n",
               simulation()->type_str(), c_name(), m_mass.m_value);
        m_mass.m_value = 1e-6;
    }
    dMassSetSphereTotal(&ode_object->mass(), m_mass.m_value, m_radius.m_value);
    dBodySetMass(ode_object->body(), &ode_object->mass());

    dReal volume = 4*M_PI*m_radius.m_value*m_radius.m_value*m_radius.m_value/3;
    m_density.m_value = m_mass.m_value / volume;
}

void OscSphereODE::on_density()
{
    ODEObject *ode_object = static_cast<ODEObject*>(special());
    dMassSetSphere(&ode_object->mass(), m_density.m_value, m_radius.m_value);
    dBodySetMass(ode_object->body(), &ode_object->mass());

    m_mass.m_value = ode_object->mass().mass;
}

/****** OscPrismODE ******/

OscPrismODE::OscPrismODE(dWorldID odeWorld, dSpaceID odeSpace, const char *name, OscBase *parent)
    : OscPrism(NULL, name, parent)
{
    dGeomID odeGeom = dCreateBox(odeSpace, m_size.x(), m_size.y(), m_size.z());

    m_pSpecial = new ODEObject(this, odeGeom, odeWorld, odeSpace);
    m_density.setValue(m_density.m_value);
}

void OscPrismODE::on_size()
{
    if (m_size.x() <= 0)
        m_size.x(0.0001);
    if (m_size.y() <= 0)
        m_size.y(0.0001);
    if (m_size.z() <= 0)
        m_size.z(0.0001);

    ODEObject *ode_object = static_cast<ODEObject*>(special());

    // resize ODE geom
    dGeomBoxSetLengths (ode_object->geom(), m_size(0), m_size(1), m_size(2));

    // reset the mass to maintain same density
    dMassSetBox(&ode_object->mass(), m_density.m_value,
                m_size(0), m_size(1), m_size(2));
    dBodySetMass(ode_object->body(), &ode_object->mass());

    m_mass.m_value = ode_object->mass().mass;
}

void OscPrismODE::on_mass()
{
    ODEObject *ode_object = static_cast<ODEObject*>(special());
    dMassSetBoxTotal(&ode_object->mass(), m_mass.m_value,
                     m_size.x(), m_size.y(), m_size.z());
    dBodySetMass(ode_object->body(), &ode_object->mass());

    dReal volume = m_size.x() * m_size.y() * m_size.z();
    m_density.m_value = m_mass.m_value / volume;
}

void OscPrismODE::on_density()
{
    ODEObject *ode_object = static_cast<ODEObject*>(special());
    dMassSetBox(&ode_object->mass(), m_density.m_value,
                m_size.x(), m_size.y(), m_size.z());
    dBodySetMass(ode_object->body(), &ode_object->mass());

    m_mass.m_value = ode_object->mass().mass;
}

//! A hinge requires a fixed anchor point and an axis
OscHingeODE::OscHingeODE(dWorldID odeWorld, dSpaceID odeSpace,
                         const char *name, OscBase* parent,
                         OscObject *object1, OscObject *object2,
                         double x, double y, double z, double ax, double ay, double az)
    : OscHinge(name, parent, object1, object2, x, y, z, ax, ay, az)
{
    m_response = new OscResponse("response",this);

	// create the constraint for object1
    cVector3d anchor(x,y,z);
    cVector3d axis(ax,ay,az);

    dJointID odeJoint = dJointCreateHinge(odeWorld,0);

    m_pSpecial = new ODEConstraint(this, odeJoint, odeWorld, odeSpace,
                                   object1, object2);

    dJointSetHingeAnchor(odeJoint, anchor.x(), anchor.y(), anchor.z());
    dJointSetHingeAxis(odeJoint, axis.x(), axis.y(), axis.z());

    printf("Hinge joint created between %s and %s at anchor (%f,%f,%f), axis (%f,%f,%f)\n",
        object1->c_name(), object2?object2->c_name():"world", x,y,z,ax,ay,az);
}

OscHingeODE::~OscHingeODE()
{
    delete m_response;
}

//! This function is called once per simulation step, allowing the
//! constraint to be "motorized" according to some response.  It runs
//! in the physics thread.
void OscHingeODE::simulationCallback()
{
    ODEConstraint& me = *static_cast<ODEConstraint*>(special());

    dReal angle = dJointGetHingeAngle(me.joint());
    dReal rate = dJointGetHingeAngleRate(me.joint());

    dReal addtorque =
        - m_response->m_stiffness.m_value*angle
        - m_response->m_damping.m_value*rate;

    // Limit the torque otherwise we get ODE assertions.
    if (addtorque >  1000) addtorque =  1000;
    if (addtorque < -1000) addtorque = -1000;

    m_torque.m_value = addtorque;
    m_angle.m_value = angle;

    dJointAddHingeTorque(me.joint(), addtorque);
}

OscHinge2ODE::OscHinge2ODE(dWorldID odeWorld, dSpaceID odeSpace,
                           const char *name, OscBase *parent,
                           OscObject *object1, OscObject *object2,
                           double x, double y, double z, double a1x,
                           double a1y, double a1z, double a2x,
                           double a2y, double a2z)
    : OscHinge2(name, parent, object1, object2, x, y, z,
                a1x, a1y, a1z, a2x, a2y, a2z)
{
    m_response = new OscResponse("response",this);

    dJointID odeJoint = dJointCreateHinge2(odeWorld,0);

    m_pSpecial = new ODEConstraint(this, odeJoint, odeWorld, odeSpace,
                                   object1, object2);

    dJointSetHinge2Anchor(odeJoint, x, y, z);
    dJointSetHinge2Axis1(odeJoint, a1x, a1y, a1z);
    dJointSetHinge2Axis2(odeJoint, a2x, a2y, a2z);

    printf("[%s] Hinge2 joint created between %s and %s at (%f, %f, %f) for axes (%f, %f, %f) and (%f,%f,%f)\n",
           simulation()->type_str(),
           object1->c_name(), object2?object2->c_name():"world",
           x, y, z, a1x, a1y, a1z, a2x, a2y, a2z);
}

OscHinge2ODE::~OscHinge2ODE()
{
    delete m_response;
}

//! This function is called once per simulation step, allowing the
//! constraint to be "motorized" according to some response.
void OscHinge2ODE::simulationCallback()
{
    ODEConstraint& me = *static_cast<ODEConstraint*>(special());

    dReal angle1 = dJointGetHinge2Angle1(me.joint());
    dReal rate1 = dJointGetHinge2Angle1Rate(me.joint());

    dReal addtorque1 =
        - m_response->m_stiffness.m_value*angle1
        - m_response->m_damping.m_value*rate1;

#if 0  // TODO: dJointGetHinge2Angle2 is not yet available in ODE.
    dReal angle2 = dJointGetHinge2Angle2(me.joint());
#else
    dReal angle2 = 0;
#endif
    dReal rate2 = dJointGetHinge2Angle2Rate(me.joint());

    dReal addtorque2 =
        - m_response->m_stiffness.m_value*angle2
        - m_response->m_damping.m_value*rate2;

    m_torque1.m_value = addtorque1;
    m_torque2.m_value = addtorque2;

    m_angle1.m_value = angle1;
    m_angle2.m_value = angle2;

    dJointAddHinge2Torques(me.joint(), addtorque1, addtorque2);
}

OscFixedODE::OscFixedODE(dWorldID odeWorld, dSpaceID odeSpace,
                         const char *name, OscBase* parent,
                         OscObject *object1, OscObject *object2)
    : OscFixed(name, parent, object1, object2)
{
    if (object2) {
        dJointID odeJoint = dJointCreateFixed(odeWorld,0);

        m_pSpecial = new ODEConstraint(this, odeJoint, odeWorld, odeSpace,
                                       object1, object2);

        dJointSetFixed(odeJoint);
    }
    else {
        ODEObject *o = NULL;
        if (object1)
            o = dynamic_cast<ODEObject*>(object1->special());
        if (o)
            o->disconnectBody();

        m_pSpecial = new ODEConstraint(this, NULL, odeWorld, odeSpace,
                                       object1, object2);
    }

    printf("[%s] Fixed joint created between %s and %s.\n",
           simulation()->type_str(),
        object1->c_name(), object2?object2->c_name():"world");
}

OscFixedODE::~OscFixedODE()
{
    ODEConstraint *c = static_cast<ODEConstraint*>(special());
    if (!object2())
        static_cast<ODEObject*>(object1()->special())->connectBody();
}

OscFreeODE::OscFreeODE(dWorldID odeWorld, dSpaceID odeSpace,
                         const char *name, OscBase* parent,
                         OscObject *object1, OscObject *object2)
    : OscFree(name, parent, object1, object2)
{
    m_response = new OscResponse("response",this);

    // The "Free" constraint is not actually an ODE constraint.
    // However, we need an ODEConstraint to remember the objects so
    // that they can receive forces and torques.

    m_pSpecial = new ODEConstraint(this, NULL, odeWorld, odeSpace,
                                   object1, object2);

    ODEConstraint& cons = *static_cast<ODEConstraint*>(special());
    const dReal *pos1 = dBodyGetPosition(cons.body1());
    const dReal *pos2 = dBodyGetPosition(cons.body2());

    m_initial_distance[0] = pos2[0] - pos1[0];
    m_initial_distance[1] = pos2[1] - pos1[1];
    m_initial_distance[2] = pos2[2] - pos1[2];

    printf("[%s] Free constraint created between %s and %s.\n",
           simulation()->type_str(),
           object1->c_name(), object2->c_name());
}

void OscFreeODE::simulationCallback()
{
    ODEConstraint& me = *static_cast<ODEConstraint*>(special());

    const dReal *pos1 = dBodyGetPosition(me.body1());
    const dReal *pos2 = dBodyGetPosition(me.body2());

    const dReal *vel1 = dBodyGetLinearVel(me.body1());
    const dReal *vel2 = dBodyGetLinearVel(me.body2());

    dReal force[3];

    force[0] =
        - ((pos2[0]-pos1[0]-m_initial_distance[0])
           * m_response->m_stiffness.m_value)
        - (vel2[0]-vel1[0]) * m_response->m_damping.m_value;

    force[1] =
        - ((pos2[1]-pos1[1]-m_initial_distance[1])
           * m_response->m_stiffness.m_value)
        - (vel2[1]-vel1[1]) * m_response->m_damping.m_value;

    force[2] =
        - ((pos2[2]-pos1[2]-m_initial_distance[2])
           * m_response->m_stiffness.m_value)
        - (vel2[2]-vel1[2]) * m_response->m_damping.m_value;

    dBodyAddForce(me.body1(), -force[0], -force[1], -force[2]);
    dBodyAddForce(me.body2(),  force[0],  force[1],  force[2]);
}

OscFreeODE::~OscFreeODE()
{
    delete m_response;
}

void OscFreeODE::on_force()
{
    // TODO
    printf("OscFreeODE::on_force()  (not implemented)\n");
}

void OscFreeODE::on_torque()
{
    // TODO
    printf("OscFreeODE::on_torque()  (not implemented)\n");
}

OscBallJointODE::OscBallJointODE(dWorldID odeWorld, dSpaceID odeSpace,
                                 const char *name, OscBase *parent,
                                 OscObject *object1, OscObject *object2,
                                 double x, double y, double z)
    : OscBallJoint(name, parent, object1, object2, x, y, z)
{
    dJointID odeJoint = dJointCreateBall(odeWorld,0);

    m_pSpecial = new ODEConstraint(this, odeJoint, odeWorld, odeSpace,
                                   object1, object2);

    dJointSetBallAnchor(odeJoint, x, y, z);

    printf("[%s] Ball joint created between %s and %s at (%f,%f,%f)\n",
           simulation()->type_str(),
           object1->c_name(), object2?object2->c_name():"world",
           x, y, z);
}

OscSlideODE::OscSlideODE(dWorldID odeWorld, dSpaceID odeSpace,
                         const char *name, OscBase *parent,
                         OscObject *object1, OscObject *object2,
                         double ax, double ay, double az)
    : OscSlide(name, parent, object1, object2, ax, ay, az)
{
    m_response = new OscResponse("response",this);

    dJointID odeJoint = dJointCreateSlider(odeWorld,0);

    m_pSpecial = new ODEConstraint(this, odeJoint, odeWorld, odeSpace,
                                   object1, object2);

    dJointSetSliderAxis(odeJoint, ax, ay, az);
    /* TODO access to dJointGetSliderPosition */

    printf("[%s] Sliding joint created between %s and %s on axis (%f,%f,%f)\n",
           simulation()->type_str(),
           object1->c_name(), object2?object2->c_name():"world",
           ax, ay, az);
}

OscSlideODE::~OscSlideODE()
{
    delete m_response;
}

void OscSlideODE::simulationCallback()
{
    ODEConstraint& me = *static_cast<ODEConstraint*>(special());

    dReal pos = dJointGetSliderPosition(me.joint());
    dReal rate = dJointGetSliderPositionRate(me.joint());

    dReal addforce =
        - m_response->m_stiffness.m_value*pos
        - m_response->m_damping.m_value*rate;

    m_force.m_value = addforce;
    m_position.m_value = pos;

    dJointAddSliderForce(me.joint(), addforce);
}

//! A piston requires a fixed anchor point and an axis
OscPistonODE::OscPistonODE(dWorldID odeWorld, dSpaceID odeSpace,
                           const char *name, OscBase* parent,
                           OscObject *object1, OscObject *object2,
                           double x, double y, double z,
                           double ax, double ay, double az)
    : OscPiston(name, parent, object1, object2, x, y, z, ax, ay, az)
{
    m_response = new OscResponse("response",this);

	// create the constraint for object1
    cVector3d anchor(x,y,z);
    cVector3d axis(ax,ay,az);

    dJointID odeJoint = dJointCreatePiston(odeWorld,0);

    m_pSpecial = new ODEConstraint(this, odeJoint, odeWorld, odeSpace,
                                   object1, object2);

    dJointSetPistonAnchor(odeJoint, anchor.x(), anchor.y(), anchor.z());
    dJointSetPistonAxis(odeJoint, axis.x(), axis.y(), axis.z());

    printf("Piston joint created between %s and %s at anchor (%f,%f,%f), axis (%f,%f,%f)\n",
        object1->c_name(), object2?object2->c_name():"world", x,y,z,ax,ay,az);
}

OscPistonODE::~OscPistonODE()
{
    delete m_response;
}

void OscPistonODE::simulationCallback()
{
    ODEConstraint& me = *static_cast<ODEConstraint*>(special());

    dReal pos = dJointGetPistonPosition(me.joint());
    dReal rate = dJointGetPistonPositionRate(me.joint());

    dReal addforce =
        - m_response->m_stiffness.m_value*pos
        - m_response->m_damping.m_value*rate;

    m_force.m_value = addforce;
    m_position.m_value = pos;

    dJointAddPistonForce(me.joint(), addforce);
}

OscUniversalODE::OscUniversalODE(dWorldID odeWorld, dSpaceID odeSpace,
                                 const char *name, OscBase *parent,
                                 OscObject *object1, OscObject *object2,
                                 double x, double y, double z, double a1x,
                                 double a1y, double a1z, double a2x,
                                 double a2y, double a2z)
    : OscUniversal(name, parent, object1, object2, x, y, z,
                   a1x, a1y, a1z, a2x, a2y, a2z)
{
    m_response = new OscResponse("response",this);

    dJointID odeJoint = dJointCreateUniversal(odeWorld,0);

    m_pSpecial = new ODEConstraint(this, odeJoint, odeWorld, odeSpace,
                                   object1, object2);

    dJointSetUniversalAnchor(odeJoint, x, y, z);
    dJointSetUniversalAxis1(odeJoint, a1x, a1y, a1z);
    dJointSetUniversalAxis2(odeJoint, a2x, a2y, a2z);

    printf("[%s] Universal joint created between %s and %s at (%f, %f, %f) for axes (%f, %f, %f) and (%f,%f,%f)\n",
           simulation()->type_str(),
           object1->c_name(), object2?object2->c_name():"world",
           x, y, z, a1x, a1y, a1z, a2x, a2y, a2z);
}

OscUniversalODE::~OscUniversalODE()
{
    delete m_response;
}

//! This function is called once per simulation step, allowing the
//! constraint to be "motorized" according to some response.
void OscUniversalODE::simulationCallback()
{
    ODEConstraint& me = *static_cast<ODEConstraint*>(special());

    dReal angle1 = dJointGetUniversalAngle1(me.joint());
    dReal rate1 = dJointGetUniversalAngle1Rate(me.joint());

    dReal addtorque1 =
        - m_response->m_stiffness.m_value*angle1
        - m_response->m_damping.m_value*rate1;

    dReal angle2 = dJointGetUniversalAngle2(me.joint());
    dReal rate2 = dJointGetUniversalAngle2Rate(me.joint());

    dReal addtorque2 =
        - m_response->m_stiffness.m_value*angle2
        - m_response->m_damping.m_value*rate2;

    m_torque1.m_value = addtorque1;
    m_torque2.m_value = addtorque2;
    m_angle1.m_value = angle1;
    m_angle2.m_value = angle2;

    dJointAddUniversalTorques(me.joint(), addtorque1, addtorque2);
}
