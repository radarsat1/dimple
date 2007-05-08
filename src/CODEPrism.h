
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

    static void initCallbackDefaults(void *self);

    //! Change the dimensions of the prism
    void setSize(cVector3d& size);

    //! Change the dimensions of the dynamic prism
    void setDynamicSize(cVector3d& size);

protected:
	//! Hold the size of each dimension
	cVector3d m_size;
	//! Create the prism structure
	void create(bool openbox);
};

#endif // CODEPrismH
