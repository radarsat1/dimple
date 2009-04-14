// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-
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

const char *address_send_host = "localhost";
const char *address_send_port = "7775";

lo_address address_send;
int quit = 0;

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
#endif

     address_send = lo_address_new(address_send_host, address_send_port);

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
