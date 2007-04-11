
//===========================================================================
/*
    This file is part of a proof-of-concept implementation for using
    Open SoundControl to interact with a haptic virtual environment.

    stephen.sinclair@mail.mcgill.ca
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
