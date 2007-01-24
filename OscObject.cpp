
#include "OscObject.h"
#include "osc_chai_glut.h"
#include <assert.h>

//! OscBase objects always have a name. Class name defaults to "".
OscBase::OscBase(const char *name, const char *classname)
    : m_name(name), m_classname(classname)
{
    addHandler("destroy", "", destroy_handler);
}

void OscBase::addHandler(const char *methodname, const char* type, lo_method_handler h)
{
    // build OSC method name
    std::string n("/");
    if (m_classname.length()>0)
        n += m_classname + "/";
    n += m_name + "/" + methodname;

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

int OscBase::destroy_handler(const char *path, const char *types, lo_arg **argv,
                             int argc, void *data, void *user_data)
{
    OscObject *me = (OscObject*)user_data;

    if (me) {
        world_objects.erase(me->m_name);
        delete me;
    }
    return 0;
}

// ----------------------------------------------------------------------------------

//! OscObject has a CHAI/ODE object associated with it. Class name = "object"
OscObject::OscObject(cGenericObject* p, const char *name)
    : OscBase(name, "object")
{
    m_objChai = p;
    addHandler("force", "fff", force_handler);
}

//! OscObject destructor is responsible for deleting the object from the CHAI world.
//! ODE removal is taken care of in the cODEPrimitive destructor.
OscObject::~OscObject()
{
    cGenericObject *p = m_objChai->getParent();
    p->deleteChild(m_objChai);
}

//! Add an instantaneous force to an object
int OscObject::force_handler(const char *path, const char *types, lo_arg **argv,
                             int argc, void *data, void *user_data)
{
    if (argc!=3) return 0;
    OscObject *me = (OscObject*)user_data;
    dBodySetForce(me->odePrimitive()->m_odeBody, argv[0]->f, argv[1]->f, argv[2]->f);
    return 0;
}

// ----------------------------------------------------------------------------------

OscPrism::OscPrism(cGenericObject* p, const char *name)
    : OscObject(p, name)
{
    addHandler("size", "fff", OscPrism::size_handler);
}

int OscPrism::size_handler(const char *path, const char *types, lo_arg **argv,
                           int argc, void *data, void *user_data)
{
    if (argc!=3)
        return 0;

    OscPrism* me = (OscPrism*)user_data;
	cODEPrism *prism = me->odePrimitive();
    if (prism)
    {
        cVector3d size;
        size.x = argv[0]->f;
        size.y = argv[1]->f;
        size.z = argv[2]->f;
        prism->setSize(size);
    }

	return 0;
}

// ----------------------------------------------------------------------------------

OscSphere::OscSphere(cGenericObject* p, const char *name)
    : OscObject(p, name)
{
    addHandler("radius", "f", OscSphere::radius_handler);
}

int OscSphere::radius_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data)
{
    if (argc!=1)
        return 0;

    OscSphere* me = (OscSphere*)user_data;
	cODESphere *sphere = me->odePrimitive();
    if (sphere)
        sphere->setRadius(argv[0]->f);
    return 0;
}

// ----------------------------------------------------------------------------------

//! OscConstraint has two CHAI/ODE object associated with it, though not owned by it. Class name = "constraint"
OscConstraint::OscConstraint(const char *name, OscObject *object1, OscObject *object2)
    : OscBase(name, "constraint")
{
    assert(m_object1);
    m_object1 = object1;
    m_object2 = object2;
}

OscConstraint::~OscConstraint()
{
    if (!m_object1) return;
    m_object1->odePrimitive()->destroyJoint(m_name);
}

// ----------------------------------------------------------------------------------

//! A ball joint requires a single fixed anchor point
OscBallJoint::OscBallJoint(const char *name, OscObject *object1, OscObject *object2,
                           double x, double y, double z)
    : OscConstraint(name, object1, object2)
{
    if (!object1) return;

    cVector3d anchor(x,y,z);
    object1->odePrimitive()->ballLink(name, object2?object2->odePrimitive():NULL, anchor);

    printf("ball link created between %s and %s at (%f,%f,%f)\n", object1->name(), object2?object2->name():"world", x,y,z);
}
