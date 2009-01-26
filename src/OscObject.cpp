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

#include "OscObject.h"
#include "ValueTimer.h"
#include "Simulation.h"
#include <assert.h>

// ----------------------------------------------------------------------------------

//! OscObject has a CHAI/ODE object associated with it. Class name = "object"
OscObject::OscObject(cGenericObject* p, const char *name, OscBase *parent)
    : OscBase(name, parent),  // was "object"
      m_velocity("velocity", this),
      m_accel("acceleration", this),
      m_position("position", this),
      m_force("force", this),
      m_color("color", this),
      m_friction_static("friction/static", this),
      m_friction_dynamic("friction/dynamic", this),
      m_texture_image("texture/image", this),
      m_rotation("rotation", this),
      m_mass("mass", this),
      m_collide("collide", this)
{
    // Track pointer for ODE/Chai object
    m_objChai = p;

    // Set user data to point to this object for ODE geom, so that
    // we can identify this OscObject during collision detction
    if (odePrimitive())
        odePrimitive()->setGeomData(this);

    // Create handlers for OSC messages
    addHandler("destroy"    , ""   , OscObject::destroy_handler);
    addHandler("grab"       , ""   , OscObject::grab_handler);
    addHandler("grab"       , "i"  , OscObject::grab_handler);
    addHandler("oscillate"  , "ff" , OscObject::oscillate_handler);
    addHandler("visible"    , "i"  , OscObject::visible_handler);

    // Set initial physical properties
    m_accel.set(0,0,0);
    m_velocity.set(0,0,0);
    m_position.set(0,0,0);
    m_force.set(0,0,0);

    // Sane friction defaults
    m_friction_static.set(1);
    m_friction_dynamic.set(0.5);

    // Set callbacks for when values change
    m_position.setSetCallback(set_position, this, DIMPLE_THREAD_PHYSICS);
    m_rotation.setSetCallback(set_rotation, this, DIMPLE_THREAD_PHYSICS);
    m_force.setSetCallback(set_force, this, DIMPLE_THREAD_PHYSICS);
    m_color.setSetCallback(set_color, this, DIMPLE_THREAD_PHYSICS);
    m_velocity.setSetCallback(set_velocity, this, DIMPLE_THREAD_PHYSICS);
    m_friction_static.setSetCallback((OscScalar::SetCallback*)setFrictionStatic, this, DIMPLE_THREAD_HAPTICS);
    m_friction_dynamic.setSetCallback((OscScalar::SetCallback*)setFrictionDynamic, this, DIMPLE_THREAD_HAPTICS);
    m_texture_image.setSetCallback((OscString::SetCallback*)setTextureImage, this, DIMPLE_THREAD_HAPTICS);
    m_mass.setSetCallback(set_mass, this, DIMPLE_THREAD_PHYSICS);
    m_collide.setSetCallback(set_collide, this, DIMPLE_THREAD_PHYSICS);

    // If the new object is supposed to be a part of a
    // composite object, find it and join.
	const char *s;
    if ((s=strchr(name, '/'))) {
        char firstname[256];
        int len = (s-name<255)?(s-name):255;
        strncpy(firstname, name, len);
        firstname[len]=0;
        
        OscObject *obj = findObject(firstname);
        OscComposite *parent = dynamic_cast<OscComposite*>(obj);
        if (!parent) {
            if (obj) return; // error, parent is not a composite object
            parent = new OscComposite(firstname);
            world_objects[firstname] = parent;
        }

        parent->addChild(this);
    }
}

//! OscObject destructor.  Destoys any associated constraints.
OscObject::~OscObject()
{
    constraint_list_iterator it = m_constraintList.begin();
    while (it!=m_constraintList.end()) {
        /* Remove the constraint from the other object's constraint
           list before deleting. */
        OscConstraint *o = *it;
        if (o->object2()) {
            if (this==o->object1())
                o->object2()->m_constraintList.remove(o);
            else if (this==o->object2())
                o->object1()->m_constraintList.remove(o);
        }

        (**it).on_destroy();
        it++;
    }

    ptrace(m_bTrace, ("[%s] %s.~OscObject()\n",
                      simulation()->type_str(), c_name()));
}

//! This function must be called if the object becomes linked to another object's constraint
//! so that the constraint can be destroyed if this object is destroyed.
void OscObject::linkConstraint(std::string &name)
{
	 m_constraintLinks.push_back(name);
}

