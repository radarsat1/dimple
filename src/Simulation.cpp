// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#include <lo/lo.h>
#include "dimple.h"
#include "Simulation.h"
#include "OscBase.h"

ShapeFactory::ShapeFactory(char *name, Simulation *parent)
    : OscBase(name, parent)
{
}

ShapeFactory::~ShapeFactory()
{
}

PrismFactory::PrismFactory(Simulation *parent)
    : ShapeFactory("prism", parent)
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

SphereFactory::SphereFactory(Simulation *parent)
    : ShapeFactory("sphere", parent)
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
    SphereFactory *me = static_cast<SphereFactory*>(user_data);
    me->create(&argv[0]->s, argv[1]->f);
    return 0;
}

Simulation::Simulation(const char *port)
    : OscBase("world", NULL, lo_server_new(port, NULL))
{
    m_bDone = false;
    if (pthread_create(&m_thread, NULL, Simulation::run, this))
    {
        printf("Error creating simulation thread.");
        m_thread = 0;
    }
}

Simulation::~Simulation()
{
    printf("Ending simulation... ");
    m_bDone = true;
    if (m_thread)
        pthread_join(m_thread, NULL);

    if (m_server)
        lo_server_free(m_server);
    printf("done.\n");
}

void* Simulation::run(void* param)
{
    Simulation* me = static_cast<Simulation*>(param);

    printf("Simulation running.\n");

    while (!me->m_bDone)
    {
        lo_server_recv_noblock(me->m_server, 100);
    }

    return 0;
}
