/*
 * svcproc.h
 *
 * Service Process (daemon) class.
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

#ifndef PTLIB_SERVICEPROCESS_H
#define PTLIB_SERVICEPROCESS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/pprocess.h>
#include <ptlib/syslog.h>



/** A process type that runs as a "background" service.
    This may be a service under the Windows NT operating system, or a "daemon" under Unix, or a hidden application under Windows.
 */
class PServiceProcess : public PProcess
{
  PCLASSINFO(PServiceProcess, PProcess);

  public:
  /**@name Construction */
  //@{
    /** Create a new service process.
     */
    PServiceProcess(
      const char * manuf,     ///< Name of manufacturer
      const char * name,      ///< Name of product
      unsigned majorVersion,  ///< Major version number of the product
      unsigned minorVersion,  ///< Minor version number of the product
      CodeStatus status,      ///< Development status of the product
      unsigned patchVersion,  ///< Patch version number of the product
      unsigned oemVersion = 0 ///< OEM version number of the product
    );
  //@}

    /** User override function for the main execution routine of the thread. A
       descendent class could provide the code that will be executed in the
       thread within this function.
       
       Default behaviour is to wait until m_exitMain is signalled.
     */
    virtual void Main() override;

  /**@name Callback functions */
  //@{
    /** Called when the service is started. This typically initialises the
       service and returns true if the service is ready to run. The
       <code>Main()</code> function is then executed.

       @return
       true if service may start, false if an initialisation failure occurred.
     */
    virtual bool OnStart() = 0;

    /** Called by the system when the service is stopped. One return from this
       function there is no guarentee that any more user code will be executed.
       Any cleaning up or closing of resource must be done in here.

       Default behaviour signals the m_exitMain to exit Main().
     */
    virtual void OnStop();

    /** Called by the system when the service is to be paused. This will
       suspend any actions that the service may be executing. Usually this is
       less expensive in resource allocation etc than stopping and starting
       the service.

       @return
       true if the service was successfully paused.
     */
    virtual bool OnPause();

    /** Resume after the service was paused.
     */
    virtual void OnContinue();

    /** The Control menu option was used in the SysTray menu.
     */
    virtual void OnControl();
  //@}

  /**@name Miscellaneous functions */
  //@{
    /** Get the current service process object.

       @return
       Pointer to service process.
     */
    static PServiceProcess & Current();

#if P_SYSTEMLOG
    /** Set the level at which errors are logged. Only messages higher than or
       equal to the specified level will be logged.
    
       The default is PSystemLog::Error allowing fatal errors and ordinary\
       errors to be logged and warning and information to be ignored.

       If in debug mode then the default is PSystemLog::Info allowing all
       messages to be displayed.
     */
    void SetLogLevel(
      PSystemLog::Level level  ///< New log level
    );

    /** Get the current level for logging.

       @return
       Log level.
     */
    PSystemLog::Level GetLogLevel() const { return PSystemLog::GetTarget().GetThresholdLevel(); }
#endif
  //@}


    /* Internal initialisation function called directly from
       <code>main()</code>. The user should never call this function.
     */
    virtual int InternalMain(int argc, char * argv[], void * hInstance) override;

    virtual bool IsServiceProcess() const override;

  protected:
  // Member variables
    PSyncPoint m_exitMain;

    /// Flag to indicate service is run in simulation mode.
    bool m_debugMode;

// Include platform dependent part of class
#ifdef _WIN32
    #undef PCREATE_PROCESS
    #define PCREATE_PROCESS(cls) \
      extern "C" int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) \
        { return std::make_unique<cls>()->InternalMain(__argc, __argv, hInstance); }

  public:
    virtual const char * GetServiceDependencies() const;
    // Get a set of null terminated strings terminated with double null.

    void SetDescription(const PString & description);
    const PString & GetDescription() const;

  protected:
    bool m_debugHidden; /// Flag to indicate service is run in simulation mode without showing the control window 

  private:
    static void __stdcall StaticMainEntry(DWORD argc, LPTSTR * argv);
    /* Internal function called from the Service Manager. This simply calls the
    <A>MainEntry()</A> function on the PServiceProcess instance.
    */

    void MainEntry(uint32_t argc, LPTSTR * argv);
    /* Internal function function that takes care of actually starting the
    service, informing the service controller at each step along the way.
    After launching the worker thread, it waits on the event that the worker
    thread will signal at its termination.
    */

    void ThreadEntry();
    /* Internal function function that starts the worker thread for the
    service.
    */

    static void __stdcall StaticControlEntry(DWORD code);
    /* This function is called by the Service Controller whenever someone calls
    ControlService in reference to our service.
    */

    void ControlEntry(uint32_t code);
    /* This function is called by the Service Controller whenever someone calls
    ControlService in reference to our service.
    */

    bool ReportStatus(
      uint32_t dwCurrentState,
      uint32_t dwWin32ExitCode = NO_ERROR,
      uint32_t dwCheckPoint = 0,
      uint32_t dwWaitHint = 0
    );
    /* This function is called by the Main() and Control() functions to update the
    service's status to the service control manager.
    */


    bool ProcessCommand(const char * cmd);
    // Process command line argument for controlling the service.

    bool CreateControlWindow(bool createDebugWindow);
    static LPARAM WINAPI StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LPARAM WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void DebugOutput(const char * out);

    class LogToWindow : public PSystemLogTarget
    {
      virtual void Output(PSystemLog::Level level, const char * msg);
    };
    friend class LogToWindow;

    SERVICE_STATUS        m_status;
    SERVICE_STATUS_HANDLE m_statusHandle;
    HANDLE                m_startedEvent;
    HANDLE                m_terminationEvent;
    HWND                  m_controlWindow;
    HWND                  m_debugWindow;

    PString               m_description;
#else
  public:
    ~PServiceProcess();
    virtual void AddRunTimeSignalHandlers(const int * signals = NULL) override;
    virtual void AsynchronousRunTimeSignal(int signal, PProcessIdentifier source) override;
    virtual void HandleRunTimeSignal(int signal) override;
  protected:
    int  InitialiseService();
    PString pidFileToRemove;
    bool isTerminating;
#endif
};


#endif // PTLIB_SERVICEPROCESS_H


// End Of File ///////////////////////////////////////////////////////////////
