// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-
//======================================================================================
/*
    This file is part of DIMPLE, the Dynamic Interactive Musically PhysicaL Environment,

    This code is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License("GPL") version 2
    as published by the Free Software Foundation.  See the file LICENSE
    for more information.

    sinclair@music.mcgill.ca
    http://www.music.mcgill.ca/~sinclair/content/dimple
*/
//======================================================================================

#define _WINSOCKAPI_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>

#include "config.h"
#include "dimple.h"

#include "PhysicsSim.h"
#include "HapticsSim.h"
#include "VisualSim.h"
#include "InterfaceSim.h"

/** Defaults for global variables **/
int visual_fps = 30;
int visual_timestep_ms = (int)((1.0/visual_fps)*1000.0);
int physics_timestep_ms = 10;
int haptics_timestep_ms = 1;
int msg_queue_size = DEFAULT_QUEUE_SIZE*1024;
bool force_enabled = true;
const char *interface_port_str = "7774";

static struct {
    const char *visual, *haptics, *physics;
} sim_spec = { "", "", "" };

const char *address_send_url = "osc.udp://localhost:%u";

lo_address address_send;
int quit = 0;

void help()
{
    printf("Usage: dimple [options], where options are:\n\n"
        "--send-url (-u)     A LibLo-style URL specifying the address\n"
        "                    to send OSC messages to.\n"
        "                    Example: osc.udp://localhost:9000\n"
        "                    Other protocols may be 'tcp', 'unix'.\n\n");
    printf("--queue-size (-q)   Size of the message queues in kB.\n"
           "                    Default is %d.\n\n", DEFAULT_QUEUE_SIZE);
    printf("--sim (-s)  A string specifying which simulations to enable.\n"
           "            `v' for visual, `p' for physics, `h' for haptics.\n"
           "            May be followed by ',' and a Liblo-style URL,\n"
           "            indicating that these components be addressed\n"
           "            remotely. Multiple -s flags may be provided for\n"
           "            different addresses. Defaults to \"vph\".\n\n");
    printf("--port (-p)  A local port number for the OSC/UDP interface.\n"
           "             (i.e. where external applications should send\n"
           "             messages to communicate with DIMPLE.)  Defaults\n"
           "             to 7774.  Ports for physics, haptics and visual\n"
           "             simulations are consecutive following this number,\n"
           "             respectively.\n\n");
    printf("--noforce (-n)  Disable force output to haptic device.\n");
}

void parse_command_line(int argc, char* argv[])
{
    int c=0;
    const char *s, *u;

    struct option long_options[] = {
        { "help",       no_argument,       0, 'h' },
        { "send-url",   required_argument, 0, 'u' },
        { "queue-size", required_argument, 0, 'q' },
        { "sim",        required_argument, 0, 's' },
        { "port",       required_argument, 0, 'p' },
        { "connect",    required_argument, 0, 'c' },
        { "noforce",    no_argument,       0, 'n' },
        {0, 0, 0, 0}
    };

    while (c!=-1) {
        int option_index = 0;

        c = getopt_long (argc, argv, "hu:q:s:p:c:n",
                         long_options, &option_index);

        switch (c) {
        case 'u':
            if (optarg==0) {
                printf("Error parsing --send-url option.\n");
                exit(1);
            }
            address_send_url = optarg;
            break;
        case 'q':
            if (optarg==0 || atoi(optarg)<=0) {
                printf("Error parsing --queue-size option, "
                       "must be an integer > 0.\n");
                exit(1);
            }
            msg_queue_size = atoi(optarg)*1024;
            break;
        case 's':
            s = optarg;
            u = "local";
            while (*s++ != 0) {
                if (*s == ',') {
                    u = s+1;
                    break;
                }
            }
            s = optarg;
            while (*s != 0 && *s != ',') {
                switch (*s++) {
                case 'v': sim_spec.visual = u; break;
                case 'h': sim_spec.haptics = u; break;
                case 'p': sim_spec.physics = u; break;
                }
            };
            break;
        case 'p':
            interface_port_str = optarg;
            break;
        case 'n':
            force_enabled = false;
            break;
        case 'h':
            help();
            exit(0);
            break;
        case '?':
            ;//exit(1);
            break;
        case -1:
            break;
        default:
            exit(1);
        }
    }

    // If all simulations are disabled, enable all of them locally
    if (sim_spec.visual[0] == '\0'
        && sim_spec.haptics[0] == '\0'
        && sim_spec.physics[0] == '\0')
        sim_spec.visual = sim_spec.haptics = sim_spec.physics = "local";
}

