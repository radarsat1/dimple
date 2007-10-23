// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-


#include <lo/lo.h>
#include "Simulation.h"

Simulation::Simulation(int port) : OscBase("world", NULL)
{
    char str[10];
    sprintf(str, "%d", port);
    m_server = lo_server_new(str);
}

Simuluation::~Simulation()
{
    if (m_server)
        lo_server_free(m_server);
}
