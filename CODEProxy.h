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
    \author:    Chris Sewell
    \version    1.0
    \date       06/2004
*/
//===========================================================================

#if !defined(_CODE_PROXY_H_)
#define _CODE_PROXY_H_

#include "CProxyPointForceAlgo.h"

// A slightly modified proxy algorithm is required to support ODE;
// we override the relevant functions in a subclass...
class cODEProxy : public cProxyPointForceAlgo
{    
  
protected:
    //! Remove the approximate test for whether the proxy has reached the goal point; use exact distances
    virtual bool goalAchieved(const cVector3d& a_proxy, const cVector3d& a_goal) const
    {
        // Always fail this test to force the proxy to continue...
        return false;
    }

    //! Remove the offsetting of the goal to account for proxy volume; use exact collision vectors
    virtual void offsetGoalPosition(cVector3d& a_goal, const cVector3d& a_proxy) const
    {
        // Leave the goal where it is...
        return;
    }
  
public:
    //! A constructor that copies relevant initialization state from another proxy
    cODEProxy(cProxyPointForceAlgo* a_oldProxy)
    {
        m_deviceGlobalPos = a_oldProxy->getDeviceGlobalPosition();
        m_proxyGlobalPos = a_oldProxy->getProxyGlobalPosition();
        m_lastGlobalForce.zero();
        m_world = a_oldProxy->getWorld();
        m_radius = a_oldProxy->getProxyRadius();
    }
};

#endif // _CODE_PROXY_H_
