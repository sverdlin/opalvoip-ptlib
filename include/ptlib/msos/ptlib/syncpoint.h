/*
 * $Id: syncpoint.h,v 1.1 1998/03/23 02:42:03 robertj Exp $
 *
 * Portable Windows Library
 *
 * User Interface Classes Interface Declarations
 *
 * Copyright 1993 Equivalence
 *
 * $Log: syncpoint.h,v $
 * Revision 1.1  1998/03/23 02:42:03  robertj
 * Initial revision
 *
 */


#ifndef _PSYNCPOINT


///////////////////////////////////////////////////////////////////////////////
// PSyncPoint

#include "../../common/ptlib/syncpoint.h"
  public:
    virtual void Signal();
};


#endif
