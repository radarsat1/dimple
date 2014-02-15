
//======================================================================================
/*
    This file is part of DIMPLE, the Dynamic Interactive Musically PhysicaL Environment,

    This code is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License("GPL") version 2
    as published by the Free Software Foundation.  See the file LICENSE
    for more information.

    sinclair@music.mcgill.ca
    http://www.music.mcgill.ca/~sinclair/content/dimple

    This file based on example code from CHAI 3D
*/
//======================================================================================

//---------------------------------------------------------------------------
#include "CODEPotentialProxy.h"
#include <scenegraph/CWorld.h>
//---------------------------------------------------------------------------

//===========================================================================
/*!
    Compute forces for all potential field based objects (cGenericPotentialField).

    \fn       cVector3d cPotentialFieldForceAlgo::computeForces(const
              cVector3d& a_nextDevicePos)
    \param    a_nextDevicePos  Next position of haptic device or virtual finger.
*/
//===========================================================================
cVector3d cODEPotentialProxy::computeForces(const cVector3d& a_nextDevicePos)
{
    // initialize force
    cVector3d force;
    force.zero();

    // compute force feedback for all potential field based objects
    if (m_world != NULL)
    {   
        // Descend through child objects to compute interaction forces for all 
        // cGenericPotentialField objects 
        int numObjects = m_world->getNumChildren();
        for (int i=0; i<numObjects; i++)
        {
			 /*
            if (m_world->getChild(i)->getHapticEnabled()) {
				 cVector3d nextForce = m_world->getChild(i)->computeInteractions(a_nextDevicePos, a_nextDeviceVel);
                if (nextForce.lengthsq()!=0) {
                    m_lastContactObject = m_world->getChild(i);
                    // TODO: determine contact point on potential field
                    //       for spheres, this is inconsequential, but for anything else,
                    //       it is needed for calculating torque.
                    //m_lastContactPoint 
                }
                force.add(nextForce);
            }
			 */
        }
    }

    if (force.lengthsq()==0)
        m_lastContactObject = NULL;

    // return result
    return (force);
}

