// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-

#ifndef _INTERFACE_SIM_H_
#define _INTERFACE_SIM_H_

#include "Simulation.h"
#include "OscObject.h"

// Macros for defining forwarding handlers for OscValue instances to
// the other simulations.

#define FWD_OSCSCALAR(o,t)                                              \
    virtual void on_##o() {                                             \
        simulation()->send(0,m_##o.c_path(), "f",                       \
                           m_##o.m_value); }                            \
    static void on_get_##o(void *me, OscScalar &o, int interval) {      \
        ((OscBase*)me)->simulation()->sendtotype(t,0,                   \
                                       (o.path()+"/get").c_str(),       \
                                       (interval>=0)?"i":"", interval); }
#define FWD_OSCVECTOR3(o,t)                                             \
    virtual void on_##o() {                                             \
        simulation()->send(0,m_##o.c_path(), "fff",                     \
                           m_##o.x(), m_##o.y(), m_##o.z()); }          \
    static void on_get_##o(void *me, OscVector3 &o, int interval){      \
        ((OscBase*)me)->simulation()->sendtotype(t,0,                   \
                                        (o.path()+"/get").c_str(),      \
                                        (interval>=0)?"i":"", interval);}\
    static void on_get_##o##_mag(void *me, OscScalar &o, int interval){ \
        ((OscBase*)me)->simulation()->sendtotype(t,0,                   \
                                        (o.path()+"/get").c_str(),      \
                                        (interval>=0)?"i":"", interval);}
#define FWD_OSCMATRIX3(o,t)                                             \
    virtual void on_##o() {                                             \
        simulation()->send(0,m_##o.c_path(), "fffffffff",               \
                           m_##o(0,0), m_##o(0,1), m_##o(0,2),          \
                           m_##o(1,0), m_##o(1,1), m_##o(1,2),          \
                           m_##o(2,0), m_##o(2,1), m_##o(2,2));}        \
    static void on_get_##o(void *me, OscMatrix3 &o, int interval){      \
        ((OscBase*)me)->simulation()->sendtotype(t,0,                   \
                                        (o.path()+"/get").c_str(),      \
                                        (interval>=0)?"i":"", interval);}
#define FWD_OSCBOOLEAN(o,t)                                              \
    virtual void on_##o() {                                             \
        simulation()->send(0,m_##o.c_path(), "i",                       \
                           m_##o.m_value!=0); }                         \
    static void on_get_##o(void *me, OscBoolean &o, int interval) {     \
        ((OscBase*)me)->simulation()->sendtotype(t,0,                   \
                                       (o.path()+"/get").c_str(),       \
                                       (interval>=0)?"i":"", interval); }
#define FWD_OSCSTRING(o,t)                                              \
    virtual void on_##o() {                                             \
        simulation()->send(0,m_##o.c_path(), "s",                       \
                           m_##o.m_value); }                            \
    static void on_get_##o(void *me, OscString &o, int interval){       \
        ((OscBase*)me)->simulation()->sendtotype(t,0,                   \
                                        (o.path()+"/get").c_str(),      \
                                        (interval>=0)?"i":"", interval);}

class OscCameraInterface;
class OscCursorInterface;

class InterfaceSim : public Simulation
{
  public:
    InterfaceSim(const char *port);
    virtual ~InterfaceSim();

    virtual void on_clear() {
        send(0, "/world/clear", "");
        Simulation::on_clear();
    }

    virtual void on_collide() {
        sendtotype(Simulation::ST_PHYSICS, 0, "/world/collide",
                   "f", m_collide.m_value);
        Simulation::on_collide();
    }

    virtual void on_gravity() {
        sendtotype(Simulation::ST_PHYSICS, 0, "/world/gravity",
                   "fff", m_gravity.x(), m_gravity.y(), m_gravity.z());
        Simulation::on_gravity();
    }

    virtual void on_drop() {
        send(0, "/world/drop", "");
        Simulation::on_drop();
    }