//! If a linked constraint is destroyed, it must be removed from this object's linked
//! constraints list by calling this function.
void OscObject::unlinkConstraint(std::string &name)
{
	 std::vector<std::string>::iterator it=m_constraintLinks.begin();
	 for (; it<m_constraintLinks.end(); it++)
		  if ((*it)==name) m_constraintLinks.erase(it);
}

//! Set the dynamic object velocity
void OscObject::setVelocity(OscObject *me, const OscVector3& vel)
{
    me->odePrimitive()->setDynamicLinearVelocity(vel);
}

//! Set the haptic object static friction coefficient.
void OscObject::setFrictionStatic(OscObject *me, const OscScalar& value)
{
    // Note: unfortunately cMesh and cGenericPotentialField don't
    // share the same m_material field...
    cShapeSphere *sphere = dynamic_cast<cShapeSphere*>(me->chaiObject());
    if (sphere) {
        sphere->m_material.setStaticFriction(value.m_value);
        return;
    }

    cMesh *mesh = dynamic_cast<cMesh*>(me->chaiObject());
    if (mesh)
        mesh->m_material.setStaticFriction(value.m_value);
}

//! Set the haptic object dynamic friction coefficient.
void OscObject::setFrictionDynamic(OscObject *me, const OscScalar& value)
{
    cShapeSphere *sphere = dynamic_cast<cShapeSphere*>(me->chaiObject());
    if (sphere) {
        sphere->m_material.setDynamicFriction(value.m_value);
        return;
    }

    cMesh *mesh = dynamic_cast<cMesh*>(me->chaiObject());
    if (mesh)
        mesh->m_material.setDynamicFriction(value.m_value);
}

//! Set the texture file to use for this object.
void OscObject::setTextureImage(OscObject *me, const OscString& filename)
{
    // TODO: ensure same texture file doesn't need to be loaded more than once
    cTexture2D *texture, *old_texture;
    texture = new cTexture2D;
    texture->setEnvironmentMode(GL_MODULATE);
    if (!texture->loadFromFile(filename.c_str())) {
        printf("Error loading %s\n", filename.c_str());
        delete texture;
        return;
    }

    cShapeSphere *sphere = dynamic_cast<cShapeSphere*>(me->chaiObject());
    if (sphere) {
        old_texture = sphere->m_texture;
        if (old_texture)
            delete old_texture;
        sphere->m_texture = texture;
//        texture->setSphericalMappingEnabled(true);
        return;
    }

    cMesh *mesh = dynamic_cast<cMesh*>(me->chaiObject());
    if (mesh) {
        old_texture = mesh->getTexture();
        if (old_texture)
            delete old_texture;
        mesh->setTexture(texture);
        texture->setSphericalMappingEnabled(false);
    }
}

//! Update the position extracted from the dynamic simulation
void OscObject::updateDynamicPosition(const dReal* pos)
{
    m_position.set(pos[0], pos[1], pos[2]);
}

//! Update the velocity extracted from the dynamic simulation
void OscObject::updateDynamicVelocity(const dReal* vel)
{
    m_accel.set(
        m_velocity[0] - vel[0],
        m_velocity[1] - vel[1],
        m_velocity[2] - vel[2]);
    m_velocity.set(vel[0], vel[1], vel[2]);
}

//! Inform object that it is in collision with another object.
//! \return True if this is a new collision
bool OscObject::collidedWith(OscObject *o, int count)
{
    bool rc=false;
    if (m_collisions[o] != count-1) {
        rc=true;
        if (m_collide.m_value) {
            lo_send(address_send, ("/world/"+m_name+"/collide").c_str(),
                    "sf", o->c_name(),
                    (double)(m_velocity - o->m_velocity).length());
        }
    }
    m_collisions[o] = count;

    return rc;
}

//! Destroy the object
void OscObject::on_destroy()
{
    simulation()->delete_object(*this);

    /* The object's memory is freed in the above delete_object call.
     * Should it be done here instead? Or perhaps moved to a deleted
     * objects pool for later garbage collection. */

    return;
}

void OscObject::ungrab(int thread)
{
    if (thread != DIMPLE_THREAD_HAPTICS)
        return;

    if (proxyObject == this) {
        proxyObject = NULL;

        // add self back into haptics contact
        chaiObject()->setHapticEnabled(true, true);
    }
}

