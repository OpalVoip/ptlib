/*
 * serchan.h
 *
 * Asynchronous Serial I/O channel class.
 *
 * Portable Tools Library
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
 * The Original Code is Portable Tools Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef PTLIB_SERIALCHANNEL_H
#define PTLIB_SERIALCHANNEL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptlib/channel.h>

#if HAVE_TERMIOS_H
  #include <termios.h>
#elif HAVE_SYS_TERMIOS_H
  #include <sys/termios.h>
#endif


class PConfig;


///////////////////////////////////////////////////////////////////////////////
// Serial Channel

/**This class defines an I/O channel that communicates via a serial port. This
   is usually an RS-232 port.
 */
class PSerialChannel : public PChannel
{
  PCLASSINFO(PSerialChannel, PChannel);

  public:
  /**@name Construction */
  //@{
    /// Create a new serial channel object, but do not open it.
    PSerialChannel();

    /// Configuration of serial port parity options.
    enum Parity {
      /// Use the default parity, ie do not change it.
      DefaultParity,    
      /// Set the port for no parity bit.
      NoParity,         
      /// Set the port to generate parity and make it even.
      EvenParity,       
      /// Set the port to generate parity and make it odd.
      OddParity,        
      /// Set the port parity bit to mark only.
      MarkParity,       
      /// Set the port parity bit to space only.
      SpaceParity       
    };

    /// Configuration of serial port flow control options.
    enum FlowControl {
      /// Use the default flow control, ie do not change it.
      DefaultFlowControl,
      /// Set the port for no flow control.
      NoFlowControl,    
      /// Set the port for software or XON/XOFF flow control.
      XonXoff,         
      /// Set the port for hardware or RTS/CTS flow control.
      RtsCts           
    };

    /**Create a serial channal.
       The channel is opened it on the specified port and with the specified
       attributes.
     */
    PSerialChannel(
      /**The name of the serial port to connect to. This is a platform
         dependent string and woiuld rarely be a literal. The static function
         <code>GetPortNames()</code> can be used to find the platforms serial ports.
       */
      const PString & port,
      /**Serial port speed or baud rate. The actual values possible here are
         platform dependent, but the standard value of 300, 1200, 2400, 4800,
         9600, 19200, 38400 always be legal.
       */
      uint32_t speed = 0,
      /**Number of data bits for serial port. The actual values possible here
         are platform dependent, but 7 and 8 should always be legal.
       */
      uint8_t data = 0,
      /**Parity for serial port. The actual values possible here are platform
         dependent, but <code>NoParity</code>, <code>OddParity</code> and
         <code>EvenParity</code> should always be legal.
       */
      Parity parity = DefaultParity,
      /**Number of stop bits for serial port. The actual values possible here
         are platform dependent, but 1 and 2 should always be legal.
       */
      uint8_t stop = 0,
      /**Flow control for data from the remote system into this conputer.*/
      FlowControl inputFlow = DefaultFlowControl,
      /**Flow control for data from this conputer out to remote system. */
      FlowControl outputFlow = DefaultFlowControl      
    );

#if P_CONFIG_FILE
    /**Create a serial channal.
       The channel is opened using attributes obtained from standard variables
       in the configuration file. Note that it assumed that the correct
       configuration file section is already set.
     */
    PSerialChannel(
      PConfig & cfg  ///< Configuration file to read serial port attributes from.
    );
#endif // P_CONFIG_FILE

    /// Close the serial channel on destruction.
    ~PSerialChannel();
  //@}


  /**@name Open functions */
  //@{
    /** Get the platform and I/O channel type name of the channel. For example,
       it would return the filename in <code>PFile</code> type channels.

       @return the name of the channel.
     */
    virtual PString GetName() const override = 0;

    /**Open a serial channal.
       The channel is opened it on the specified port and with the specified
       attributes.
     */
    virtual bool Open(
      /**The name of the serial port to connect to. This is a platform
         dependent string and woiuld rarely be a literal. The static function
         <code>GetPortNames()</code> can be used to find the platforms serial ports.
       */
      const PString & port,
      /**Serial port speed or baud rate. The actual values possible here are
         platform dependent, but the standard value of 300, 1200, 2400, 4800,
         9600, 19200, 38400 always be legal.
       */
      uint32_t speed = 0,
      /**Number of data bits for serial port. The actual values possible here
         are platform dependent, but 7 and 8 should always be legal.
       */
      uint8_t data = 0,
      /**Parity for serial port. The actual values possible here are platform
         dependent, but <code>NoParity</code>, <code>OddParity</code> and
         <code>EvenParity</code> should always be legal.
       */
      Parity parity = DefaultParity,
      /**Number of stop bits for serial port. The actual values possible here
         are platform dependent, but 1 and 2 should always be legal.
       */
      uint8_t stop = 0,
      /**Flow control for data from the remote system into this conputer.*/
      FlowControl inputFlow = DefaultFlowControl,
      /**Flow control for data from this conputer out to remote system. */
      FlowControl outputFlow = DefaultFlowControl      
    );

#if P_CONFIG_FILE
    /**Open a serial channal.
       The channel is opened using attributes obtained from standard variables
       in the configuration file. Note that it assumed that the correct
       configuration file section is already set.
     */
    virtual bool Open(
      PConfig & cfg  ///< Configuration file to read serial port attributes from.
    );
#endif // P_CONFIG_FILE