    virtual void on_add_receiver(const char *type);

    FWD_OSCVECTOR3(scale, Simulation::ST_HAPTICS);

  protected:
    OscCameraInterface *m_camera;
    OscCursorInterface *m_cursor;
    virtual void step();
};

class InterfacePrismFactory : public PrismFactory
{
public:
    InterfacePrismFactory(Simulation *parent) : PrismFactory(parent) {}
    virtual ~InterfacePrismFactory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

class InterfaceSphereFactory : public SphereFactory
{
public:
    InterfaceSphereFactory(Simulation *parent) : SphereFactory(parent) {}
    virtual ~InterfaceSphereFactory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

class InterfaceMeshFactory : public MeshFactory
{
public:
    InterfaceMeshFactory(Simulation *parent) : MeshFactory(parent) {}
    virtual ~InterfaceMeshFactory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, const char *filename,
                float x, float y, float z);
};

class InterfaceHingeFactory : public HingeFactory
{
public:
    InterfaceHingeFactory(Simulation *parent) : HingeFactory(parent) {}
    virtual ~InterfaceHingeFactory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2,
                double x, double y, double z, double ax, double ay, double az);
};

class InterfaceHinge2Factory : public Hinge2Factory
{
public:
    InterfaceHinge2Factory(Simulation *parent) : Hinge2Factory(parent) {}
    virtual ~InterfaceHinge2Factory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2,
                double x,   double y,   double z,
                double a1x, double a1y, double a1z,
                double a2x, double a2y, double a2z);
};

class InterfaceFixedFactory : public FixedFactory
{
public:
    InterfaceFixedFactory(Simulation *parent) : FixedFactory(parent) {}
    virtual ~InterfaceFixedFactory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2);
};

class InterfaceFreeFactory : public FreeFactory
{
public:
    InterfaceFreeFactory(Simulation *parent) : FreeFactory(parent) {}
    virtual ~InterfaceFreeFactory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2);
};

class InterfaceBallJointFactory : public BallJointFactory
{
public:
    InterfaceBallJointFactory(Simulation *parent) : BallJointFactory(parent) {}
    virtual ~InterfaceBallJointFactory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2,
                double x, double y, double z);
};

class InterfaceSlideFactory : public SlideFactory
{
public:
    InterfaceSlideFactory(Simulation *parent) : SlideFactory(parent) {}
    virtual ~InterfaceSlideFactory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2,
                double ax, double ay, double az);
};

class InterfacePistonFactory : public PistonFactory
{
public:
    InterfacePistonFactory(Simulation *parent) : PistonFactory(parent) {}
    virtual ~InterfacePistonFactory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2,
                double x, double y, double z, double ax, double ay, double az);
};

class InterfaceUniversalFactory : public UniversalFactory
{
public:
    InterfaceUniversalFactory(Simulation *parent) : UniversalFactory(parent) {}
    virtual ~InterfaceUniversalFactory() {}

    virtual InterfaceSim* simulation() { return static_cast<InterfaceSim*>(m_parent); }

protected:
    bool create(const char *name, OscObject *object1, OscObject *object2,
                double x,   double y,   double z,
                double a1x, double a1y, double a1z,
                double a2x, double a2y, double a2z);
};

class OscSphereInterface : public OscSphere
{
public:
    OscSphereInterface(cGenericObject *p, const char *name, OscBase *parent=NULL)
        : OscSphere(p, name, parent)
        {
            m_position.setGetCallback(on_get_position, this);
            m_velocity.setGetCallback(on_get_velocity, this);
            m_accel.setGetCallback(on_get_accel, this);
            m_color.setGetCallback(on_get_color, this);
            m_radius.setGetCallback(on_get_radius, this);
            m_force.setGetCallback(on_get_force, this);
            m_mass.setGetCallback(on_get_mass, this);
            m_density.setGetCallback(on_get_density, this);
            m_friction_static.setGetCallback(on_get_friction_static, this);
            m_friction_dynamic.setGetCallback(on_get_friction_dynamic, this);
            m_visible.setGetCallback(on_get_visible, this);

            // TODO: also forward set handlers for magnitudes

            m_position.m_magnitude.setGetCallback(on_get_position_mag, this);
            m_velocity.m_magnitude.setGetCallback(on_get_velocity_mag, this);
            m_accel.m_magnitude.setGetCallback(on_get_accel_mag, this);
            m_force.m_magnitude.setGetCallback(on_get_force_mag, this);

            addHandler("push", "ffffff", OscSphereInterface::push_handler);
        }
    virtual ~OscSphereInterface() {}