static float sinewave=M_PI/2;
void oscillate_callback(void* user)
{
    cODEPrimitive* ob = static_cast<cODEPrimitive*>(user);
    float force = ((sinewave > M_PI)-0.50f)*10.0f;
    //float force = sin(sinewave)*10.f;
    printf("oscillate force: %f   \r", force);
    dBodyAddForce(ob->m_odeBody, 0, force, 0);
    sinewave += 0.4;
    if (sinewave >= M_PI*2) sinewave -= M_PI*2;
}

void *oscillate_thread(void* user)
{
    float* args = (float*)user;
    OscObject *ob = ((OscObject**)user)[0];
    float hz = args[1];
    float amp = args[2];
    delete args;
    
    printf("Oscillate thread!  %s, %f, %f\n", ob->c_name(), hz, amp);

    while (1) {
        wait_ode_request(oscillate_callback, ob->odePrimitive());
    }
}

int OscObject::oscillate_handler(const char *path, const char *types, lo_arg **argv,
                                 int argc, void *data, void *user_data)
{
    handler_data *hd = (handler_data*)user_data;
	OscObject *me = (OscObject*)hd->user_data;

    if (hd->thread != DIMPLE_THREAD_HAPTICS)
        return 0;

    float hz = argv[0]->f;
    float amp = argv[1]->f;
    void **args = new void*[3];
    args[0] = (void*)me;
    args[1] = (void*)*(unsigned int*)&hz;
    args[2] = (void*)*(unsigned int*)&amp;

    pthread_t th;
    pthread_create(&th, NULL, oscillate_thread, args);
    printf("%s is oscillating at %f Hz, %f amplitude.\n", me->c_name(), hz, amp);
    return 0;
}

// ----------------------------------------------------------------------------------

class cEmptyODEObject : public cGenericObject, public cODEPrimitive
{
public:
    cEmptyODEObject(cWorld *world, dWorldID ode_world, dSpaceID ode_space)
        : cODEPrimitive(world, ode_world, ode_space) {}
};

OscComposite::OscComposite(const char *name)
    : OscObject(NULL, name)
{
    m_objChai = new cEmptyODEObject(world, ode_world, ode_space);
    printf("m_objChai: %#x\n", m_objChai);
    printf("chaiObject: %#x\n", dynamic_cast<cGenericObject*>(m_objChai));
    printf("odePrimitive(): %#x\n", dynamic_cast<cODEPrimitive*>(m_objChai));

    odePrimitive()->m_objType = DYNAMIC_OBJECT;
    odePrimitive()->m_odeBody = dBodyCreate(ode_world);
}

void OscComposite::addChild(OscObject *o)
{
    m_children.push_back(o);
    
    // add this child to the composite ODE body
    dBodyDestroy(o->odePrimitive()->m_odeBody);
    o->odePrimitive()->m_odeBody = odePrimitive()->m_odeBody;
    dGeomSetBody(o->odePrimitive()->m_odeGeom, odePrimitive()->m_odeBody);

    printf("%s added to %s\n", o->c_name(), c_name());
}

// ----------------------------------------------------------------------------------

OscPrism::OscPrism(cGenericObject* p, const char *name, OscBase* parent)
    : OscObject(p, name, parent), m_size("size", this)
{
    m_size.setSetCallback(set_size, this, DIMPLE_THREAD_PHYSICS);
}

// ----------------------------------------------------------------------------------

OscSphere::OscSphere(cGenericObject* p, const char *name, OscBase* parent)
    : OscObject(p, name, parent), m_radius("radius", this)
{
//    addHandler("radius", "f", OscSphere::radius_handler);
//    m_radius.setCallback((OscScalar::set_callback*)OscSphere::setRadius, this, DIMPLE_THREAD_PHYSICS);
    m_radius.setSetCallback(set_radius, this, DIMPLE_THREAD_PHYSICS);
}

/*
void OscSphere::setRadius(void *data, const OscScalar&)
{
    printf ("OscSphere::setRadius()\n");
    OscSphere *me = (OscSphere*)data;
    me->onSetRadius();
}
*/

