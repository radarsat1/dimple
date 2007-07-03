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
#include "dimple.h"
#include "valuetimer.h"
#include <assert.h>

//! OscBase objects always have a name. Class name defaults to "".
OscBase::OscBase(const char *name, const char *classname)
    : m_name(name), m_classname(classname)
{
}

//! Add a handler for some OSC method
void OscBase::addHandler(const char *methodname, const char* type, lo_method_handler h)
{
    // build OSC method name
    std::string n("/");
    if (m_classname.length()>0)
        n += m_classname + "/";
    n += m_name;
    if (strlen(methodname)>0)
        n = n + "/" + methodname;

    // add it to liblo server and store it
    if (lo_server_thread_add_method(loserver, n.c_str(), type, h, this))
    {
        method_t m;
        m.name = n;
        m.type = type;
        m_methods.push_back(m);
    }
}

OscBase::~OscBase()
{
    // remove all stored OSC methods from the liblo server
    while (m_methods.size()>0) {
        method_t m = m_methods.back();
        m_methods.pop_back();
        lo_server_thread_del_method(loserver, m.name.c_str(), m.type.c_str());
    }
}

// ----------------------------------------------------------------------------------

OscValue::OscValue(const char *name, const char *classname)
  : OscBase(name, classname)
{
    m_callback = NULL;
    m_callback_data = NULL;
    m_callback_thread = DIMPLE_THREAD_PHYSICS;
}

OscValue::~OscValue()
{
    valuetimer.removeValue(this);
}

int OscValue::get_handler(const char *path, const char *types, lo_arg **argv,
                          int argc, void *data, void *user_data)
{
    handler_data *hd = (handler_data*)user_data;
    OscValue *me = (OscValue*)hd->user_data;
    
    if (hd->thread != DIMPLE_THREAD_PHYSICS)
        return 0;

    if (argc==0) {
        me->send();
    }

    else if (argc==1) {
        if (argv[0]->i == 0)
            valuetimer.removeValue(me);
        else
            valuetimer.addValue(me, argv[0]->i);
    }

    return 0;
}

// ----------------------------------------------------------------------------------

OscScalar::OscScalar(const char *name, const char *classname)
	 : OscValue(name, classname)
{
    m_callback = NULL;
    m_callback_data = NULL;
    
    m_value = 0;
	
    addHandler("",              "f", OscScalar::_handler);
    addHandler("get",           "i", OscScalar::get_handler);
    addHandler("get",           "" , OscScalar::get_handler);
}

void OscScalar::set(double value)
{
	 m_value = value;
	 setChanged();
}

void OscScalar::send()
{
    lo_send(address_send, ("/" + m_classname +
                           "/" + m_name).c_str(),
            "f", m_value
        );
}

int OscScalar::_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
	 handler_data *hd = (handler_data*)user_data;
	 OscScalar *me = (OscScalar*)hd->user_data;

     if (hd->thread != me->m_callback_thread)
         return 0;

	 if (argc == 1)
		  me->m_value = argv[0]->f;

     if (me->m_callback)
         me->m_callback(me->m_callback_data, *me);

	 return 0;
}

// ----------------------------------------------------------------------------------

//! OscVector3 is a 3-vector which can report its magnitude.
OscVector3::OscVector3(const char *name, const char *classname)
    : OscValue(name, classname),
	  m_magnitude("magnitude", (std::string(classname)+"/"+name).c_str()),
      cVector3d()
{
    m_callback = NULL;
    m_callback_data = NULL;
    m_magnitude.setCallback((OscScalar::set_callback*)OscVector3::setMagnitude, this,
        DIMPLE_THREAD_PHYSICS);

    addHandler("",              "fff", OscVector3::_handler);
    addHandler("get",           "i"  , OscVector3::get_handler);
    addHandler("get",           ""   , OscVector3::get_handler);
}

