/*
 * videoio1394avc.h
 *
 * This file is a based on videoio1394dc.h
 *
 * Portable Tools Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Tools Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Georgi Georgiev <chutz@gg3.net>
 *
 */


#ifndef _PVIDEOIO1394AVC

#define _PVIDEOIO1394AVC

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <sys/utsname.h>
#include <libraw1394/raw1394.h>
#include <libavc1394/avc1394.h>
#include <libavc1394/rom1394.h>
#include <libraw1394/csr.h>
#include <libdv/dv.h>

#include <ptlib.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptlib/file.h>
#if !P_USE_INLINES
#include <ptlib/contain.inl>
#endif
#include <ptclib/delaychan.h>


/** This class defines a video input device that
    generates fictitous image data.
*/

class PVideoInputDevice_1394AVC : public PVideoInputDevice
{
    PCLASSINFO(PVideoInputDevice_1394AVC, PVideoInputDevice);
 public:
  /** Create a new video input device.
   */
    PVideoInputDevice_1394AVC();

    /**Close the video input device on destruction.
      */
    ~PVideoInputDevice_1394AVC();

    /**Open the device given the device name.
      */
    bool Open(
      const PString & deviceName,   /// Device name to open
      bool startImmediate = true    /// Immediately start device
    );

    /**Determine of the device is currently open.
      */
    bool IsOpen();

    /**Close the device.
      */
    bool Close();

    /**Start the video device I/O.
      */
    bool Start();

    /**Stop the video device I/O capture.
      */
    bool Stop();

    /**Determine if the video device I/O capture is in progress.
      */
    bool IsCapturing();

    /**Get a list of all of the drivers available.
      */
    static PStringArray GetInputDeviceNames();

    PStringArray GetDeviceNames() const
    { return GetInputDeviceNames(); }

    /**Get the maximum frame size in bytes.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    PINDEX GetMaxFrameBytes();

    /**Get the minimum & maximum size of a frame on the device.
    */
    bool GetFrameSizeLimits(
      unsigned & minWidth,   /// Variable to receive minimum width
      unsigned & minHeight,  /// Variable to receive minimum height
      unsigned & maxWidth,   /// Variable to receive maximum width
      unsigned & maxHeight   /// Variable to receive maximum height
    ) ;

    void ClearMapping();

    int GetNumChannels();
    bool SetChannel(
         int channelNumber  /// New channel number for device.
    );
    bool SetFrameRate(
      unsigned rate  /// Frames  per second
    );
    bool SetVideoFormat(
      VideoFormat videoFormat   /// New video format
    );
    bool SetFrameSize(
      unsigned width,   /// New width of frame
      unsigned height   /// New height of frame
    );
    bool SetColourFormat(
      const PString & colourFormat   // New colour format for device.
    );


    /**Try all known video formats & see which ones are accepted by the video driver
     */
    bool TestAllFormats();


 protected:
    virtual bool InternalGetFrameData(uint8_t * buffer, PINDEX & bytesReturned, bool & keyFrame, bool wait);

    raw1394handle_t handle;
    bool is_capturing;
    bool UseDMA;
    PINDEX frameBytes;
    int port;
    PAdaptiveDelay m_pacing;

    bool SetupHandle();
};

#endif


// End Of File ///////////////////////////////////////////////////////////////
