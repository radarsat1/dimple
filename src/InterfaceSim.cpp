// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#include "dimple.h"
#include "InterfaceSim.h"
#include "HapticsSim.h"

bool InterfacePrismFactory::create(const char *name, float x, float y, float z)
{
    printf("InterfacePrismFactory (%s) is creating a prism object called '%s'\n",
           m_parent->c_name(), name);

    OscPrism *obj = new OscPrismInterface(NULL, name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.set(x, y, z);
    obj->traceOn();

    simulation()->send(0, "/world/prism/create", "sfff", name, x, y, z);

    return true;
}

bool InterfaceSphereFactory::create(const char *name, float x, float y, float z)
{
    OscSphere *obj = new OscSphereInterface(NULL, name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.set(x, y, z);
    obj->traceOn();

    simulation()->send(0, "/world/sphere/create", "sfff", name, x, y, z);

    return true;
}

bool InterfaceMeshFactory::create(const char *name, const char *filename,
                                  float x, float y, float z)
{
    OscMesh *obj = new OscMeshInterface(NULL, name, filename, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.set(x, y, z);
    obj->traceOn();

    simulation()->send(0, "/world/mesh/create", "ssfff",
                       name, filename, x, y, z);

    return true;
}

bool InterfaceHingeFactory::create(const char *name, OscObject *object1, OscObject *object2,
                                   double x, double y, double z,
                                   double ax, double ay, double az)
{
    if (!object1) return false;

    OscHinge *cons = new OscHingeInterface(name, m_parent, object1, object2,
                                           x, y, z, ax, ay, az);
    if (!(cons && simulation()->add_constraint(*cons)))
            return false;

    cons->traceOn();

    simulation()->send(0, "/world/hinge/create", "sssffffff",
                       name, object1->c_name(), object2?object2->c_name():"world",
                       x, y, z, ax, ay, az);

    return true;
}

bool InterfaceHinge2Factory::create(const char *name, OscObject *object1,
                                    OscObject *object2, double x,
                                    double y, double z, double a1x,
                                    double a1y, double a1z, double a2x,
                                    double a2y, double a2z)
{
    if (!object1) return false;

    OscHinge2 *cons = new OscHinge2Interface(name, m_parent, object1,
                                             object2, x, y, z,
                                             a1x, a1y, a1z,
                                             a2x, a2y, a2z);
    if (!(cons && simulation()->add_constraint(*cons)))
            return false;

    cons->traceOn();

    simulation()->send(0, "/world/hinge2/create", "sssfffffffff",
                       name, object1->c_name(),
                       object2?object2->c_name():"world",
                       x, y, z, a1x, a1y, a1z, a2x, a2y, a2z);

    return true;
}

bool InterfaceFixedFactory::create(const char *name, OscObject *object1, OscObject *object2)
{
    if (!object1) return false;

    OscFixed *cons = new OscFixedInterface(name, m_parent, object1, object2);
    if (!(cons && simulation()->add_constraint(*cons)))
            return false;

    cons->traceOn();

    simulation()->send(0, "/world/fixed/create", "sss",
                       name, object1->c_name(), object2?object2->c_name():"world");

    return true;
}

bool InterfaceBallJointFactory::create(const char *name, OscObject *object1,
                                       OscObject *object2, double x,
                                       double y, double z)
{
    if (!object1) return false;

    OscBallJoint *cons = new OscBallJointInterface(name, m_parent, object1,
                                                   object2, x, y, z);
    if (!(cons && simulation()->add_constraint(*cons)))
            return false;

    cons->traceOn();

    simulation()->send(0, "/world/ball/create", "sssfff",
                       name, object1->c_name(),
                       object2?object2->c_name():"world",
                       x, y, z);

    return true;
}

bool InterfaceSlideFactory::create(const char *name, OscObject *object1,
                                       OscObject *object2, double ax,
                                       double ay, double az)
{
    if (!object1) return false;

    OscSlide *cons = new OscSlideInterface(name, m_parent, object1,
                                           object2, ax, ay, az);
    if (!(cons && simulation()->add_constraint(*cons)))
            return false;

    cons->traceOn();

    simulation()->send(0, "/world/slide/create", "sssfff",
                       name, object1->c_name(),
                       object2?object2->c_name():"world",
                       ax, ay, az);

    return true;
}

bool InterfacePistonFactory::create(const char *name, OscObject *object1, OscObject *object2,
                                    double x, double y, double z,
                                    double ax, double ay, double az)
{
    if (!object1) return false;

    OscPiston *cons = new OscPistonInterface(name, m_parent, object1, object2,
                                             x, y, z, ax, ay, az);
    if (!(cons && simulation()->add_constraint(*cons)))
        return false;

    cons->traceOn();

    simulation()->send(0, "/world/piston/create", "sssffffff",
                       name, object1->c_name(), object2?object2->c_name():"world",
                       x, y, z, ax, ay, az);

    return true;
}

bool InterfaceUniversalFactory::create(const char *name, OscObject *object1,
                                       OscObject *object2, double x,
                                       double y, double z, double a1x,
                                       double a1y, double a1z, double a2x,
                                       double a2y, double a2z)
{
    if (!object1) return false;

    OscUniversal *cons = new OscUniversalInterface(name, m_parent, object1,
                                                   object2, x, y, z,
                                                   a1x, a1y, a1z,
                                                   a2x, a2y, a2z);
    if (!(cons && simulation()->add_constraint(*cons)))
            return false;

    cons->traceOn();

    simulation()->send(0, "/world/universal/create", "sssfffffffff",
                       name, object1->c_name(),
                       object2?object2->c_name():"world",
                       x, y, z, a1x, a1y, a1z, a2x, a2y, a2z);

    return true;
}

/****** InterfaceSim ******/

InterfaceSim::InterfaceSim(const char *port)
    : Simulation(port, ST_INTERFACE)
{
    m_pPrismFactory = new InterfacePrismFactory(this);
    m_pSphereFactory = new InterfaceSphereFactory(this);
    m_pMeshFactory = new InterfaceMeshFactory(this);
    m_pHingeFactory = new InterfaceHingeFactory(this);
    m_pHinge2Factory = new InterfaceHinge2Factory(this);
    m_pFixedFactory = new InterfaceFixedFactory(this);
    m_pBallJointFactory = new InterfaceBallJointFactory(this);
    m_pSlideFactory = new InterfaceSlideFactory(this);
    m_pPistonFactory = new InterfacePistonFactory(this);
    m_pUniversalFactory = new InterfaceUniversalFactory(this);

    m_camera = new OscCameraInterface("camera", this);
    new OscCursorInterface(NULL, "cursor", this);

    m_fTimestep = 1;
}

InterfaceSim::~InterfaceSim()
{
    if (m_camera) delete m_camera;
}

void InterfaceSim::step()
{
}

int OscPrismInterface::push_handler(const char *path, const char *types,
                                    lo_arg **argv, int argc, void *data,
                                    void *user_data)
{
    OscPrismInterface *me = static_cast<OscPrismInterface*>(user_data);
    me->simulation()->sendtotype(Simulation::ST_PHYSICS, 0,
                                 (me->path()+"/push").c_str(), "ffffff",
                                 argv[0]->f, argv[1]->f, argv[2]->f,
                                 argv[3]->f, argv[4]->f, argv[5]->f);
}

int OscSphereInterface::push_handler(const char *path, const char *types,
                                     lo_arg **argv, int argc, void *data,
                                     void *user_data)
{
    OscSphereInterface *me = static_cast<OscSphereInterface*>(user_data);
    me->simulation()->sendtotype(Simulation::ST_PHYSICS, 0,
                                 (me->path()+"/push").c_str(), "ffffff",
                                 argv[0]->f, argv[1]->f, argv[2]->f,
                                 argv[3]->f, argv[4]->f, argv[5]->f);
}

int OscMeshInterface::push_handler(const char *path, const char *types,
                                    lo_arg **argv, int argc, void *data,
                                    void *user_data)
{
    OscMeshInterface *me = static_cast<OscMeshInterface*>(user_data);
    me->simulation()->sendtotype(Simulation::ST_PHYSICS, 0,
                                 (me->path()+"/push").c_str(), "ffffff",
                                 argv[0]->f, argv[1]->f, argv[2]->f,
                                 argv[3]->f, argv[4]->f, argv[5]->f);
}
