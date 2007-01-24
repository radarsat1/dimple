
#include "OscObject.h"
#include "osc_chai_glut.h"

OscObject::OscObject(cGenericObject* p, const char *name)
{
    m_objChai = p;
    m_name = name;
}

OscObject::~OscObject()
{
    cGenericObject *p = m_objChai->getParent();
    p->deleteChild(m_objChai);
}

OscPrism::OscPrism(cGenericObject* p, const char *name)
    : OscObject(p, name)
{
    lo_server_thread_add_method(loserver, ("/object/"+m_name+"/size").c_str(),
                                "fff", OscPrism::size_handler, this);
}

OscPrism::~OscPrism()
{
    lo_server_thread_del_method(loserver, ("/object/"+m_name+"/size").c_str(), "fff");
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

OscSphere::OscSphere(cGenericObject* p, const char *name)
    : OscObject(p, name)
{
    lo_server_thread_add_method(loserver, ("/object/"+m_name+"/radius").c_str(),
                                "f", OscSphere::radius_handler, this);
}

OscSphere::~OscSphere()
{
    lo_server_thread_del_method(loserver, ("/object/"+m_name+"/radius").c_str(), "f");
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
}
