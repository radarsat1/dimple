//===========================================================================
/*
    This file is part of the CHAI 3D visualization and haptics libraries.
    Copyright (C) 2003-2004 by CHAI 3D. All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License("GPL") version 2
    as published by the Free Software Foundation.

    For using the CHAI 3D libraries with software that can not be combined
    with the GNU GPL, and for taking advantage of the additional benefits
    of our support services, please contact CHAI 3D about acquiring a
    Professional Edition License.

    \author:    <http://www.chai3d.org>
    \author:    Francois Conti
    \version    1.1
    \date       01/2004
*/
//===========================================================================

//---------------------------------------------------------------------------
#include "CODEPotentialProxy.h"
#include "CWorld.h"
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
            cVector3d nextForce = m_world->getChild(i)->computeForces(a_nextDevicePos);
            if (nextForce.lengthsq()!=0) {
                m_lastContactObject = m_world->getChild(i);
                // TODO: determine contact point on potential field
                //       for spheres, this is inconsequential, but for anything else,
                //       it is needed for calculating torque.
                //m_lastContactPoint 
            }
            force.add(nextForce);
        }
    }

    if (force.lengthsq()==0)
        m_lastContactObject = NULL;

    // return result
    return (force);
}

