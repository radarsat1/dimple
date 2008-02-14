// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#include "dimple.h"
#include "PhysicsSim.h"

bool PhysicsPrismFactory::create(const char *name, float x, float y, float z)
{
    printf("PhysicsPrismFactory (%s) is creating a prism object called '%s'\n",
           m_parent->c_name(), name);
    return true;
}

bool PhysicsSphereFactory::create(const char *name, float x, float y, float z)
{
    OscSphereODE *obj = new OscSphereODE(simulation()->odeWorld(),
                                         simulation()->odeSpace(),
                                         name, m_parent);
    if (obj)
        return simulation()->add_object(*obj);

    return false;
}

/****** PhysicsSim ******/

const int PhysicsSim::MAX_CONTACTS = 30;

PhysicsSim::PhysicsSim(const char *port)
    : Simulation(port, ST_PHYSICS)
{
    m_pPrismFactory = new PhysicsPrismFactory(this);
    m_pSphereFactory = new PhysicsSphereFactory(this);

    dSetDebugHandler(ode_errorhandler);
    dSetErrorHandler(ode_errorhandler);
    dSetMessageHandler(ode_errorhandler);

    m_odeWorld = dWorldCreate();
    dWorldSetGravity (m_odeWorld,0,0,0);
    m_fTimestep = PHYSICS_TIMESTEP_MS/1000.0;
    printf("ODE timestep: %f\n", m_fTimestep);
    m_odeSpace = dSimpleSpaceCreate(0);
    m_odeContactGroup = dJointGroupCreate(0);
}

PhysicsSim::~PhysicsSim()
{
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
        ODEObject *o = static_cast<ODEObject*>((OscSphereODE*)(it->second));
        cVector3d pos(o->getPosition());
        send(true, (it->second->path()+"/position").c_str(),"fff",pos.x,pos.y,pos.z);
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
}

void OscSphereODE::on_force()
{
    printf("OscSphereODE::on_force(). force = %f, %f, %f\n", m_force.x, m_force.y, m_force.z);
    dBodyAddForce(m_odeBody, m_force.x, m_force.y, m_force.z);
}
