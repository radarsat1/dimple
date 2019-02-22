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
      m_density("density", this),
      m_collide("collide", this),
      m_visible("visible", this)
{
    m_pSpecial = NULL;

    // Create handlers for OSC messages
    addHandler("destroy"    , ""   , OscObject::destroy_handler);
    addHandler("grab"       , ""   , OscObject::grab_handler);
    addHandler("grab"       , "i"  , OscObject::grab_handler);

    // Set initial physical properties
    m_accel.setValue(0,0,0);
    m_velocity.setValue(0,0,0);
    m_position.setValue(0,0,0);
    m_force.setValue(0,0,0);
    m_density.setValue(100);
    m_visible.setValue(true);

    // Sane friction defaults
    m_friction_static.setValue(1);
    m_friction_dynamic.setValue(0.5);

    // Set callbacks for when values change
    m_position.setSetCallback(set_position, this);
    m_rotation.setSetCallback(set_rotation, this);
    m_force.setSetCallback(set_force, this);
    m_color.setSetCallback(set_color, this);
    m_velocity.setSetCallback(set_velocity, this);
    m_accel.setSetCallback(set_accel, this);
    m_friction_static.setSetCallback(set_friction_static, this);
    m_friction_dynamic.setSetCallback(set_friction_dynamic, this);
    m_mass.setSetCallback(set_mass, this);
    m_density.setSetCallback(set_density, this);
    m_collide.setSetCallback(set_collide, this);
    m_visible.setSetCallback(set_visible, this);

    // If the new object is supposed to be a part of a
    // composite object, find it and join.
#if 0 // TODO
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
#endif
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

    if (m_pSpecial) delete m_pSpecial;

    ptrace(m_bTrace, ("[%s] %s.~OscObject()\n",
                      simulation()->type_str(), c_name()));
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

OscPrism::OscPrism(cGenericObject* p, const char *name, OscBase* parent)
    : OscObject(p, name, parent), m_size("size", this)
{
    m_size.setSetCallback(set_size, this);
    cVector3d(0.01, 0.01, 0.01).copyto(m_size);
}

OscSphere::OscSphere(cGenericObject* p, const char *name, OscBase* parent)
    : OscObject(p, name, parent), m_radius("radius", this)
{
    m_radius.setSetCallback(set_radius, this);
    m_radius.m_value = 0.01;
}

OscMesh::OscMesh(cGenericObject *p, const char *name,
                 const char *filename, OscBase *parent)
    : OscObject(p, name, parent),
      m_size("size", this)
{
    m_size.setSetCallback(set_size, this);
}

OscCamera::OscCamera(const char *name, OscBase *parent)
    : OscBase(name, parent),
      m_position("position", this),
      m_lookat("lookat", this),
      m_up("up", this)
{
    m_position.setSetCallback(set_position, this);
    m_lookat.setSetCallback(set_lookat, this);
    m_up.setSetCallback(set_up, this);
}

OscResponse::OscResponse(const char* name, OscBase *parent)
    : OscBase(name, parent),
      m_stiffness("stiffness", this),
      m_damping("damping", this),
      m_offset("offset", this)
{
    m_stiffness.setSetCallback(set_stiffness, this);
    m_damping.setSetCallback(set_damping, this);
    m_offset.setSetCallback(set_offset, this);

    addHandler("spring", "ff", OscResponse::spring_handler);
}

double OscResponse::response(double position, double velocity)
{
    // Damped spring
    return (-m_stiffness.m_value*(position-m_offset.m_value)
            -m_damping.m_value*velocity);
}

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

    m_pSpecial = NULL;

    m_stiffness = 0;
    m_damping = 0;

    addHandler("destroy", "", OscConstraint::destroy_handler);
}

//! Destroy the constraint
void OscConstraint::on_destroy()
{
    if (m_pSpecial)
        delete m_pSpecial;

    simulation()->delete_constraint(*this);

    /* The constraint's memory is freed in the above delete_object
     * call.  Should it be done here instead? Or perhaps moved to a
     * deleted objects pool for later garbage collection. */

    return;
}

//! A ball joint requires a single fixed anchor point
OscBallJoint::OscBallJoint(const char *name, OscBase *parent,
                           OscObject *object1, OscObject *object2,
                           double x, double y, double z)
    : OscConstraint(name, parent, object1, object2)
{
}

//! A hinge requires a fixed anchor point and an axis
OscHinge::OscHinge(const char *name, OscBase* parent,
                   OscObject *object1, OscObject *object2,
                   double x, double y, double z, double ax, double ay, double az)
    : OscConstraint(name, parent, object1, object2),
      m_torque("torque", this), m_angle("angle", this)
{
    m_torque.setSetCallback(set_torque, this);
}

//! A hinge requires a fixed anchor point and an axis
OscHinge2::OscHinge2(const char *name, OscBase *parent,
                     OscObject *object1, OscObject *object2,
                     double x, double y, double z,
                     double ax, double ay, double az,
                     double bx, double by, double bz)
    : OscConstraint(name, parent, object1, object2),
      m_torque1("torque1", this), m_torque2("torque2", this),
      m_angle1("angle1", this), m_angle2("angle2", this)
{
    m_torque1.setSetCallback(set_torque1, this);
    m_torque2.setSetCallback(set_torque2, this);
}

//! A free contraint has no requirements.
OscFree::OscFree(const char *name, OscBase *parent,
                 OscObject *object1, OscObject *object2)
    : OscConstraint(name, parent, object1, object2),
      m_force("force", this), m_torque("torque", this)
{
    m_force.setSetCallback(set_force, this);
    m_torque.setSetCallback(set_torque, this);
}

//! A hinge requires a fixed anchor point and an axis
OscUniversal::OscUniversal(const char *name, OscBase *parent,
                           OscObject *object1, OscObject *object2,
                           double x,   double y,   double z,
                           double a1x, double a1y, double a1z,
                           double a2x, double a2y, double a2z)
    : OscConstraint(name, parent, object1, object2),
      m_torque1("torque1", this), m_torque2("torque2", this),
      m_angle1("angle1", this), m_angle2("angle2", this)
{
    m_torque1.setSetCallback(set_torque1, this);
    m_torque2.setSetCallback(set_torque2, this);
}

//! A slide requires an axis
OscSlide::OscSlide(const char *name, OscBase *parent,
                   OscObject *object1, OscObject *object2,
                   double ax, double ay, double az)
    : OscConstraint(name, parent, object1, object2),
      m_force("force", this), m_position("position", this)
{
    m_force.setSetCallback(set_force, this);
}

//! A piston requires a position and an axis
OscPiston::OscPiston(const char *name, OscBase *parent, OscObject *object1,
                     OscObject *object2, double x, double y, double z,
                     double ax, double ay, double az)
    : OscConstraint(name, parent, object1, object2),
      m_force("force", this), m_position("position", this)
{
    m_force.setSetCallback(set_force, this);
}
