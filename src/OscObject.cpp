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

OscValue::OscValue(const char *name, OscBase *parent)
    : OscBase(name, parent)
{
    m_set_callback = NULL;
    m_set_callback_data = NULL;
    m_set_callback_thread = DIMPLE_THREAD_PHYSICS;
    m_get_callback = NULL;
    m_get_callback_data = NULL;
    m_get_callback_thread = DIMPLE_THREAD_PHYSICS;

    addHandler("get",           "i"  , OscValue::get_handler);
    addHandler("get",           ""   , OscValue::get_handler);
}

OscValue::~OscValue()
{
    valuetimer.removeValue(this);
}

int OscValue::get_handler(const char *path, const char *types, lo_arg **argv,
                          int argc, void *data, void *user_data)
{
    OscValue *me = (OscValue*)user_data;
    
    if (me->m_get_callback) {
        me->m_get_callback(me->m_get_callback_data, *me, (argc==1)?argv[0]->i:-1);
        return 0;
    }

    if (argc==0) {
        me->send();
    }

    else if (argc==1) {
        if (argv[0]->i == 0)
            me->simulation()->valuetimer().removeValue(me);
        else if (argv[0]->i > 0)
            me->simulation()->valuetimer().addValue(me, argv[0]->i);
    }

    return 0;
}

// ----------------------------------------------------------------------------------

OscScalar::OscScalar(const char *name, OscBase *owner)
	 : OscValue(name, owner)
{
    m_value = 0;
    addHandler("",              "f", OscScalar::_handler);
}

void OscScalar::set(double value)
{
	 m_value = value;
     if (m_set_callback)
         m_set_callback(m_set_callback_data, *this);
}

void OscScalar::send()
{
    lo_send(address_send, ("/" + m_parent->name() +
                           "/" + m_name).c_str(),
            "f", m_value
        );
}

int OscScalar::_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
    OscScalar *me = static_cast<OscScalar*>(user_data);

	 if (argc == 1)
		  me->m_value = argv[0]->f;

     if (me->m_set_callback)
         me->m_set_callback(me->m_set_callback_data, *me);

	 return 0;
}

// ----------------------------------------------------------------------------------

//! OscVector3 is a 3-vector which can report its magnitude.
OscVector3::OscVector3(const char *name, OscBase *owner)
    : OscValue(name, owner),
	  m_magnitude("magnitude", this),
      cVector3d()
{
    m_magnitude.setSetCallback((OscScalar::SetCallback*)set_magnitude_callback, this, DIMPLE_THREAD_PHYSICS);

    addHandler("",              "fff", OscVector3::_handler);
}

void OscVector3::set(double _x, double _y, double _z)
{
    x = _x;
    y = _y;
    z = _z;
    m_magnitude.set(sqrt(x*x + y*y + z*z));
    if (m_set_callback)
        m_set_callback(m_set_callback_data, *this);
}

void OscVector3::send()
{
    lo_send(address_send, ("/" + m_parent->name() +
                           "/" + m_name).c_str(),
            "fff", x, y, z
        );
}

void OscVector3::set_magnitude_callback(OscVector3 *me, OscScalar& s)
{
    double ratio;
    if (me->x==0 && me->y==0 && me->z==0)
        ratio = 0;
    else
        ratio = s.m_value / sqrt(me->x*me->x + me->y*me->y + me->z*me->z);
    
    *me *= ratio;

    if (me->m_set_callback)
        me->m_set_callback(me->m_set_callback_data, *me);
}

int OscVector3::_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
    OscVector3 *me = static_cast<OscVector3*>(user_data);

	 if (argc != 3)
         return 0;

     me->x = argv[0]->f;
     me->y = argv[1]->f;
     me->z = argv[2]->f;
     me->m_magnitude.m_value = sqrt(me->x*me->x + me->y*me->y + me->z*me->z);

     if (me->m_set_callback)
         me->m_set_callback(me->m_set_callback_data, *me);
     
	 return 0;
}

// ----------------------------------------------------------------------------------