    virtual void on_grab() {
        simulation()->send(0, (path()+"/grab").c_str(), "");
        OscSphere::on_grab();
    }

    virtual void on_destroy() {
        simulation()->send(0, (path()+"/destroy").c_str(), "");
        OscSphere::on_destroy();
    }

    static int push_handler(const char *path, const char *types, lo_arg **argv,
                            int argc, void *data, void *user_data);

protected:
    FWD_OSCVECTOR3(position,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(velocity,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(accel,Simulation::ST_PHYSICS);
    FWD_OSCMATRIX3(rotation,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(color,Simulation::ST_VISUAL);
    FWD_OSCSCALAR(radius,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(force,Simulation::ST_PHYSICS);
    FWD_OSCSCALAR(mass,Simulation::ST_PHYSICS);
    FWD_OSCSCALAR(density,Simulation::ST_PHYSICS);
    FWD_OSCSCALAR(friction_dynamic,Simulation::ST_HAPTICS);
    FWD_OSCSCALAR(friction_static,Simulation::ST_HAPTICS);
    FWD_OSCSCALAR(collide,Simulation::ST_PHYSICS);
    FWD_OSCBOOLEAN(visible,Simulation::ST_VISUAL);
};

class OscPrismInterface : public OscPrism
{
public:
    OscPrismInterface(cGenericObject *p, const char *name, OscBase *parent=NULL)
        : OscPrism(p, name, parent)
        {
            m_position.setGetCallback(on_get_position, this);
            m_velocity.setGetCallback(on_get_velocity, this);
            m_accel.setGetCallback(on_get_accel, this);
            m_color.setGetCallback(on_get_color, this);
            m_force.setGetCallback(on_get_force, this);
            m_size.setGetCallback(on_get_size, this);
            m_mass.setGetCallback(on_get_mass, this);
            m_density.setGetCallback(on_get_density, this);
            m_friction_static.setGetCallback(on_get_friction_static, this);
            m_friction_dynamic.setGetCallback(on_get_friction_dynamic, this);
            m_visible.setGetCallback(on_get_visible, this);

            m_position.m_magnitude.setGetCallback(on_get_position_mag, this);
            m_velocity.m_magnitude.setGetCallback(on_get_velocity_mag, this);
            m_accel.m_magnitude.setGetCallback(on_get_accel_mag, this);
            m_force.m_magnitude.setGetCallback(on_get_force_mag, this);

            addHandler("push", "ffffff", OscPrismInterface::push_handler);
        }
    virtual ~OscPrismInterface() {}

    virtual void on_grab() {
        simulation()->send(0, (path()+"/grab").c_str(), "");
        OscPrism::on_grab();
    }

    virtual void on_destroy() {
        simulation()->send(0, (path()+"/destroy").c_str(), "");
        OscPrism::on_destroy();
    }

    static int push_handler(const char *path, const char *types, lo_arg **argv,
                            int argc, void *data, void *user_data);

protected:
    FWD_OSCVECTOR3(position,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(velocity,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(accel,Simulation::ST_PHYSICS);
    FWD_OSCMATRIX3(rotation,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(color,Simulation::ST_VISUAL);
    FWD_OSCVECTOR3(force,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(size,Simulation::ST_PHYSICS);
    FWD_OSCSCALAR(mass,Simulation::ST_PHYSICS);
    FWD_OSCSCALAR(density,Simulation::ST_PHYSICS);
    FWD_OSCSCALAR(friction_dynamic,Simulation::ST_HAPTICS);
    FWD_OSCSCALAR(friction_static,Simulation::ST_HAPTICS);
    FWD_OSCSCALAR(collide,Simulation::ST_PHYSICS);
    FWD_OSCBOOLEAN(visible,Simulation::ST_VISUAL);
};

class OscMeshInterface : public OscMesh
{
public:
    OscMeshInterface(cGenericObject *p, const char *name,
                     const char *filename, OscBase *parent=NULL)
        : OscMesh(p, name, filename, parent)
        {
            m_position.setGetCallback(on_get_position, this);
            m_velocity.setGetCallback(on_get_velocity, this);
            m_accel.setGetCallback(on_get_accel, this);
            m_color.setGetCallback(on_get_color, this);
            m_friction_static.setGetCallback(on_get_friction_static, this);
            m_friction_dynamic.setGetCallback(on_get_friction_dynamic, this);
            m_force.setGetCallback(on_get_force, this);
            m_size.setGetCallback(on_get_size, this);
            m_visible.setGetCallback(on_get_visible, this);

            m_position.m_magnitude.setGetCallback(on_get_position_mag, this);
            m_velocity.m_magnitude.setGetCallback(on_get_velocity_mag, this);
            m_accel.m_magnitude.setGetCallback(on_get_accel_mag, this);
            m_force.m_magnitude.setGetCallback(on_get_force_mag, this);

            addHandler("push", "ffffff", OscMeshInterface::push_handler);
        }
    virtual ~OscMeshInterface() {}

    virtual void on_grab() {
        simulation()->send(0, (path()+"/grab").c_str(), "");
        OscMesh::on_grab();
    }

    virtual void on_destroy() {
        simulation()->send(0, (path()+"/destroy").c_str(), "");
        OscMesh::on_destroy();
    }

    static int push_handler(const char *path, const char *types, lo_arg **argv,
                            int argc, void *data, void *user_data);

protected:
    FWD_OSCVECTOR3(position,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(velocity,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(accel,Simulation::ST_PHYSICS);
    FWD_OSCMATRIX3(rotation,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(color,Simulation::ST_VISUAL);
    FWD_OSCVECTOR3(force,Simulation::ST_PHYSICS);
    FWD_OSCVECTOR3(size,Simulation::ST_PHYSICS);
    FWD_OSCSCALAR(mass,Simulation::ST_PHYSICS);
    FWD_OSCSCALAR(friction_dynamic,Simulation::ST_HAPTICS);
    FWD_OSCSCALAR(friction_static,Simulation::ST_HAPTICS);
    FWD_OSCSCALAR(collide,Simulation::ST_PHYSICS);
    FWD_OSCBOOLEAN(visible,Simulation::ST_VISUAL);
};

class OscCameraInterface : public OscCamera
{
public:
    OscCameraInterface(const char *name, OscBase *parent=NULL)
        : OscCamera(name, parent)
        {
            m_position.setGetCallback(on_get_position, this);
            m_lookat.setGetCallback(on_get_lookat, this);
            m_up.setGetCallback(on_get_up, this);
        }
    virtual ~OscCameraInterface() {}

protected:
    FWD_OSCVECTOR3(position,Simulation::ST_VISUAL);
    FWD_OSCVECTOR3(lookat,Simulation::ST_VISUAL);
    FWD_OSCVECTOR3(up,Simulation::ST_VISUAL);
};

class OscCursorInterface : public OscSphere
{
public:
    OscCursorInterface(cGenericObject *p, const char *name, OscBase *parent=NULL)
        : OscSphere(p, name, parent)
        {
            m_position.setGetCallback(on_get_position, this);
            m_velocity.setGetCallback(on_get_velocity, this);
            m_accel.setGetCallback(on_get_accel, this);
            m_color.setGetCallback(on_get_color, this);
            m_radius.setGetCallback(on_get_radius, this);
            m_force.setGetCallback(on_get_force, this);
            m_mass.setGetCallback(on_get_mass, this);
            m_density.setGetCallback(on_get_density, this);
            m_visible.setGetCallback(on_get_visible, this);

            m_position.m_magnitude.setGetCallback(on_get_position_mag, this);
            m_velocity.m_magnitude.setGetCallback(on_get_velocity_mag, this);
            m_accel.m_magnitude.setGetCallback(on_get_accel_mag, this);
            m_force.m_magnitude.setGetCallback(on_get_force_mag, this);
        }
    virtual ~OscCursorInterface() {}

protected:
    FWD_OSCVECTOR3(position,Simulation::ST_HAPTICS);
    FWD_OSCVECTOR3(velocity,Simulation::ST_HAPTICS);
    FWD_OSCVECTOR3(accel,Simulation::ST_HAPTICS);
    FWD_OSCMATRIX3(rotation,Simulation::ST_HAPTICS);
    FWD_OSCVECTOR3(color,Simulation::ST_VISUAL);
    FWD_OSCSCALAR(radius,Simulation::ST_HAPTICS);
    FWD_OSCVECTOR3(force,Simulation::ST_HAPTICS);
    FWD_OSCSCALAR(mass,Simulation::ST_HAPTICS);
    FWD_OSCSCALAR(density,Simulation::ST_HAPTICS);
    FWD_OSCSCALAR(collide,Simulation::ST_HAPTICS);
    FWD_OSCBOOLEAN(visible,Simulation::ST_VISUAL);
};

class OscResponseInterface : public OscResponse
{
public:
    OscResponseInterface(const char *name, OscBase *parent)
        : OscResponse(name, parent) {}

    virtual void on_spring(float arg1, float arg2) {
        simulation()->send(0, (path()+"/spring").c_str(), "ff", arg1, arg2);
    }

protected:
    FWD_OSCSCALAR(stiffness,Simulation::ST_PHYSICS);
    FWD_OSCSCALAR(damping,Simulation::ST_PHYSICS);
};

class OscHingeInterface : public OscHinge
{
public:
    OscHingeInterface(const char *name, OscBase *parent,
                      OscObject *object1, OscObject *object2,
                      double x, double y, double z, double ax, double ay, double az)
        : OscHinge(name, parent, object1, object2, x, y, z, ax, ay, az)
        {
            m_response = new OscResponseInterface("response",this);
            m_torque.setGetCallback(on_get_torque, this);
        }

    virtual ~OscHingeInterface() { delete m_response; }

    virtual void on_destroy() {
        simulation()->send(0, (path()+"/destroy").c_str(), "");
        OscHinge::on_destroy();
    }

protected:
    FWD_OSCSCALAR(torque,Simulation::ST_PHYSICS);
};

class OscHinge2Interface : public OscHinge2
{
public:
    OscHinge2Interface(const char *name, OscBase *parent,
                       OscObject *object1, OscObject *object2,
                       double x,   double y,   double z,
                       double a1x, double a1y, double a1z,
                       double a2x, double a2y, double a2z)
        : OscHinge2(name, parent, object1, object2, x, y, z,
                    a1x, a1y, a1z, a2x, a2y, a2z)
        {
            m_response = new OscResponseInterface("response",this);
            m_torque1.setGetCallback(on_get_torque1, this);
            m_torque2.setGetCallback(on_get_torque2, this);
        }

    virtual ~OscHinge2Interface() { delete m_response; }

    virtual void on_destroy() {
        simulation()->send(0, (path()+"/destroy").c_str(), "");
        OscHinge2::on_destroy();
    }

protected:
    FWD_OSCSCALAR(torque1,Simulation::ST_PHYSICS);
    FWD_OSCSCALAR(torque2,Simulation::ST_PHYSICS);
};

class OscFixedInterface : public OscFixed
{
public:
    OscFixedInterface(const char *name, OscBase *parent,
                      OscObject *object1, OscObject *object2)
        : OscFixed(name, parent, object1, object2) {}

    virtual ~OscFixedInterface() {}

    virtual void on_destroy() {
        simulation()->send(0, (path()+"/destroy").c_str(), "");
        OscFixed::on_destroy();
    }

protected:
};

class OscFreeInterface : public OscFree
{
public:
    OscFreeInterface(const char *name, OscBase *parent,
                      OscObject *object1, OscObject *object2)
        : OscFree(name, parent, object1, object2)
        { m_response = new OscResponseInterface("response",this); }

    virtual ~OscFreeInterface() { delete m_response; }

    virtual void on_destroy() {
        simulation()->send(0, (path()+"/destroy").c_str(), "");
        OscFree::on_destroy();
    }

protected:
};

class OscBallJointInterface : public OscBallJoint
{
public:
    OscBallJointInterface(const char *name, OscBase *parent,
                          OscObject *object1, OscObject *object2,
                          double x, double y, double z)
        : OscBallJoint(name, parent, object1, object2, x, y, z) {}

    virtual ~OscBallJointInterface() {}

    virtual void on_destroy() {
        simulation()->send(0, (path()+"/destroy").c_str(), "");
        OscBallJoint::on_destroy();
    }

protected:
};

class OscSlideInterface : public OscSlide
{
public:
    OscSlideInterface(const char *name, OscBase *parent,
                          OscObject *object1, OscObject *object2,
                          double ax, double ay, double az)
        : OscSlide(name, parent, object1, object2, ax, ay, az)
        {
            m_response = new OscResponseInterface("response",this);
            m_force.setGetCallback(on_get_force, this);
        }

    virtual ~OscSlideInterface() { delete m_response; }

    virtual void on_destroy() {
        simulation()->send(0, (path()+"/destroy").c_str(), "");
        OscSlide::on_destroy();
    }

protected:
    FWD_OSCSCALAR(force,Simulation::ST_PHYSICS);
};

class OscPistonInterface : public OscPiston
{
public:
    OscPistonInterface(const char *name, OscBase *parent,
                       OscObject *object1, OscObject *object2,
                       double x, double y, double z, double ax, double ay, double az)
        : OscPiston(name, parent, object1, object2, x, y, z, ax, ay, az)
        {
            m_response = new OscResponseInterface("response",this);
            m_force.setGetCallback(on_get_force, this);
        }

    virtual ~OscPistonInterface() { delete m_response; }

    virtual void on_destroy() {
        simulation()->send(0, (path()+"/destroy").c_str(), "");
        OscPiston::on_destroy();
    }

protected:
    FWD_OSCSCALAR(force,Simulation::ST_PHYSICS);
};

class OscUniversalInterface : public OscUniversal
{
public:
    OscUniversalInterface(const char *name, OscBase *parent,
                          OscObject *object1, OscObject *object2,
                          double x,   double y,   double z,
                          double a1x, double a1y, double a1z,
                          double a2x, double a2y, double a2z)
        : OscUniversal(name, parent, object1, object2, x, y, z,
                       a1x, a1y, a1z, a2x, a2y, a2z)
        {
            m_response = new OscResponseInterface("response",this);
            m_torque1.setGetCallback(on_get_torque1, this);
            m_torque2.setGetCallback(on_get_torque2, this);
        }

    virtual ~OscUniversalInterface() { delete m_response; }

    virtual void on_destroy() {
        simulation()->send(0, (path()+"/destroy").c_str(), "");
        OscUniversal::on_destroy();
    }

protected:
    FWD_OSCSCALAR(torque1,Simulation::ST_PHYSICS);
    FWD_OSCSCALAR(torque2,Simulation::ST_PHYSICS);
};

#endif // _INTERFACE_SIM_H_
