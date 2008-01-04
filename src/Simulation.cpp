// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-


#include <lo/lo.h>
#include "Simulation.h"
#include "OscBase.h"

ShapeFactory::ShapeFactory(char *name, Simulation *parent)
    : OscBase(name, parent)
{
}

ShapeFactory::~ShapeFactory()
{
}

PrismFactory::PrismFactory(char *name, Simulation *parent)
    : ShapeFactory(name, parent)
{
    // Name, Width, Height, Depth
    addHandler("create", "sfff", create_handler);
}

PrismFactory::~PrismFactory()
{
}

int PrismFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                 int argc, void *data, void *user_data)
{
}

SphereFactory::SphereFactory(char *name, Simulation *parent)
    : ShapeFactory(name, parent)
{
    // Name, Radius
    addHandler("create", "sf", create_handler);
}

SphereFactory::~SphereFactory()
{
}

int SphereFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
    SphereFactory *me = (SphereFactory*)user_data;
    printf("SphereFactory (%s) is creating a sphere object.\n", me->m_parent->c_name());
}


Simulation::Simulation(const char *port)
    : OscBase("world", NULL, lo_server_new(port, NULL)),
      m_prismFactory("prism", this),
      m_sphereFactory("sphere", this)
{
}

Simulation::~Simulation()
{
    if (m_server)
        lo_server_free(m_server);
}
