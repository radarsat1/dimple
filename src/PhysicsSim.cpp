// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#include "dimple.h"
#include "PhysicsSim.h"

bool PhysicsPrismFactory::create(const char *name, float x, float y, float z)
{
    printf("PhysicsPrismFactory (%s) is creating a prism object called '%s'\n",
           m_parent->c_name(), name);

    OscPrismODE *obj = new OscPrismODE(simulation()->odeWorld(),
                                         simulation()->odeSpace(),
                                         name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.set(x, y, z);

    return true;
}

bool PhysicsSphereFactory::create(const char *name, float x, float y, float z)
{
    OscSphereODE *obj = new OscSphereODE(simulation()->odeWorld(),
                                         simulation()->odeSpace(),
                                         name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.set(x, y, z);

    return true;
}

bool PhysicsHingeFactory::create(const char *name, OscObject *object1, OscObject *object2,
                                 double x, double y, double z, double ax, double ay, double az)
{
    printf("PhysicsHingeFactory (%s) is creating a hinge constraint called '%s'\n",
           m_parent->c_name(), name);

    OscHinge *cons=NULL;
    cons = new OscHingeODE(simulation()->odeWorld(),
                           simulation()->odeSpace(),
                           name, m_parent, object1, object2,
                           x, y, z, ax, ay, az);

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

    m_fTimestep = PHYSICS_TIMESTEP_MS/1000.0;
    printf("ODE timestep: %f\n", m_fTimestep);
}

PhysicsSim::~PhysicsSim()
{
}

void PhysicsSim::initialize()
{
    dSetDebugHandler(ode_errorhandler);
    dSetErrorHandler(ode_errorhandler);
    dSetMessageHandler(ode_errorhandler);

    m_odeWorld = dWorldCreate();
    dWorldSetGravity (m_odeWorld,0,0,0);
    m_odeSpace = dSimpleSpaceCreate(0);
    m_odeContactGroup = dJointGroupCreate(0);

    Simulation::initialize();
}

void PhysicsSim::step()
{
    // Perform simulation step
	dSpaceCollide (m_odeSpace, this, &ode_nearCallback);
	dWorldStepFast1 (m_odeWorld, m_fTimestep, 5);
    /*
	for (int j = 0; j < dSpaceGetNumGeoms(ode_space); j++){
		dSpaceGetGeom(ode_space, j);
	}
    */
	dJointGroupEmpty (m_odeContactGroup);

    /* Update positions of each object in the other simulations */
    std::map<std::string,OscObject*>::iterator it;
    for (it=world_objects.begin(); it!=world_objects.end(); it++)
    {
        // TODO: it would be very nice to do this without involving dynamic_cast

        ODEObject *o=NULL;
        OscSphereODE *s = dynamic_cast<OscSphereODE*>(it->second);
        if (s) o = static_cast<ODEObject*>(s);
        else {
            OscPrismODE *p = dynamic_cast<OscPrismODE*>(it->second);
            if (p) o = static_cast<ODEObject*>(p);
        }
        if (o) {
            cVector3d pos(o->getPosition());
            cMatrix3d rot(o->getRotation());
            send(true, (it->second->path()+"/position").c_str(), "fff",
                 pos.x,pos.y,pos.z);
            send(true, (it->second->path()+"/rotation").c_str(), "fffffffff",
                 rot.m[0][0], rot.m[0][1], rot.m[0][2],
                 rot.m[1][0], rot.m[1][1], rot.m[1][2],
                 rot.m[2][0], rot.m[2][1], rot.m[2][2]);
        }
    }
}

void PhysicsSim::ode_nearCallback (void *data, dGeomID o1, dGeomID o2)
{
    PhysicsSim *me = static_cast<PhysicsSim*>(data);

    int i;
    // if (o1->body && o2->body) return;

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
            bool co1 = p1->collidedWith(p2);
            bool co2 = p2->collidedWith(p1);
            if ( (co1 || co2) && me->m_bGetCollide ) {
                /* TODO
                lo_send(address_send, "/object/collide", "ssf", p1->c_name(), p2->c_name(),
                        (double)(p1->getVelocity() + p2->getVelocity()).length());
                */
                // TODO: send collision force instead of velocity?
            }
            // TODO: this strategy will NOT work for multiple collisions between same objects!!
        }
		for (i=0; i<numc; i++) {
			dJointID c = dJointCreateContact (me->m_odeWorld, me->m_odeContactGroup, contact+i);
			dJointAttach (c,b1,b2);
		}
	}
}

/****** ODEObject ******/

ODEObject::ODEObject(dWorldID odeWorld, dSpaceID odeSpace)
    : m_odeWorld(odeWorld), m_odeSpace(odeSpace)
{
    m_odeGeom = NULL;
    m_odeBody = NULL;
    m_odeBody = dBodyCreate(m_odeWorld);
    dBodySetPosition(m_odeBody, 0, 0, 0);
}

ODEObject::~ODEObject()
{
    if (m_odeBody)  dBodyDestroy(m_odeBody);
    if (m_odeGeom)  dGeomDestroy(m_odeGeom);
}

/****** ODEConstraint ******/