void OscVector3::setChanged()
{
	 // TODO: this would be more efficient if it was only
	 // calculated when asked for, but only the first time
     // if it hasn't changed.
	 m_magnitude.set(sqrt(x*x + y*y + z*z));
}

void OscVector3::set(double _x, double _y, double _z)
{
    x = _x;
    y = _y;
    z = _z;
    setChanged();
}

void OscVector3::send()
{
    lo_send(address_send, ("/" + m_classname +
                           "/" + m_name).c_str(),
            "fff", x, y, z
        );
}

void OscVector3::setMagnitude(OscVector3 *me, const OscScalar& s)
{
    double ratio;
    if (me->x==0 && me->y==0 && me->z==0)
        ratio = 0;
    else
        ratio = s.m_value / sqrt(me->x*me->x + me->y*me->y + me->z*me->z);
    
    *me *= ratio;

    if (me->m_callback)
        me->m_callback(me->m_callback_data, *me);
}

int OscVector3::_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
	 handler_data *hd = (handler_data*)user_data;
	 OscVector3 *me = (OscVector3*)hd->user_data;

     if (hd->thread != me->m_callback_thread)
         return 0;

	 if (argc == 3) {
		  me->x = argv[0]->f;
		  me->y = argv[1]->f;
		  me->z = argv[2]->f;
          me->setChanged();
	 }
     
     if (me->m_callback)
         me->m_callback(me->m_callback_data, *me);

	 return 0;
}

// ----------------------------------------------------------------------------------

//! OscString is an OSC-accessible and -settable string value.
OscString::OscString(const char *name, const char *classname)
    : OscValue(name, classname)
{
    addHandler("",              "s",   OscString::_handler);
    addHandler("get",           "i"  , OscString::get_handler);
    addHandler("get",           ""   , OscString::get_handler);
}

void OscString::send()
{
    lo_send(address_send, ("/" + m_classname +
                           "/" + m_name).c_str(),
            "s", c_str()
        );
}

int OscString::_handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data)
{
	 handler_data *hd = (handler_data*)user_data;
	 OscString *me = (OscString*)hd->user_data;

     if (hd->thread != me->m_callback_thread)
         return 0;

	 if (argc == 1) {
         me->assign(&argv[0]->s);
         me->setChanged();
	 }
     
     if (me->m_callback)
         me->m_callback(me->m_callback_data, *me);

	 return 0;
}

// ----------------------------------------------------------------------------------