//! OscMatrix3 is a 3-matrix which can report its magnitude.
OscMatrix3::OscMatrix3(const char *name, OscBase *owner)
    : OscValue(name, owner),
      cMatrix3d()
{
    addHandler("",              "fffffffff", OscMatrix3::_handler);
}

void OscMatrix3::set(double m00, double m01, double m02,
                     double m10, double m11, double m12,
                     double m20, double m21, double m22)
{
    cMatrix3d::set(m00, m01, m02, m10, m11, m12, m20, m21, m22);
    if (m_set_callback)
        m_set_callback(m_set_callback_data, *this);
}

void OscMatrix3::send()
{
    lo_send(address_send, ("/" + m_parent->name() +
                           "/" + m_name).c_str(),
            "fffffffff",
            m[0][0], m[0][1], m[0][2],
            m[1][0], m[1][1], m[1][2],
            m[2][0], m[2][1], m[2][2]
        );
}

int OscMatrix3::_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
    OscMatrix3 *me = static_cast<OscMatrix3*>(user_data);

	 if (argc != 9)
         return 0;

     me->set(argv[0]->f, argv[1]->f, argv[2]->f,
             argv[3]->f, argv[4]->f, argv[5]->f,
             argv[6]->f, argv[7]->f, argv[8]->f);

     if (me->m_set_callback)
         me->m_set_callback(me->m_set_callback_data, *me);
     
	 return 0;
}

// ----------------------------------------------------------------------------------

//! OscString is an OSC-accessible and -settable string value.
OscString::OscString(const char *name, OscBase *owner)
    : OscValue(name, owner)
{
    addHandler("",              "s",   OscString::_handler);
//    addHandler("get",           "i"  , OscString::get_handler);
//    addHandler("get",           ""   , OscString::get_handler);
}

void OscString::send()
{
    lo_send(address_send, ("/" + m_parent->name() +
                           "/" + m_name).c_str(),
            "s", c_str()
        );
}

void OscString::set(const std::string& s)
{
    assign(s);
    if (m_set_callback)
        m_set_callback(m_set_callback_data, *this);
}

void OscString::set(const char* s)
{
    assign(s);
    if (m_set_callback)
        m_set_callback(m_set_callback_data, *this);
}