    /// <summary>
    ///  Close the channel
    /// </summary>
    /// <returns></returns>
    virtual bool Close() override;

    /**Get a list of the available serial ports. This returns a set of
       platform dependent strings which describe the serial ports of the
       computer. For example under unix it may be "ttyS0", under MS-DOS or
       NT it would be "COM1" and for the Macintosh it could be "Modem".

       @return
       list of strings for possible serial ports.
     */
    static PStringList GetPortNames();
  //@}


  /**@name Configuration functions */
  //@{
    /**Set the speed (baud rate) of the serial channel.

       @return
       true if the change was successfully made.
     */
    bool SetSpeed(
      uint32_t speed   ///< New speed for serial channel.
    );

    /**Get the speed (baud rate) of the serial channel.

       @return
       current setting.
     */
    uint32_t GetSpeed() const;

    /**Set the data bits (5, 6, 7 or 8) of the serial port.

       @return
       true if the change was successfully made.
     */
    bool SetDataBits(
      uint8_t data   ///< New number of data bits for serial channel.
    );

    /**Get the data bits (5, 6, 7 or 8) of the serial port.

       @return
       current setting.
     */
    uint8_t GetDataBits() const;

    /**Set the parity of the serial port.

       @return
       true if the change was successfully made.
     */
    bool SetParity(
      Parity parity   ///< New parity option for serial channel.
    );

    /**Get the parity of the serial port.

       @return
       current setting.
     */
    Parity GetParity() const;

    /**Set the stop bits (1 or 2) of the serial port.

       @return
       true if the change was successfully made.
     */
    bool SetStopBits(
      uint8_t stop   ///< New number of stop bits for serial channel.
    );

    /**Get the stop bits (1 or 2) of the serial port.

       @return
       current setting.
     */
    uint8_t GetStopBits() const;

    /**Set the flow control (handshaking) protocol of the input to the serial
       port.

       @return
       true if the change was successfully made.
     */
    bool SetInputFlowControl(
      FlowControl flowControl   ///< New flow control for serial channel input.
    );

    /**Get the flow control (handshaking) protocol of the input to the serial
       port.

       @return
       current setting.
     */
    FlowControl GetInputFlowControl() const;

    /**Set the flow control (handshaking) protocol of the output to the serial
       port.

       @return
       true if the change was successfully made.
     */
    bool SetOutputFlowControl(
      FlowControl flowControl   ///< New flow control for serial channel output.
    );

    /**Get the flow control (handshaking) protocol of the output from the
       serial port.

       @return
       current setting.
     */
    FlowControl GetOutputFlowControl() const;

#if P_CONFIG_FILE
    /**Save the current port settings into the configuration file. Note that
       it assumed that the correct configuration file section is already set.
     */
    virtual void SaveSettings(
      PConfig & cfg   ///< Configuration file to save setting into.
    );
#endif // P_CONFIG_FILE
  //@}


  /**@name Status functions */
  //@{
    /** Set the Data Terminal Ready signal of the serial port. */
    void SetDTR(
      bool state = true   ///< New state of the DTR signal.
    );

    /**Clear the Data Terminal Ready signal of the serial port. This is
       equivalent to <code>SetDTR(false)</code>.
     */
    void ClearDTR();

    /**Set the Request To Send signal of the serial port. */
    void SetRTS(
      bool state = true   ///< New state of the RTS signal.
    );

    /**Clear the Request To Send signal of the serial port. This is equivalent
       to <code>SetRTS(false)</code>.
     */
    void ClearRTS();

    /** Set the break condition of the serial port. */
    void SetBreak(
      bool state = true   ///< New state of the serial port break condition.
    );

    /**Clear the break condition of the serial port. This is equivalent to
       <code>SetBreak(false)</code>.
     */
    void ClearBreak();

    /**Get the Clear To Send signal of the serial port.
    
       @return
       true if the CTS signal is asserted.
     */
    bool GetCTS();

    /**Get the Data Set Ready signal of the serial port.
    
       @return
       true if the DSR signal is asserted.
     */
    bool GetDSR();

    /**Get the Data Carrier Detect signal of the serial port.
    
       @return
       true if the DCD signal is asserted.
     */
    bool GetDCD();

    /**Get the Ring Indicator signal of the serial port.
    
       @return
       true if the RI signal is asserted.
     */
    bool GetRing();
  //@}


  private:
    void Construct();
    // Platform dependent construct of the serial channel.

    PString m_portName;

// Include platform dependent part of class
#ifdef _WIN32
  public:
    virtual bool Read(void * buf, PINDEX len);
    virtual bool Write(const void * buf, PINDEX len);

  private:
    bool SetCommsParam(uint32_t speed, uint8_t data, Parity parity, uint8_t stop, FlowControl inputFlow, FlowControl outputFlow);

    HANDLE m_commsResource;
    enum { InputQueueSize = 2048, OutputQueueSize = 1024 };
    DCB m_deviceControlBlock;
#else
  private:
    struct termios m_oldTermio;
    struct termios m_currentTermio;
    uint32_t m_baudRate;
    uint8_t  m_dataBits;
    Parity   m_parityBits;
    uint8_t  m_stopBits;
#endif
};


#endif // PTLIB_SERIALCHANNEL_H


// End Of File ///////////////////////////////////////////////////////////////