//! Change the sphere's radius to the given size.
int OscSphere::radius_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data)
{
    if (argc!=1)
        return 0;

	handler_data *hd = (handler_data*)user_data;
    OscSphere* me = (OscSphere*)hd->user_data;
	cODESphere *sphere = me->odePrimitive();
    if (sphere) {
        LOCK_WORLD();
		if (hd->thread == DIMPLE_THREAD_HAPTICS)
            sphere->setRadius(argv[0]->f);
		else if (hd->thread == DIMPLE_THREAD_PHYSICS)
            sphere->setDynamicRadius(argv[0]->f);
        UNLOCK_WORLD();
    }
    return 0;
}

// ----------------------------------------------------------------------------------

OscMesh::OscMesh(cGenericObject *p, const char *name,
                 const char *filename, OscBase *parent)
    : OscObject(p, name, parent),
      m_size("size", this)
{
    m_size.setSetCallback(set_size, this, DIMPLE_THREAD_PHYSICS);

    addHandler("size", "f", OscMesh::size_handler);
    addHandler("size", "fff", OscMesh::size_handler);
}

int OscMesh::size_handler(const char *path, const char *types, lo_arg **argv,
                          int argc, void *data, void *user_data)
{
    cVector3d size;

	handler_data *hd = (handler_data*)user_data;
    OscMesh* me = (OscMesh*)hd->user_data;

    cODEMesh *odemesh = me->odePrimitive();
    cMesh *chaimesh = me->chaiObject();
    if (chaimesh && odemesh) {
        LOCK_WORLD();
        if (hd->thread == DIMPLE_THREAD_HAPTICS) {
            cVector3d boundarySize(chaimesh->getBoundaryMax() -
                                   chaimesh->getBoundaryMin());

            cVector3d scale;
            if (argc == 1) {
                float xymax = (boundarySize.x>boundarySize.y)?boundarySize.x:boundarySize.y;
                float xyzmax = (xymax>boundarySize.z)?xymax:boundarySize.z;
                float fscale = argv[0]->f / xyzmax;
                scale.set(fscale, fscale, fscale);
            }
            else if (argc == 3)
                scale.set(
                    argv[0]->f / boundarySize.x,
                    argv[1]->f / boundarySize.y,
                    argv[2]->f / boundarySize.z );

            chaimesh->scale(scale, true);
            me->m_vLastScaled = scale;
            
            printf("(haptics) Scaled %s by %f, %f, %f\n",
                   me->c_name(), scale.x, scale.y, scale.z);

            // Perform similar scaling in the physics thread
            // TODO: should be *post*, need to fix queueing first
            wait_ode_request(size_physics_callback, (cODEPrimitive*)me);
        }
        UNLOCK_WORLD();
    }
    return 0;
}

//! The scaling function to be called in the physics thread after
//! scaling has been done in the haptics thread.
void OscMesh::size_physics_callback(void *self)
{ 
    OscMesh* me = static_cast<OscMesh*>(self);
    me->odePrimitive()->scaleDynamicObject(me->m_vLastScaled.x,
                                           me->m_vLastScaled.y,
                                           me->m_vLastScaled.z);

    printf("(physics) Scaled %s by %f, %f, %f\n",
           me->c_name(), me->m_vLastScaled.x,
           me->m_vLastScaled.y,
           me->m_vLastScaled.z);
}

// ------------------------------------------------------------------------

OscResponse::OscResponse(const char* name, OscBase *parent)
    : OscBase(name, parent),
      m_stiffness("stiffness", this),
      m_damping("damping", this),
      m_offset("offset", this)
{
    m_stiffness.setSetCallback(set_stiffness, this, DIMPLE_THREAD_PHYSICS);
    m_damping.setSetCallback(set_damping, this, DIMPLE_THREAD_PHYSICS);
    m_offset.setSetCallback(set_offset, this, DIMPLE_THREAD_PHYSICS);

    addHandler("spring", "ff", OscResponse::spring_handler);
}

double OscResponse::response(double position, double velocity)
{
    // Damped spring
    return (-m_stiffness.m_value*(position-m_offset.m_value)
            -m_damping.m_value*velocity);
}

// ----------------------------------------------------------------------------------

