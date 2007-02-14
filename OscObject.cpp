
#include "OscObject.h"
#include "osc_chai_glut.h"
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

// ----------------------------------------------------------------------------------

//! OscObject has a CHAI/ODE object associated with it. Class name = "object"
OscObject::OscObject(cGenericObject* p, const char *name)
    : OscBase(name, "object")
{
    m_objChai = p;
    addHandler("destroy", ""   , OscObject::destroy_handler);
    addHandler("mass"   , "f"  , OscObject::mass_handler);
    addHandler("force"  , "fff", OscObject::force_handler);

    // If the new object is supposed to be a part of a
    // composite object, find it and join.
    char *s;
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
    printf("destroying %s: is locked = %d\n", m_name.c_str(), WORLD_LOCKED());
    // Destroy any constraints associated with the object
    while (odePrimitive()->m_Joint.size() > 0) {
        std::string jointname = odePrimitive()->m_Joint.begin()->first;
        if (world_constraints.find(jointname)!=world_constraints.end())
            delete world_constraints[jointname];
        world_constraints.erase(jointname);
        printf("Deleted constraint %s\n", jointname.c_str());
    }
    
    // Destroy any constraints to which this object is linked
    while (m_constraintLinks.size() > 0)
    {
        std::string jointname = m_constraintLinks.back();
        m_constraintLinks.pop_back();
        
        if (world_constraints.find(jointname)!=world_constraints.end())
            delete world_constraints[jointname];
        world_constraints.erase(jointname);
        printf("Deleted constraint %s\n", jointname.c_str());
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

//! Destroy the object
int OscObject::destroy_handler(const char *path, const char *types, lo_arg **argv,
							   int argc, void *data, void *user_data)
{
    OscObject *me = (OscObject*)user_data;

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
    OscObject *me = (OscObject*)user_data;
    me->odePrimitive()->setMass(argv[0]->f);
    UNLOCK_WORLD();
    return 0;
}

//! Add an instantaneous force to an object
int OscObject::force_handler(const char *path, const char *types, lo_arg **argv,
                             int argc, void *data, void *user_data)
{
    if (argc!=3) return 0;
    LOCK_WORLD();
    OscObject *me = (OscObject*)user_data;
    dBodySetForce(me->odePrimitive()->m_odeBody, argv[0]->f, argv[1]->f, argv[2]->f);
    UNLOCK_WORLD();
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

    OscPrism* me = (OscPrism*)user_data;
	cODEPrism *prism = me->odePrimitive();
    if (prism)
    {
        cVector3d size;
        size.x = argv[0]->f;
        size.y = argv[1]->f;
        size.z = argv[2]->f;
        LOCK_WORLD();
        prism->setSize(size);
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

    OscSphere* me = (OscSphere*)user_data;
	cODESphere *sphere = me->odePrimitive();
    if (sphere) {
        LOCK_WORLD();
        sphere->setRadius(argv[0]->f);
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
    OscConstraint *me = (OscConstraint*)user_data;

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

    OscConstraint *me = (OscConstraint*)user_data;
    me->m_stiffness = argv[0]->f;
    me->m_damping = 0;
}

int OscConstraint::responseSpring_handler(const char *path, const char *types, lo_arg **argv,
                                          int argc, void *data, void *user_data)
{
    if (argc!=2) return 0;

    OscConstraint *me = (OscConstraint*)user_data;
    me->m_stiffness = argv[0]->f;
    me->m_damping = argv[1]->f;
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

    printf("ball link created between %s and %s at (%f,%f,%f)\n", object1->name(), object2?object2->name():"world", x,y,z);
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

    printf("Hinge joint created between %s and %s at anchor (%f,%f,%f), axis (%f,%f,%f)\n",
        object1->name(), object2?object2->name():"world", x,y,z,ax,ay,az);
}

//! This function is called once per simulation step, allowing the
//! constraint to be "motorized" according to some response.
//! It runs in the haptics thread.
void OscHinge::simulationCallback()
{
    dJointID *id;
    if (!m_object1->odePrimitive()->getJoint(m_name, id))
        return;

    dReal angle = dJointGetHingeAngle(*id);
    dReal rate = dJointGetHingeAngleRate(*id);
    dJointAddHingeTorque(*id, -m_stiffness*angle - m_damping*rate);
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
    object1->odePrimitive()->fixedLink(name, object2?object2->odePrimitive():NULL);

    printf("Fixed joint created between %s and %s\n",
           object1->name(), object2?object2->name():"world");
}
