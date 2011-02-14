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

char *address_send_url = "osc.udp://localhost:7775";

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
           "                    Default is %d.\n", DEFAULT_QUEUE_SIZE);
}

void parse_command_line(int argc, char* argv[])
{
    int c=0;

    struct option long_options[] = {
        { "help",       no_argument,       0, 'h' },
        { "send-url",   optional_argument, 0, 'u' },
        { "queue-size", optional_argument, 0, 'q' },
        {0, 0, 0, 0}
    };

    while (c!=-1) {
        int option_index = 0;

        c = getopt_long (argc, argv, "hu:q:",
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

     address_send = lo_address_new_from_url(address_send_url);
     if (!address_send) {
         printf("Unable to open OSC URL for sending: %s\n", address_send_url);
         exit(1);
     }
     printf("URL opened for sending: %s\n", address_send_url);

     try {

     PhysicsSim   physics   ("7771");
     HapticsSim   haptics   ("7772");
     VisualSim    visual    ("7773");
     InterfaceSim interface ("7774");

     // Physics can change object positions
     // in any of the other simulations
     physics.add_simulation(haptics);
     physics.add_simulation(visual);

     // Haptics can add force to objects
     // in the physics simulation.
     haptics.add_simulation(physics);
     haptics.add_simulation(visual);

     // Visual can send a message to haptics due to keyboard
     // shortcuts. (e.g. reset_workspace.)
     visual.add_simulation(haptics);

     // Interface can modify anything in
     // any other simulation.
     interface.add_simulation(physics);
     interface.add_simulation(haptics);
     interface.add_simulation(visual);

     // Start all simulations
     physics.start();
     haptics.start();
     interface.start();
     visual.run_unthreaded();

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

	 return 0;
}

void dimple_cleanup()
{
	quit = 1;
}

#if defined(WIN32) && defined(PTW32_STATIC_LIB)
class deal_with_pthread_win32
{
public:
	deal_with_pthread_win32() {
		/* initialize win32 pthreads */
		pthread_win32_process_attach_np();
	}
	~deal_with_pthread_win32() {
		/* clean up win32 pthreads */
		pthread_win32_process_detach_np();
	}
} _deal_with_pthread_win32;
#endif

//---------------------------------------------------------------------------