ODEConstraint::ODEConstraint(dWorldID odeWorld, dSpaceID odeSpace,
                             OscObject *object1, OscObject *object2)
{
    m_odeWorld = odeWorld;
    m_odeSpace = odeSpace;
    m_odeBody1 = 0;
    m_odeBody2 = 0;

    ODEObject *o = dynamic_cast<ODEObject*>(object1);
    if (o)
        m_odeBody1 = o->m_odeBody;

    if (!object2) {
        printf("constraint created with bodies %#x and world", m_odeBody1);
        return;
    }

    o = dynamic_cast<ODEObject*>(object2);
    if (o)
        m_odeBody2 = o->m_odeBody;

    printf("constraint created with bodies %#x and %#x\n", m_odeBody1, m_odeBody2);
}


/****** OscSphereODE ******/

OscSphereODE::OscSphereODE(dWorldID odeWorld, dSpaceID odeSpace, const char *name, OscBase *parent)
    : OscSphere(NULL, name, parent), ODEObject(odeWorld, odeSpace)
{
    m_odeGeom = dCreateSphere(m_odeSpace, 0.1);
    dGeomSetPosition(m_odeGeom, 0, 0, 0);
    dMassSetSphere(&m_odeMass, 0.1, 0.1);
    dBodySetMass(m_odeBody, &m_odeMass);
    dGeomSetBody(m_odeGeom, m_odeBody);
    dBodySetPosition(m_odeBody, 0, 0, 0);
}

void OscSphereODE::on_radius()
{
    printf("OscSphereODE::on_radius(). radius = %f\n", m_radius.m_value);
    dGeomSphereSetRadius(m_odeGeom, m_radius.m_value);
}

void OscSphereODE::on_force()
{
    printf("OscSphereODE::on_force(). force = %f, %f, %f\n", m_force.x, m_force.y, m_force.z);
    dBodyAddForce(m_odeBody, m_force.x, m_force.y, m_force.z);
}

/****** OscPrismODE ******/

OscPrismODE::OscPrismODE(dWorldID odeWorld, dSpaceID odeSpace, const char *name, OscBase *parent)
    : OscPrism(NULL, name, parent), ODEObject(odeWorld, odeSpace)
{
    m_odeGeom = dCreateBox(m_odeSpace, 0.01, 0.01, 0.01);
    dGeomSetPosition(m_odeGeom, 0, 0, 0);
    dMassSetBox(&m_odeMass, 1.0, 0.01, 0.01, 0.01);
    dBodySetMass(m_odeBody, &m_odeMass);
    dGeomSetBody(m_odeGeom, m_odeBody);
    dBodySetPosition(m_odeBody, 0, 0, 0);
}

void OscPrismODE::on_size()
{
    if (m_size.x <= 0)
        m_size.x = 0.0001;
    if (m_size.y <= 0)
        m_size.y = 0.0001;
    if (m_size.z <= 0)
        m_size.z = 0.0001;

    // TODO: need previous values here!
#if 0

    // calculate the ratio between the two sizes
    cVector3d ratio;
    ratio.x = a_size[0] / m_size[0];
    ratio.y = a_size[1] / m_size[1];
    ratio.z = a_size[2] / m_size[2];

    // assign new size
    m_size = a_size;

    // remember original mass
    dMass mass;
    dReal m;
    dBodyGetMass(m_odeBody, &mass);
    m = mass.mass;

    // scale the mass accordingly
    setDynamicMass(m*ratio.x*ratio.y*ratio.z);
#endif

    // resize ODE geom
    dGeomBoxSetLengths (m_odeGeom, m_size[0], m_size[1], m_size[2]);
}

void OscPrismODE::on_force()
{
    printf("OscPrismODE::on_force(). force = %f, %f, %f\n", m_force.x, m_force.y, m_force.z);
    dBodyAddForce(m_odeBody, m_force.x, m_force.y, m_force.z);
}

//! A hinge requires a fixed anchor point and an axis
OscHingeODE::OscHingeODE(dWorldID odeWorld, dSpaceID odeSpace,
                         const char *name, OscBase* parent,
                         OscObject *object1, OscObject *object2,
                         double x, double y, double z, double ax, double ay, double az)
    : OscHinge(name, parent, object1, object2, x, y, z, ax, ay, az),
      ODEConstraint(odeWorld, odeSpace, object1, object2)
{
	// create the constraint for object1
    cVector3d anchor(x,y,z);
    cVector3d axis(ax,ay,az);
    
    m_odeJoint = dJointCreateHinge(m_odeWorld,0);
    dJointAttach(m_odeJoint, m_odeBody1, m_odeBody2);
    dJointSetHingeAnchor(m_odeJoint, anchor.x, anchor.y, anchor.z);
    dJointSetHingeAxis(m_odeJoint, axis.x, axis.y, axis.z);

    printf("Hinge joint created between %s and %s at anchor (%f,%f,%f), axis (%f,%f,%f)\n",
        object1->c_name(), object2?object2->c_name():"world", x,y,z,ax,ay,az);
}

//! This function is called once per simulation step, allowing the
//! constraint to be "motorized" according to some response.
//! It runs in the physics thread.
void OscHingeODE::simulationCallback()
{
    dJointID *id;
    if (!m_object1->odePrimitive()->getJoint(m_name, id))
        return;

    dReal angle = dJointGetHingeAngle(*id);
    dReal rate = dJointGetHingeAngleRate(*id);
    m_torque.set(-m_stiffness*angle - m_damping*rate);
    dJointAddHingeTorque(*id, m_torque.m_value);
}
