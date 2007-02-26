
//===========================================================================
/*
    This file is part of a proof-of-concept implementation for using
    Open SoundControl to interact with a haptic virtual environment.

    stephen.sinclair@mail.mcgill.ca
*/
//===========================================================================

#ifndef CODEPrismH
#define CODEPrismH

#include "CODEMesh.h"
#include "CVertex.h"

class cODEPrism : public cODEMesh
{
public:
    //! Constructor of cODEPrism.
    cODEPrism(cWorld* a_parent, dWorldID a_odeWorld, dSpaceID a_odeSpace, const cVector3d& a_size);
    //! Destructor of cODEPrism.
    ~cODEPrism();

    static void initCallbackDefaults(cODEPrimitive *self);

    //! Change the dimensions of the prism
    void setSize(cVector3d& size);

protected:
	//! Hold the size of each dimension
	cVector3d m_size;
	//! Create the prism structure
	void create(bool openbox);
};

#endif // CODEPrismH