//! OscObject has a CHAI/ODE object associated with it. Class name = "object"
OscObject::OscObject(cGenericObject* p, const char *name)
    : OscBase(name, "object"),
      m_velocity("velocity", (std::string("object/")+name).c_str()),
      m_accel("acceleration", (std::string("object/")+name).c_str()),
      m_position("position", (std::string("object/")+name).c_str()),
      m_color("color", (std::string("object/")+name).c_str()),
      m_friction_static("friction/static", (std::string("object/")+name).c_str()),
      m_friction_dynamic("friction/dynamic", (std::string("object/")+name).c_str()),
      m_texture_image("texture/image", (std::string("object/")+name).c_str())
{
    // Track pointer for ODE/Chai object
    m_objChai = p;

    // Set user data to point to this object for ODE geom, so that
    // we can identify this OscObject during collision detction
    odePrimitive()->setGeomData(this);

    // Create handlers for OSC messages
    addHandler("destroy"    , ""   , OscObject::destroy_handler);
    addHandler("mass"       , "f"  , OscObject::mass_handler);
    addHandler("force"      , "fff", OscObject::force_handler);
    addHandler("collide/get", ""   , OscObject::collideGet_handler);
    addHandler("collide/get", "i"  , OscObject::collideGet_handler);
    addHandler("grab"       , ""   , OscObject::grab_handler);
    addHandler("grab"       , "i"  , OscObject::grab_handler);

    // Set initial physical properties
    m_accel.set(0,0,0);
    m_velocity.set(0,0,0);
    m_position.set(0,0,0);

    // Sane friction defaults
    m_friction_static.set(1);
    m_friction_dynamic.set(0.5);

    // Set callbacks for when values change
    m_position.setCallback((OscVector3::set_callback*)OscObject::setPosition, this,
                           DIMPLE_THREAD_PHYSICS);
    m_velocity.setCallback((OscVector3::set_callback*)OscObject::setVelocity, this,
                           DIMPLE_THREAD_PHYSICS);
    m_color.setCallback((OscVector3::set_callback*)OscObject::setColor, this,
                        DIMPLE_THREAD_HAPTICS);
    m_friction_static.setCallback((OscScalar::set_callback*)OscObject::setFrictionStatic, this,
                                  DIMPLE_THREAD_HAPTICS);
    m_friction_dynamic.setCallback((OscScalar::set_callback*)OscObject::setFrictionDynamic, this,
                                   DIMPLE_THREAD_HAPTICS);
    m_texture_image.setCallback((OscScalar::set_callback*)OscObject::setTextureImage, this,
                                DIMPLE_THREAD_HAPTICS);

    m_getCollide = false;

    // If the new object is supposed to be a part of a
    // composite object, find it and join.
	const char *s;
    if (s=strchr(name, '/')) {
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

//! OscObject destructor is responsible for deleting the object from the CHAI world.
//! ODE removal is taken care of in the cODEPrimitive destructor.
//! Any constraints associated with the object are destroyed as well.
OscObject::~OscObject()
{
    // Destroy any constraints associated with the object
    while (odePrimitive()->m_Joint.size() > 0) {
        std::string jointname = odePrimitive()->m_Joint.begin()->first;
        if (world_constraints.find(jointname)!=world_constraints.end())
            delete world_constraints[jointname];
        world_constraints.erase(jointname);
    }
    
    // Destroy any constraints to which this object is linked
    while (m_constraintLinks.size() > 0)
    {
        std::string jointname = m_constraintLinks.back();
        m_constraintLinks.pop_back();
        
        if (world_constraints.find(jointname)!=world_constraints.end())
            delete world_constraints[jointname];
        world_constraints.erase(jointname);
    }
    
    // Remove object from CHAI world
    cGenericObject *p = m_objChai->getParent();
    p->deleteChild(m_objChai);
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

//! Set the dynamic object to a new position
void OscObject::setPosition(OscObject *me, const OscVector3& pos)
{
    me->odePrimitive()->setDynamicPosition(pos);
}

//! Set the dynamic object velocity
void OscObject::setVelocity(OscObject *me, const OscVector3& vel)
{
    me->odePrimitive()->setDynamicLinearVelocity(vel);
}

//! Set the graphical object color
void OscObject::setColor(OscObject *me, const OscVector3& color)
{
    cShapeSphere *sphere = dynamic_cast<cShapeSphere*>(me->chaiObject());
    if (sphere) {
        sphere->m_material.m_diffuse.set(color.x, color.y, color.z);
        return;
    }

    cMesh *mesh = dynamic_cast<cMesh*>(me->chaiObject());
    if (mesh)
        mesh->m_material.m_diffuse.set(color.x, color.y, color.z);
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
        texture->setSphericalMappingEnabled(true);
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
    m_position[0] = pos[0];
    m_position[1] = pos[1];
    m_position[2] = pos[2];
	m_position.setChanged();
}

//! Update the velocity extracted from the dynamic simulation
void OscObject::updateDynamicVelocity(const dReal* vel)
{
    m_accel[0] = m_velocity[0] - vel[0];
    m_accel[1] = m_velocity[1] - vel[1];
    m_accel[2] = m_velocity[2] - vel[2];
	m_accel.setChanged();
    m_velocity[0] = vel[0];
    m_velocity[1] = vel[1];
    m_velocity[2] = vel[2];
	m_velocity.setChanged();
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
                    "s", o->name());
            // TODO: send collision force
        }
    }
    m_collisions[o] = ode_counter;

    return rc;
}