int OscString::_handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data)
{
    OscString *me = static_cast<OscString*>(user_data);

	 if (argc == 1) {
         me->assign(&argv[0]->s);
	 }
     
     if (me->m_set_callback)
         me->m_set_callback(me->m_set_callback_data, *me);

	 return 0;
}

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
      m_rotation("rotation", this)
{
    // Track pointer for ODE/Chai object
    m_objChai = p;

    // Set user data to point to this object for ODE geom, so that
    // we can identify this OscObject during collision detction
    if (odePrimitive())
        odePrimitive()->setGeomData(this);

    // Create handlers for OSC messages
    addHandler("destroy"    , ""   , OscObject::destroy_handler);
    addHandler("mass"       , "f"  , OscObject::mass_handler);
    addHandler("collide/get", ""   , OscObject::collideGet_handler);
    addHandler("collide/get", "i"  , OscObject::collideGet_handler);
    addHandler("grab"       , ""   , OscObject::grab_handler);
    addHandler("grab"       , "i"  , OscObject::grab_handler);
    addHandler("oscillate"  , "ff" , OscObject::oscillate_handler);

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

    m_getCollide = false;

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

//! OscObject destructor.
OscObject::~OscObject()
{
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
bool OscObject::collidedWith(OscObject *o)
{
    bool rc=false;
    if (m_collisions[o] != ode_counter-1) {
        rc=true;
        if (m_getCollide) {
            lo_send(address_send, ("/object/"+m_name+"/collide").c_str(),
                    "s", o->c_name());
            // TODO: send collision force
        }
    }
    m_collisions[o] = ode_counter;

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

//! Set the object's mass
int OscObject::mass_handler(const char *path, const char *types, lo_arg **argv,
                             int argc, void *data, void *user_data)
{
    if (argc!=1) return 0;

    LOCK_WORLD();
	handler_data *hd = (handler_data*)user_data;
    OscObject *me = (OscObject*)hd->user_data;
	if (hd->thread == DIMPLE_THREAD_PHYSICS)
		 me->odePrimitive()->setDynamicMass(argv[0]->f);
    UNLOCK_WORLD();
    return 0;
}

int OscObject::collideGet_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
	 handler_data *hd = (handler_data*)user_data;
	 OscObject *me = (OscObject*)hd->user_data;

    int interval=-1;
    if (argc > 0) {
        interval = argv[0]->i;
    }
    me->m_getCollide = (interval>0);
    // TODO: only report one collision if multiple collisions occur within given interval

    return 0;
}

int OscObject::grab_handler(const char *path, const char *types, lo_arg **argv,
                            int argc, void *data, void *user_data)
{
    handler_data *hd = (handler_data*)user_data;
	OscObject *me = (OscObject*)hd->user_data;

    if (hd->thread != DIMPLE_THREAD_HAPTICS)
        return 0;

    if (proxyObject)
        proxyObject->ungrab(hd->thread);

    if (argc == 1 && argv[0]->i == 0)
        return 0;

    // remove self from haptics contact
    me->chaiObject()->setHapticEnabled(false, true);
    printf("Disabled haptics for object %s: %d\n", me->c_name(), me->chaiObject()->getHapticEnabled());

    // become the proxy object
    proxyObject = me;

    return 0;
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

OscMesh::OscMesh(cGenericObject* p, const char *name)
    : OscObject(p, name)
{
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

// ----------------------------------------------------------------------------------

//! OscConstraint has two CHAI/ODE object associated with it, though not owned by it. Class name = "constraint"
OscConstraint::OscConstraint(const char *name, OscBase *parent,
                             OscObject *object1, OscObject *object2)
    : OscBase(name, parent)
{
    assert(object1);
    m_object1 = object1;
    m_object2 = object2;

    m_stiffness = 0;
    m_damping = 0;

	// inform object2 that it is in use in a constraint
	if (object2) object2->linkConstraint(m_name);

    addHandler("destroy", "", OscConstraint::destroy_handler);
    addHandler("response/center",   "f", OscConstraint::responseCenter_handler);
    addHandler("response/constant", "f", OscConstraint::responseConstant_handler);
    addHandler("response/linear",   "f", OscConstraint::responseLinear_handler);
    addHandler("response/spring",   "ff", OscConstraint::responseSpring_handler);
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
OscBallJoint::OscBallJoint(const char *name, OscObject *object1, OscObject *object2,
                           double x, double y, double z)
    : OscConstraint(name, NULL, object1, object2)
{
	// create the constraint for object1
    cVector3d anchor(x,y,z);
    object1->odePrimitive()->ballLink(name, object2?object2->odePrimitive():NULL, anchor);

    printf("Ball link created between %s and %s at (%f,%f,%f)\n",
		   object1->c_name(), object2?object2->c_name():"world", x,y,z);
}

// ----------------------------------------------------------------------------------

//! A hinge requires a fixed anchor point and an axis
OscHinge::OscHinge(const char *name, OscBase* parent,
                   OscObject *object1, OscObject *object2,
                   double x, double y, double z, double ax, double ay, double az)
    : OscConstraint(name, parent, object1, object2),
      m_torque("torque", this)
{
}

//! This function is called once per simulation step, allowing the
//! constraint to be "motorized" according to some response.
//! It runs in the physics thread.
void OscHinge::simulationCallback()
{
    dJointID *id;
    if (!m_object1->odePrimitive()->getJoint(m_name, id))
        return;

    dReal angle = dJointGetHingeAngle(*id);
    dReal rate = dJointGetHingeAngleRate(*id);
    m_torque.set(-m_stiffness*angle - m_damping*rate);
    dJointAddHingeTorque(*id, m_torque.m_value);
}

// ----------------------------------------------------------------------------------

//! A hinge requires a fixed anchor point and an axis
OscHinge2::OscHinge2(const char *name, OscObject *object1, OscObject *object2,
                     double x, double y, double z,
                     double ax, double ay, double az,
                     double bx, double by, double bz)
    : OscConstraint(name, NULL, object1, object2)
{
	// create the constraint for object1
    cVector3d anchor(x,y,z);
    cVector3d axis1(ax,ay,az);
    cVector3d axis2(bx,by,bz);
    object1->odePrimitive()->hinge2Link(name, object2?object2->odePrimitive():NULL, anchor, axis1, axis2);

    printf("Hinge2 joint created between %s and %s at anchor (%f,%f,%f), axis1 (%f,%f,%f), axis2 (%f,%f,%f)\n",
        object1->c_name(), object2?object2->c_name():"world", x,y,z,ax,ay,az,bx,by,bz);
}

//! This function is called once per simulation step, allowing the
//! constraint to be "motorized" according to some response.
//! It runs in the haptics thread.
void OscHinge2::simulationCallback()
{
    dJointID *id;
    if (!m_object1->odePrimitive()->getJoint(m_name, id))
        return;

    // TODO: This will present difficulties until dJointGetHinge2Angle2 is defined in ODE
    dReal angle = dJointGetHinge2Angle1(*id);
    dReal rate = dJointGetHinge2Angle1Rate(*id);
    dJointAddHinge2Torques(*id, m_stiffness*angle - m_damping*rate, 0);
}

// ----------------------------------------------------------------------------------

//! A hinge requires a fixed anchor point and an axis
OscUniversal::OscUniversal(const char *name, OscObject *object1, OscObject *object2,
                           double x, double y, double z,
                           double ax, double ay, double az,
                           double bx, double by, double bz)
    : OscConstraint(name, NULL, object1, object2)
{
	// create the constraint for object1
    cVector3d anchor(x,y,z);
    cVector3d axis1(ax,ay,az);
    cVector3d axis2(bx,by,bz);
    object1->odePrimitive()->universalLink(name, object2?object2->odePrimitive():NULL, anchor, axis1, axis2);

    printf("Universal joint created between %s and %s at anchor (%f,%f,%f), axis1 (%f,%f,%f), axis2 (%f,%f,%f)\n",
        object1->c_name(), object2?object2->c_name():"world", x,y,z,ax,ay,az,bx,by,bz);
}

//! This function is called once per simulation step, allowing the
//! constraint to be "motorized" according to some response.
//! It runs in the haptics thread.
void OscUniversal::simulationCallback()
{
    dJointID *id;
    if (!m_object1->odePrimitive()->getJoint(m_name, id))
        return;

    // TODO: This will present difficulties until dJointGetHinge2Angle2 is defined in ODE
    dReal angle1 = dJointGetUniversalAngle1(*id);
    dReal angle2 = dJointGetUniversalAngle2(*id);
    dReal rate1 = dJointGetUniversalAngle1Rate(*id);
    dReal rate2 = dJointGetUniversalAngle2Rate(*id);

    dJointAddUniversalTorques(*id,
        -m_stiffness*angle1 - m_damping*rate1,
        -m_stiffness*angle2 - m_damping*rate2);
}

// ----------------------------------------------------------------------------------

//! A fixed joint requires only an anchor point
OscFixed::OscFixed(const char *name, OscObject *object1, OscObject *object2)
    : OscConstraint(name, NULL, object1, object2)
{
	// create the constraint for object1
    if (object2)
        object1->odePrimitive()->fixedLink(name, object2?object2->odePrimitive():NULL);

    // if object2 is world, then simply make the object have no "body"
    // this ensures it will not react to any interaction, and is thus fixed in place
    // (other objects will still collide with it, but it won't budge.)
    if (!object2) {
        object1->odePrimitive()->removeBody();
        
        // track this constraint
        std::string s(name);
        object1->linkConstraint(s);
    }

    printf("Fixed joint created between %s and %s\n",
           object1->c_name(), object2?object2->c_name():"world");
}