void sighandler_quit(int sig)
{
    quit = 1;
    return;
}

#ifdef FLEXT_SYS  // Needed for initialization from Flext object
int dimple_main(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	 // display pretty message
	 printf ("\n");
	 printf ("  ==========================================================\n");
	 printf ("  DIMPLE: Dynamic Interactive Musically PhysicaL Environment\n");
	 printf ("  %-29s Stephen Sinclair, IDMIL %s\n",
	         "Version " DIMPLE_VERSION " (beta).",
	         strrchr(__DATE__,' ')+1);
	 printf ("  ==========================================================\n");
	 printf ("\n");
     fflush(stdout);

#ifndef FLEXT_SYS
	 signal(SIGINT, sighandler_quit);
     parse_command_line(argc, argv);
#endif

     unsigned int interface_port = atoi(interface_port_str);

     char address_send_url_fmt[256];
     snprintf(address_send_url_fmt, 256, address_send_url, interface_port+4);
     address_send = lo_address_new_from_url(address_send_url_fmt);
     if (!address_send) {
         printf("Unable to open OSC URL for sending: %s\n", address_send_url_fmt);
         exit(1);
     }
     printf("URL opened for sending: %s\n", address_send_url_fmt);

     Simulation *physics = NULL;
     Simulation *haptics = NULL;
     Simulation *visual = NULL;

     try {

     char port_str[256];
     snprintf(port_str, 256, "%u", interface_port);
     InterfaceSim interface (port_str);

     if (strcmp(sim_spec.physics, "local")==0) {
         snprintf(port_str, 256, "%u", interface_port+1);
         physics = new PhysicsSim(port_str);
     }

     if (strcmp(sim_spec.haptics, "local")==0) {
         snprintf(port_str, 256, "%u", interface_port+2);
         haptics = new HapticsSim(port_str);
         ((HapticsSim*)haptics)->m_forceEnabled = force_enabled;
     }

     if (strcmp(sim_spec.visual, "local")==0) {
         snprintf(port_str, 256, "%u", interface_port+3);
         visual = new VisualSim(port_str);
     }

     // Physics can change object positions
     // in any of the other simulations
     if (physics) {
         physics->add_receiver( haptics, sim_spec.haptics, Simulation::ST_HAPTICS, true );
         physics->add_receiver( visual,  sim_spec.visual,  Simulation::ST_VISUAL,  true );
     }

     // Haptics can add force to objects
     // in the physics simulation.
     if (haptics) {
         haptics->add_receiver( physics, sim_spec.physics, Simulation::ST_PHYSICS, true );
         haptics->add_receiver( visual,  sim_spec.visual,  Simulation::ST_VISUAL,  true );
     }

     // Visual can send a message to haptics due to keyboard
     // shortcuts. (e.g. reset_workspace.)
     if (visual) {
         visual->add_receiver( haptics, sim_spec.haptics,  Simulation::ST_HAPTICS, true );
     }

     // Interface can modify anything in
     // any other simulation.
     interface.add_receiver( physics, sim_spec.physics, Simulation::ST_PHYSICS, true );
     interface.add_receiver( haptics, sim_spec.haptics, Simulation::ST_HAPTICS, true );
     interface.add_receiver( visual,  sim_spec.visual,  Simulation::ST_VISUAL,  true );

     // Start all simulations
     bool rc = true;
     if (physics) rc &= physics->start();
     if (haptics) { rc &= haptics->start(); Sleep(1000); }
     rc &= interface.start();

     // Above: even after adding the intiailization semaphore, STILL
     // some cases where there's a mysterious race condition for
     // OpenHaptics?!?!  Hence the Sleep(1000).  Lame.  Only happens
     // when run with args, "-s hv".

     // Re-install interrupt handler -- sometimes it is overridden by
     // the haptics driver.
     signal(SIGINT, sighandler_quit);

     if (!rc)
         quit = 1;
     else
         if (visual) visual->run_unthreaded();

#ifndef FLEXT_SYS
	 // initially loop just waiting for messages
	 while (!quit) {
		  Sleep(100);
	 }
#endif

     }
     catch (const char* s) {
         printf("Error:  %s\n", s);
         return 1;
     }

#ifndef FLEXT_SYS
	 dimple_cleanup();
#endif

     if (physics) delete physics;
     if (visual) delete visual;
     if (haptics) delete haptics;

	 return 0;
}

void dimple_cleanup()
{
	quit = 1;
}

//---------------------------------------------------------------------------
