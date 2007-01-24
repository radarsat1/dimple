
#include "OscObject.h"
#include "osc_chai_glut.h"

OscObject::~OscObject()
{
    cGenericObject *p = m_objChai->getParent();
    p->deleteChild(m_objChai);
    printf("here.1\n");
}

OscPrism::OscPrism(cGenericObject* p, const char *name)
    : OscObject(p)
{
}

OscSphere::OscSphere(cGenericObject* p, const char *name)
    : OscObject(p)
{
    char str[1024];
    snprintf(str, 1024, "/object/%s/radius", name);
    lo_server_thread_add_method(loserver, str, "f", OscSphere::radius_handler, this);
}

OscSphere::~OscSphere()
{
    printf("Here.2\n");
}

int OscSphere::radius_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data)
{
    if (argc!=1)
        return 0;

    OscSphere* me = (OscSphere*)user_data;
	cODESphere *sphere = dynamic_cast<cODESphere*>( me->odePrimitive() );
    if (sphere)
        sphere->setRadius(argv[0]->f);
}