//! Destroy the object
int OscObject::destroy_handler(const char *path, const char *types, lo_arg **argv,
							   int argc, void *data, void *user_data)
{
	 handler_data *hd = (handler_data*)user_data;
	 OscObject *me = (OscObject*)hd->user_data;

     if (hd->thread != DIMPLE_THREAD_PHYSICS)
         return 0;

    if (me) {
        LOCK_WORLD();
        world_objects.erase(me->m_name);
        delete me;
        UNLOCK_WORLD();
    }
    return 0;
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

//! Add an instantaneous force to an object
int OscObject::force_handler(const char *path, const char *types, lo_arg **argv,
                             int argc, void *data, void *user_data)
{
    if (argc!=3) return 0;
    LOCK_WORLD();
	handler_data *hd = (handler_data*)user_data;
    OscObject *me = (OscObject*)hd->user_data;
	if (hd->thread == DIMPLE_THREAD_PHYSICS)
        me->odePrimitive()->setDynamicForce(cVector3d(argv[0]->f, argv[1]->f, argv[2]->f));
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
    printf("Disabled haptics for object %s: %d\n", me->name(), me->chaiObject()->getHapticEnabled());

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

    printf("%s added to %s\n", o->name(), name());
}

// ----------------------------------------------------------------------------------

OscPrism::OscPrism(cGenericObject* p, const char *name)
    : OscObject(p, name)
{
    addHandler("size", "fff", OscPrism::size_handler);
}

//! Resize the prism to the given dimensions.
int OscPrism::size_handler(const char *path, const char *types, lo_arg **argv,
                           int argc, void *data, void *user_data)
{
    if (argc!=3)
        return 0;

	handler_data *hd = (handler_data*)user_data;
    OscPrism* me = (OscPrism*)hd->user_data;
	cODEPrism *prism = me->odePrimitive();
	if (prism)
	{
		 cVector3d size;
		 size.x = argv[0]->f;
		 size.y = argv[1]->f;
		 size.z = argv[2]->f;
		 LOCK_WORLD();
		 if (hd->thread == DIMPLE_THREAD_HAPTICS)
			  prism->setSize(size);
		 else if (hd->thread == DIMPLE_THREAD_PHYSICS)
			  prism->setDynamicSize(size);
		 UNLOCK_WORLD();
	}

	return 0;
}

// ----------------------------------------------------------------------------------

OscSphere::OscSphere(cGenericObject* p, const char *name)
    : OscObject(p, name)
{
    addHandler("radius", "f", OscSphere::radius_handler);
}

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

//! OscConstraint has two CHAI/ODE object associated with it, though not owned by it. Class name = "constraint"
OscConstraint::OscConstraint(const char *name, OscObject *object1, OscObject *object2)
    : OscBase(name, "constraint")
{
    assert(object1);
    m_object1 = object1;
    m_object2 = object2;

    m_stiffness = 0;
    m_damping = 0;

	// inform object2 that it is in use in a constraint
	if (object2) object2->linkConstraint(m_name);

    addHandler("destroy", "", OscConstraint::destroy_handler);
    addHandler("response/linear", "f", OscConstraint::responseLinear_handler);
    addHandler("response/spring", "ff", OscConstraint::responseSpring_handler);
}

OscConstraint::~OscConstraint()
{
    if (!m_object1) return;
    m_object1->odePrimitive()->destroyJoint(m_name);
	if (m_object2) m_object2->unlinkConstraint(m_name);
	printf("Constraint %s destroyed.\n", m_name.c_str());
}

int OscConstraint::destroy_handler(const char *path, const char *types, lo_arg **argv,
							   int argc, void *data, void *user_data)
{
	 handler_data *hd = (handler_data*)user_data;
	 OscConstraint *me = (OscConstraint*)hd->user_data;

    if (me) {
        LOCK_WORLD();
        world_constraints.erase(me->m_name);
        delete me;
        UNLOCK_WORLD();
    }
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

// ----------------------------------------------------------------------------------

//! A ball joint requires a single fixed anchor point
OscBallJoint::OscBallJoint(const char *name, OscObject *object1, OscObject *object2,
                           double x, double y, double z)
    : OscConstraint(name, object1, object2)
{
	// create the constraint for object1
    cVector3d anchor(x,y,z);
    object1->odePrimitive()->ballLink(name, object2?object2->odePrimitive():NULL, anchor);

    printf("Ball link created between %s and %s at (%f,%f,%f)\n",
		   object1->name(), object2?object2->name():"world", x,y,z);
}

// ----------------------------------------------------------------------------------

//! A hinge requires a fixed anchor point and an axis
OscHinge::OscHinge(const char *name, OscObject *object1, OscObject *object2,
                   double x, double y, double z, double ax, double ay, double az)
    : OscConstraint(name, object1, object2)
{
	// create the constraint for object1
    cVector3d anchor(x,y,z);
    cVector3d axis(ax,ay,az);
    object1->odePrimitive()->hingeLink(name, object2?object2->odePrimitive():NULL, anchor, axis);

    addHandler("force/magnitude/get", "i", OscHinge::forceMagnitudeGet_handler);
    addHandler("force/magnitude/get", "", OscHinge::forceMagnitudeGet_handler);

    printf("Hinge joint created between %s and %s at anchor (%f,%f,%f), axis (%f,%f,%f)\n",
        object1->name(), object2?object2->name():"world", x,y,z,ax,ay,az);
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
    m_torque = -m_stiffness*angle - m_damping*rate;
    dJointAddHingeTorque(*id, m_torque);
}

int OscHinge::forceMagnitudeGet_handler(const char *path, const char *types, lo_arg **argv,
                                        int argc, void *data, void *user_data)
{
    bool once=true;
    int interval=0;
    if (argc>0) {
        interval = argv[0]->i;
        once=false;
    }

	handler_data *hd = (handler_data*)user_data;
    OscHinge *me = (OscHinge*)hd->user_data;
    if (once && hd->thread == DIMPLE_THREAD_HAPTICS) {
        lo_send(address_send, ("/constraint/"+me->m_name+"/force/magnitude").c_str(), "f", me->m_torque);
    }

    return 0;
}

// ----------------------------------------------------------------------------------

//! A hinge requires a fixed anchor point and an axis
OscHinge2::OscHinge2(const char *name, OscObject *object1, OscObject *object2,
                     double x, double y, double z,
                     double ax, double ay, double az,
                     double bx, double by, double bz)
    : OscConstraint(name, object1, object2)
{
	// create the constraint for object1
    cVector3d anchor(x,y,z);
    cVector3d axis1(ax,ay,az);
    cVector3d axis2(bx,by,bz);
    object1->odePrimitive()->hinge2Link(name, object2?object2->odePrimitive():NULL, anchor, axis1, axis2);

    printf("Hinge2 joint created between %s and %s at anchor (%f,%f,%f), axis1 (%f,%f,%f), axis2 (%f,%f,%f)\n",
        object1->name(), object2?object2->name():"world", x,y,z,ax,ay,az,bx,by,bz);
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
    : OscConstraint(name, object1, object2)
{
	// create the constraint for object1
    cVector3d anchor(x,y,z);
    cVector3d axis1(ax,ay,az);
    cVector3d axis2(bx,by,bz);
    object1->odePrimitive()->universalLink(name, object2?object2->odePrimitive():NULL, anchor, axis1, axis2);

    printf("Universal joint created between %s and %s at anchor (%f,%f,%f), axis1 (%f,%f,%f), axis2 (%f,%f,%f)\n",
        object1->name(), object2?object2->name():"world", x,y,z,ax,ay,az,bx,by,bz);
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
    : OscConstraint(name, object1, object2)
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
           object1->name(), object2?object2->name():"world");
}