//! OscConstraint has two CHAI/ODE object associated with it, though not owned by it. Class name = "constraint"
OscConstraint::OscConstraint(const char *name, OscBase *parent,
                             OscObject *object1, OscObject *object2)
    : OscBase(name, parent)
{
    assert(object1);
    m_object1 = object1;
    m_object2 = object2;

    if (object1) object1->m_constraintList.push_back(this);
    if (object2) object2->m_constraintList.push_back(this);

    m_stiffness = 0;
    m_damping = 0;

	// inform object2 that it is in use in a constraint
	if (object2) object2->linkConstraint(m_name);

    addHandler("destroy", "", OscConstraint::destroy_handler);
    addHandler("response/center",   "f", OscConstraint::responseCenter_handler);
    addHandler("response/constant", "f", OscConstraint::responseConstant_handler);
    addHandler("response/linear",   "f", OscConstraint::responseLinear_handler);
    addHandler("response/wall",     "ff", OscConstraint::responseWall_handler);
    addHandler("response/wall",     "ffi", OscConstraint::responseWall_handler);
    addHandler("response/pluck",    "ff", OscConstraint::responsePluck_handler);
}

//! Destroy the constraint
void OscConstraint::on_destroy()
{
    simulation()->delete_constraint(*this);

    /* The constraint's memory is freed in the above delete_object
     * call.  Should it be done here instead? Or perhaps moved to a
     * deleted objects pool for later garbage collection. */

    return;
}

int OscConstraint::responseCenter_handler(const char *path, const char *types, lo_arg **argv,
                                          int argc, void *data, void *user_data)
{
    return 0;
}

int OscConstraint::responseConstant_handler(const char *path, const char *types, lo_arg **argv,
                                            int argc, void *data, void *user_data)
{
    return 0;
}

int OscConstraint::responseLinear_handler(const char *path, const char *types, lo_arg **argv,
                                          int argc, void *data, void *user_data)
{
    if (argc!=1) return 0;

	handler_data *hd = (handler_data*)user_data;
    OscConstraint *me = (OscConstraint*)hd->user_data;
    me->m_stiffness = argv[0]->f;
    me->m_damping = 0;
    return 0;
}

int OscConstraint::responseSpring_handler(const char *path, const char *types, lo_arg **argv,
                                          int argc, void *data, void *user_data)
{
    if (argc!=2) return 0;

	handler_data *hd = (handler_data*)user_data;
    OscConstraint *me = (OscConstraint*)hd->user_data;
	if (hd->thread == DIMPLE_THREAD_HAPTICS) {
		 me->m_stiffness = argv[0]->f;
		 me->m_damping = argv[1]->f;
	}
    return 0;
}

int OscConstraint::responseWall_handler(const char *path, const char *types, lo_arg **argv,
                                        int argc, void *data, void *user_data)
{
    return 0;
}

int OscConstraint::responsePluck_handler(const char *path, const char *types, lo_arg **argv,
                                         int argc, void *data, void *user_data)
{
    return 0;
}

// ----------------------------------------------------------------------------------

//! A ball joint requires a single fixed anchor point
OscBallJoint::OscBallJoint(const char *name, OscBase *parent,
                           OscObject *object1, OscObject *object2,
                           double x, double y, double z)
    : OscConstraint(name, parent, object1, object2)
{
}

// ----------------------------------------------------------------------------------

//! A hinge requires a fixed anchor point and an axis
OscHinge::OscHinge(const char *name, OscBase* parent,
                   OscObject *object1, OscObject *object2,
                   double x, double y, double z, double ax, double ay, double az)
    : OscConstraint(name, parent, object1, object2),
      m_torque("torque", this)
{
    m_torque.setSetCallback(set_torque, this, DIMPLE_THREAD_PHYSICS);
}

// ----------------------------------------------------------------------------------

//! A hinge requires a fixed anchor point and an axis
OscHinge2::OscHinge2(const char *name, OscBase *parent,
                     OscObject *object1, OscObject *object2,
                     double x, double y, double z,
                     double ax, double ay, double az,
                     double bx, double by, double bz)
    : OscConstraint(name, parent, object1, object2)
{
}

// ----------------------------------------------------------------------------------

//! A hinge requires a fixed anchor point and an axis
OscUniversal::OscUniversal(const char *name, OscBase *parent,
                           OscObject *object1, OscObject *object2,
                           double x,   double y,   double z,
                           double a1x, double a1y, double a1z,
                           double a2x, double a2y, double a2z)
    : OscConstraint(name, parent, object1, object2)
{
}
