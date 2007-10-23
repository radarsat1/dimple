// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-


#include <lo/lo.h>
#include "Simulation.h"
#include "OscBase.h"

ShapeFactory::ShapeFactory(char *name, char *classname)
    : OscBase(name, classname)
{
}

ShapeFactory::~ShapeFactory()
{
}

PrismFactory::PrismFactory(char *name, char *classname)
    : ShapeFactory(name, classname)
{
}

PrismFactory::~PrismFactory()
{
}

SphereFactory::SphereFactory(char *name, char *classname)
    : ShapeFactory(name, classname)
{
}

SphereFactory::~SphereFactory()
{
}

Simulation::Simulation(int port)
    : OscBase("world", NULL),
      m_prismFactory("prism", "world"),
      m_sphereFactory("sphere", "world")
{
    char str[10];
    sprintf(str, "%d", port);
    m_server = lo_server_new(str, NULL);
}

Simulation::~Simulation()
{
    if (m_server)
        lo_server_free(m_server);
}
