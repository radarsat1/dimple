// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-


#include <lo/lo.h>
#include "Simulation.h"
#include "OscBase.h"

ShapeFactory::ShapeFactory(char *name, OscBase *parent)
    : OscBase(name, parent)
{
}

ShapeFactory::~ShapeFactory()
{
}

PrismFactory::PrismFactory(char *name, OscBase *parent)
    : ShapeFactory(name, parent)
{
}

PrismFactory::~PrismFactory()
{
}

SphereFactory::SphereFactory(char *name, OscBase *parent)
    : ShapeFactory(name, parent)
{
}

SphereFactory::~SphereFactory()
{
}

Simulation::Simulation(int port)
    : OscBase("world", NULL),
      m_prismFactory("prism", this),
      m_sphereFactory("sphere", this)
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
