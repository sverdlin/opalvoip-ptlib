/*
 * debstrm.h
 *
 * Debug output stream for Windows.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */


#ifndef _PDEBUGSTREAM
#define _PDEBUGSTREAM

///////////////////////////////////////////////////////////////////////////////
// PDebugStream for MS-Windows

class PDebugStream : public ostream {
  public:
    PDebugStream();

  private:
    class Buffer : public std::streambuf {
      public:
        Buffer();
        virtual int overflow(int=EOF);
        virtual int underflow();
        virtual int sync();
        char buffer[250];
    } buffer;
};



#endif


// End Of File ///////////////////////////////////////////////////////////////
