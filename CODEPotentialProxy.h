
//===========================================================================
/*
    This file is part of a proof-of-concept implementation for using
    Open SoundControl to interact with a haptic virtual environment.

    stephen.sinclair@mail.mcgill.ca
*/
//===========================================================================

//---------------------------------------------------------------------------
#ifndef CODEPotentialProxyH
#define CODEPotentialProxyH
//---------------------------------------------------------------------------
#include <CPotentialFieldForceAlgo.h>
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
