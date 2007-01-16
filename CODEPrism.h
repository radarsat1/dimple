

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

protected:
	//! Hold the size of each dimension
	cVector3d m_size;
	//! Create the prism structure
	void create(bool openbox);
};

#endif // CODEPrismH
