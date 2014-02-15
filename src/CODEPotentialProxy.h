
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
#ifndef CODEPotentialProxyH
#define CODEPotentialProxyH
//---------------------------------------------------------------------------
#include <forces/CPotentialFieldForceAlgo.h>
//---------------------------------------------------------------------------

//===========================================================================
/*!
      \file     CODEPotentialProxy.h
      \class    cODEPotentialProxy
      \brief    cODEPotentialProxy is an abstract class for algorithms
                that compute single point force contacts.
*/
//===========================================================================
class cODEPotentialProxy : public cPotentialFieldForceAlgo
{
  public:
    // CONSTRUCTOR & DESTRUCTOR:
    //! A constructor that copies relevant initialization state from another proxy
    cODEPotentialProxy(cPotentialFieldForceAlgo* a_oldProxy)
    {
        /*
        m_deviceGlobalPos = a_oldProxy->getDeviceGlobalPosition();
        m_proxyGlobalPos = a_oldProxy->getProxyGlobalPosition();
        m_lastGlobalForce.zero();
        m_radius = a_oldProxy->getProxyRadius();
        */
        m_lastContactObject = NULL;
        m_lastContactPoint = cVector3d(0,0,0);
        m_world = a_oldProxy->getWorld();
    }
    //! Destructor of cODEPotentialProxy.
    //virtual ~cODEPotentialProxy() {};

    // METHODS:
    //! Initialize the algorithm by passing the initial position of the device.
    //virtual void initialize(cWorld* a_world, const cVector3d& a_initialPos) { m_world = a_world; };
    //! Compute the next force given the updated position of the device.
    virtual cVector3d computeForces(const cVector3d& a_nextDevicePos);

    virtual cGenericObject *getContactObject() { return m_lastContactObject; }
    virtual cVector3d& getContactPoint() { return m_lastContactPoint; }

protected:
        cGenericObject *m_lastContactObject;
        cVector3d m_lastContactPoint;
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
