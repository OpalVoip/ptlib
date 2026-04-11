/*
 * thread.h
 *
 * Thread of execution control class.
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

///////////////////////////////////////////////////////////////////////////////
// PThread

  public:
    int PXBlockOnChildTerminate(int pid, const PTimeInterval & timeout);
    int PXBlockOnIO(int handle, int type, const PTimeInterval & timeout);
    void PXAbortBlock() const;

    static bool PX_kill(PThreadIdentifier tid, PUniqueThreadIdentifier uid, int sig);

  protected:
    atomic<Priority> PX_priority;
    
#if defined(P_LINUX)
    PTimeInterval     PX_startTick;
    PTimeInterval     PX_endTick;
#endif
    enum PX_states {
      PX_firstResume,
      PX_starting,
      PX_running,
      PX_finishing,
      PX_finished
    };
    atomic<PX_states> PX_state;

    int unblockPipe[2];
    friend class PSocket;


    // End Of File ////////////////////////////////////////////////////////////////
