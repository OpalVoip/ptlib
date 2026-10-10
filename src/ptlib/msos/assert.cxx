/*
 * assert.cxx
 *
 * Function to implement assert clauses.
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

#define P_DISABLE_FACTORY_INSTANCES

#include <ptlib.h>
#include <ptlib/svcproc.h>


///////////////////////////////////////////////////////////////////////////////
// PProcess

#ifdef __MINGW32__

  void PProcess::WaitOnExitConsoleWindow()
  {
  }

  // DbgHelp stack walking is the MSVC path below. MinGW still needs this
  // symbol because PTrace::WalkStack and asserts call it.
  void PPlatformWalkStack(ostream &, PThreadIdentifier, PUniqueThreadIdentifier, unsigned, bool, bool)
  {
  }

#else

  static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM thisProcess)
  {
    char wndClassName[100];
    GetClassName(hWnd, wndClassName, sizeof(wndClassName));
    if (strcmp(wndClassName, "ConsoleWindowClass") != 0)
      return true;

    DWORD wndProcess;
    GetWindowThreadProcessId(hWnd, &wndProcess);
    if (wndProcess != (DWORD)thisProcess)
      return true;

    PTRACE(2, "PTLib", "Awaiting key press on exit.");
    cerr << "\nPress a key to exit . . .";
    cerr.flush();

    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    SetConsoleMode(in, ENABLE_PROCESSED_INPUT);
    FlushConsoleInputBuffer(in);
    char dummy;
    DWORD readBytes;
    ReadConsole(in, &dummy, 1, &readBytes, NULL);
    return false;
  }


  void PProcess::WaitOnExitConsoleWindow()
  {
    if (m_waitOnExitConsoleWindow && !m_library)
      EnumWindows(EnumWindowsProc, GetCurrentProcessId());
  }

  #pragma warning(disable:4091)
  #include <DbgHelp.h>
  #include <Psapi.h>
  #pragma warning(default:4091)

  #if PTRACING
    #define InternalMaxStackWalk PTrace::MaxStackWalk
  #else
    #define InternalMaxStackWalk 20
  #endif // PTRACING

  class PDebugDLL : public PDynaLink
  {
    PCLASSINFO(PDebugDLL, PDynaLink)
    public:
      BOOL (__stdcall *m_SymInitialize)(
        __in HANDLE hProcess,
        __in_opt PCSTR UserSearchPath,
        __in BOOL fInvadeProcess
      );
      BOOL (__stdcall *m_SymCleanup)(
        __in HANDLE hProcess
      );
      DWORD (__stdcall *m_SymGetOptions)(
        VOID
      );
      DWORD (__stdcall *m_SymSetOptions)(
        __in DWORD   SymOptions
      );
      BOOL (__stdcall *m_StackWalk64)(
        __in DWORD MachineType,
        __in HANDLE hProcess,
        __in HANDLE hThread,
        __inout LPSTACKFRAME64 StackFrame,
        __inout PVOID ContextRecord,
        __in_opt PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
        __in_opt PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
        __in_opt PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
        __in_opt PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
      );
      BOOL (__stdcall *m_SymGetSymFromAddr64)(
        __in HANDLE hProcess,
        __in DWORD64 qwAddr,
        __out_opt PDWORD64 pdwDisplacement,
        __inout PIMAGEHLP_SYMBOL64  Symbol
      );
      BOOL (__stdcall *m_SymGetLineFromAddr64)(
        _In_  HANDLE           hProcess,
        _In_  DWORD64          dwAddr,
        _Out_ PDWORD           pdwDisplacement,
        _Out_ PIMAGEHLP_LINE64 Line
      );

      PFUNCTION_TABLE_ACCESS_ROUTINE64 m_SymFunctionTableAccess64;
      PGET_MODULE_BASE_ROUTINE64       m_SymGetModuleBase64;

      HANDLE m_hProcess;


      PDebugDLL()
        : PDynaLink("DBGHELP.DLL")
        , m_hProcess(GetCurrentProcess())
      {
      }

      ~PDebugDLL()
      {
        if (IsLoaded())
          m_SymCleanup(GetCurrentProcess());
      }

      bool Initialise(ostream & strm)
      {
        if (!IsLoaded() ||
            !GetFunction("SymInitialize", (Function &)m_SymInitialize) ||
            !GetFunction("SymCleanup", (Function &)m_SymCleanup) ||
            !GetFunction("SymGetOptions", (Function &)m_SymGetOptions) ||
            !GetFunction("SymSetOptions", (Function &)m_SymSetOptions) ||
            !GetFunction("StackWalk64", (Function &)m_StackWalk64) ||
            !GetFunction("SymGetSymFromAddr64", (Function &)m_SymGetSymFromAddr64) ||
            !GetFunction("SymFunctionTableAccess64", (Function &)m_SymFunctionTableAccess64) ||
            !GetFunction("SymGetModuleBase64", (Function &)m_SymGetModuleBase64)) {
          strm << "\n    Invalid stack walk DLL: " << GetName() << " not all functions present.";
          return false;
        }

        if (!GetFunction("SymGetLineFromAddr64", (Function &)m_SymGetLineFromAddr64))
          m_SymGetLineFromAddr64 = NULL;

        // Get the directory the .exe file is in
        char filename[_MAX_PATH];
        if (GetModuleFileNameEx(m_hProcess, NULL, filename, sizeof(filename)) == 0) {
          DWORD err = ::GetLastError();
          strm << "\n    GetModuleFileNameEx failed, error=" << err;
          return false;
        }

        ostringstream path;

        // Always look in same place as the .exe file for .pdb file
        char * ptr = strrchr(filename, '\\');
        if (ptr == NULL)
          path << filename;
        else {
          *ptr = '\0';
          path << filename;
          *ptr = '\\';
        }

        /* Add in the environment variables as per default, but do not add in
           current directory (as default does), as if that is C:\, it searches
           the entire disk. Slooooow. */
        const char * env;
        if ((env = getenv("_NT_SYMBOL_PATH")) != NULL)
          path << ';' << env;
        if ((env = getenv("_NT_ALTERNATE_SYMBOL_PATH")) != NULL)
          path << ';' << env;

        // Initialise the symbols with path for PDB files.
        if (!m_SymInitialize(m_hProcess, path.str().c_str(), TRUE)) {
          DWORD err = ::GetLastError();
          strm << "\n    SymInitialize failed, error=" << err;
          return false;
        }

        strm << "\n    Stack walk symbols initialised, path=\"" << path.str() << '"';

        // See if PDB file exists
        ptr = strrchr(filename, '.');
        if (ptr == NULL)
          strcat(filename, ".pdb");
        else
          strcpy(ptr, ".pdb");

        if (_access(filename, 4) != 0)
          strm << "\n    Stack walk could not find symbols file \"" << filename << '"';

        m_SymSetOptions(m_SymGetOptions()|SYMOPT_LOAD_LINES|SYMOPT_FAIL_CRITICAL_ERRORS|SYMOPT_NO_PROMPTS);
        return true;
      }

      void OutputSymbol(ostream & strm, DWORD64 addrPC, DWORD64 * params = NULL)
      {
        strm << "\n    " << hex << setfill('0');

        char buffer[sizeof(IMAGEHLP_SYMBOL64) + 200];
        PIMAGEHLP_SYMBOL64 symbol = (PIMAGEHLP_SYMBOL64)buffer;
        symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
        symbol->MaxNameLength = sizeof(buffer) - sizeof(IMAGEHLP_SYMBOL64);
        DWORD64 displacement = 0;
        DWORD error = 0;
        if (m_SymGetSymFromAddr64(m_hProcess, addrPC, &displacement, symbol))
          strm << symbol->Name;
        else {
          error = ::GetLastError();
          strm << setw(8) << addrPC;
        }

        if (params) {
          strm << '(';
          for (PINDEX i = 0; i < 4; i++) {
            if (i > 0)
              strm << ", ";
            if (params[i] != 0)
              strm << "0x";
            strm << params[i];
          }
          strm << setfill(' ') << ')';
        }

        if (displacement != 0)
          strm << " + 0x" << displacement;

        strm << dec << setfill(' ');

        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(line);
        DWORD dwDisplacement;
        if (m_SymGetLineFromAddr64 != NULL && m_SymGetLineFromAddr64(m_hProcess, addrPC, &dwDisplacement, &line))
          strm << ' ' << line.FileName << '(' << line.LineNumber << ')';

        if (error != 0)
          strm << " - symbol lookup error=" << error;
      }

      void WalkStack(ostream & strm, PThreadIdentifier id, unsigned framesToSkip)
      {
        // The thread information.
        HANDLE hThread;
        int resumeCount = -1;

        CONTEXT threadContext;
        memset(&threadContext, 0, sizeof(threadContext));
        threadContext.ContextFlags = CONTEXT_FULL;

        if (id == PNullThreadIdentifier || id == GetCurrentThreadId()) {
          hThread = GetCurrentThread();
          RtlCaptureContext(&threadContext);
#if P_64BIT
          framesToSkip += 2;
#else
          ++framesToSkip;
#endif
        }
        else {
          hThread = OpenThread(THREAD_QUERY_INFORMATION|THREAD_GET_CONTEXT|THREAD_SUSPEND_RESUME, FALSE, id);
          if (hThread == NULL) {
            DWORD err = ::GetLastError();
            strm << "\n    No thread: id=" << id << " (0x" << std::hex << id << std::dec << "), error=" << err;
            return;
          }
          resumeCount = SuspendThread(hThread);
          if (!GetThreadContext(hThread, &threadContext)) {
            if (resumeCount >= 0)
              ResumeThread(hThread);
            DWORD err = ::GetLastError();
            strm << "\n    No context for thread: id=" << id << " (0x" << std::hex << id << std::dec << "), error=" << err;
            return;
          }
        }

        WalkStack(strm, hThread, threadContext, framesToSkip);

        if (resumeCount >= 0)
          ResumeThread(hThread);
      }

      void WalkStack(ostream & strm, HANDLE hThread, CONTEXT & threadContext, unsigned framesToSkip)
      {
        STACKFRAME64 frame;
        memset(&frame, 0, sizeof(frame));
        frame.AddrPC.Mode    = AddrModeFlat;
        frame.AddrStack.Mode = AddrModeFlat;
        frame.AddrFrame.Mode = AddrModeFlat;
        #ifdef _M_IX86
          // normally, call ImageNtHeader() and use machine info from PE header
          DWORD imageType = IMAGE_FILE_MACHINE_I386;
          frame.AddrPC.Offset = threadContext.Eip;
          frame.AddrFrame.Offset = frame.AddrStack.Offset = threadContext.Esp;
        #elif _M_X64
          DWORD imageType = IMAGE_FILE_MACHINE_AMD64;
          frame.AddrPC.Offset = threadContext.Rip;
          frame.AddrFrame.Offset = frame.AddrStack.Offset = threadContext.Rsp;
        #elif _M_IA64
          DWORD imageType = IMAGE_FILE_MACHINE_IA64;
          frame.AddrPC.Offset = threadContext.StIIP;
          frame.AddrFrame.Offset = threadContext.IntSp;
          frame.AddrBStore.Offset = threadContext.RsBSP;
          frame.AddrBStore.Mode = AddrModeFlat;
          frame.AddrStack.Offset = threadContext.IntSp;
        #else
          #error "Platform not supported!"
        #endif

        unsigned frameCount = 0;
        while (frameCount++ < InternalMaxStackWalk) {
          if (!m_StackWalk64(imageType,
                             m_hProcess,
                             hThread,
                             &frame,
                             &threadContext,
                             NULL,
                             m_SymFunctionTableAccess64,
                             m_SymGetModuleBase64,
                             NULL)) {
            DWORD err = ::GetLastError();
            strm << "\n    StackWalk64 failed: error=" << err;
            break;
          }

          if (frame.AddrPC.Offset == frame.AddrReturn.Offset) {
            strm << "\n    StackWalk64 returned recursive stack! PC=0x" << hex << frame.AddrPC.Offset << dec;
            break;
          }

          if (frameCount <= framesToSkip || frame.AddrPC.Offset == 0)
            continue;

          OutputSymbol(strm, frame.AddrReturn.Offset, frame.Params);

          if (frame.AddrReturn.Offset == 0)
            break;
        }
      }
  };


  void PPlatformWalkStack(ostream & strm, PThreadIdentifier id, PUniqueThreadIdentifier, unsigned framesToSkip, bool, bool)
  {
    PDebugDLL debughelp;
    if (debughelp.Initialise(strm))
      debughelp.WalkStack(strm, id, framesToSkip);
  }

#endif // __MINGW32__


  static const char ActionMessage[] = "<A>bort, <B>reak, "
  #if P_EXCEPTIONS
                                      "<T>hrow exception, "
  #endif
                                      "<I>gnore";

static bool AssertAction(int c, const char * msg)
{
  switch (c) {
    case 'A':
    case 'a':
      cerr << "Aborted" << endl;
    case IDABORT :
      PTRACE(0, "Assert abort, application exiting immediately");
      abort(); // Never returns

    case 'B':
    case 'b':
      cerr << "Break" << endl;
    case IDRETRY :
      PBreakToDebugger();
      return false; // Then ignore it

  #if P_EXCEPTIONS
      case 't' :
      case 'T' :
        PError << "\nThrowing exception.\n";
        throw std::runtime_error(msg);
  #endif

    case 'I':
    case 'i':
      cerr << "Ignored" << endl;

    case IDIGNORE :
    case EOF:
      return false;
  }

  return true;
}


enum {
  CrashDialogCopyId = 1100,
  CrashDialogTextId = 1101,
  CrashDialogCopiedId = 1102
};

struct CrashDialogInfo
{
  const wchar_t * m_text;
  bool            m_finished;
  int             m_result;
  HICON           m_copyIcon;
};


static HICON CreateCopyIcon()
{
  const int size = 32;
  HDC screen = GetDC(NULL);
  HDC dc = CreateCompatibleDC(screen);
  HBITMAP colour = CreateCompatibleBitmap(screen, size, size);
  HBITMAP mask = CreateBitmap(size, size, 1, 1, NULL);
  HGDIOBJ previous = SelectObject(dc, colour);

  RECT background = { 0, 0, size, size };
  HBRUSH face = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
  FillRect(dc, &background, face);
  DeleteObject(face);

  HPEN pen = CreatePen(PS_SOLID, 2, GetSysColor(COLOR_BTNTEXT));
  HGDIOBJ previousPen = SelectObject(dc, pen);
  HGDIOBJ previousBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
  Rectangle(dc, 12, 4, 28, 20);
  SelectObject(dc, GetStockObject(WHITE_BRUSH));
  Rectangle(dc, 4, 12, 20, 28);
  SelectObject(dc, previousBrush);
  SelectObject(dc, previousPen);
  DeleteObject(pen);
  SelectObject(dc, previous);

  HDC maskDc = CreateCompatibleDC(screen);
  HGDIOBJ previousMask = SelectObject(maskDc, mask);
  PatBlt(maskDc, 0, 0, size, size, BLACKNESS);
  SelectObject(maskDc, previousMask);
  DeleteDC(maskDc);
  DeleteDC(dc);
  ReleaseDC(NULL, screen);

  ICONINFO info;
  memset(&info, 0, sizeof(info));
  info.fIcon = TRUE;
  info.hbmColor = colour;
  info.hbmMask = mask;
  HICON icon = CreateIconIndirect(&info);
  DeleteObject(colour);
  DeleteObject(mask);
  return icon;
}


static void CopyCrashText(HWND dialog, CrashDialogInfo * info)
{
  if (info == NULL || info->m_text == NULL || !OpenClipboard(dialog))
    return;

  EmptyClipboard();
  SIZE_T bytes = (wcslen(info->m_text) + 1) * sizeof(wchar_t);
  HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
  if (memory != NULL) {
    void * destination = GlobalLock(memory);
    if (destination != NULL) {
      memcpy(destination, info->m_text, bytes);
      GlobalUnlock(memory);
      if (SetClipboardData(CF_UNICODETEXT, memory) == NULL)
        GlobalFree(memory);
    }
    else
      GlobalFree(memory);
  }
  CloseClipboard();

  HWND edit = GetDlgItem(dialog, CrashDialogTextId);
  if (edit != NULL)
    SendMessageW(edit, EM_SETSEL, 0, -1);
  HWND copied = GetDlgItem(dialog, CrashDialogCopiedId);
  if (copied != NULL)
    SetWindowTextW(copied, L"Copied");
}


static void FinishCrashDialog(HWND dialog, CrashDialogInfo * info, int result)
{
  if (info != NULL) {
    info->m_result = result;
    info->m_finished = true;
  }
  DestroyWindow(dialog);
}


static LRESULT CALLBACK CrashDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
  CrashDialogInfo * info = (CrashDialogInfo *)GetWindowLongPtrW(dialog, GWLP_USERDATA);
  switch (message) {
    case WM_NCCREATE :
      SetWindowLongPtrW(dialog, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCTW *)lParam)->lpCreateParams);
      return DefWindowProcW(dialog, message, wParam, lParam);

    case WM_COMMAND :
      switch (LOWORD(wParam)) {
        case CrashDialogCopyId :
        case CrashDialogCopiedId :
          CopyCrashText(dialog, info);
          return 0;
        case IDABORT :
        case IDRETRY :
        case IDIGNORE :
        case IDOK :
          FinishCrashDialog(dialog, info, LOWORD(wParam));
          return 0;
      }
      break;

    case WM_CLOSE :
      FinishCrashDialog(dialog, info, info != NULL && GetDlgItem(dialog, IDABORT) != NULL ? IDIGNORE : IDOK);
      return 0;

    case WM_DESTROY :
      if (info != NULL && info->m_copyIcon != NULL) {
        DestroyIcon(info->m_copyIcon);
        info->m_copyIcon = NULL;
      }
      return 0;
  }

  return DefWindowProcW(dialog, message, wParam, lParam);
}


static std::wstring CrashDialogText(const wchar_t * text)
{
  std::wstring formatted;
  if (text == NULL)
    return formatted;

  // The edit control treats only CR LF as a line break. Stack walks use LF.
  for (const wchar_t * p = text; *p != L'\0'; ++p) {
    if (*p == L'\n' && (formatted.empty() || formatted.back() != L'\r'))
      formatted += L'\r';
    formatted += *p;
  }
  return formatted;
}


static int ShowCrashDialog(const wchar_t * text, const wchar_t * title, bool choices)
{
  static bool registered = false;
  static const wchar_t className[] = L"PTLibCrashDialog";
  HINSTANCE instance = GetModuleHandleW(NULL);
  std::wstring formatted = CrashDialogText(text);
  if (!registered) {
    WNDCLASSW windowClass;
    memset(&windowClass, 0, sizeof(windowClass));
    windowClass.lpfnWndProc = CrashDialogProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    windowClass.lpszClassName = className;
    registered = RegisterClassW(&windowClass) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
  }

  CrashDialogInfo info;
  info.m_text = formatted.c_str();
  info.m_finished = false;
  info.m_result = choices ? IDIGNORE : IDOK;
  info.m_copyIcon = CreateCopyIcon();

  const int width = 760;
  const int height = 520;
  HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME,
                                className,
                                title != NULL ? title : L"PTLib",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                                (GetSystemMetrics(SM_CXSCREEN) - width) / 2,
                                (GetSystemMetrics(SM_CYSCREEN) - height) / 2,
                                width,
                                height,
                                NULL,
                                NULL,
                                instance,
                                &info);
  if (dialog == NULL) {
    if (info.m_copyIcon != NULL)
      DestroyIcon(info.m_copyIcon);
    return MessageBoxW(NULL, info.m_text, title, choices ? MB_ABORTRETRYIGNORE|MB_ICONHAND|MB_TASKMODAL
                                                         : MB_OK|MB_ICONHAND|MB_TASKMODAL);
  }

  RECT client;
  GetClientRect(dialog, &client);
  const int margin = 12;
  const int buttonHeight = 36;
  const int buttonTop = client.bottom - margin - buttonHeight;
  HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
  HFONT fixed = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                            FIXED_PITCH | FF_MODERN, L"Consolas");

  HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", info.m_text,
                              WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                                ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                              margin, margin,
                              client.right - 2 * margin, buttonTop - 2 * margin,
                              dialog, (HMENU)(INT_PTR)CrashDialogTextId, instance, NULL);
  SendMessageW(edit, WM_SETFONT, (WPARAM)(fixed != NULL ? fixed : font), TRUE);

  HWND copy = CreateWindowW(L"BUTTON", L"",
                            WS_CHILD | WS_VISIBLE | BS_ICON | WS_TABSTOP,
                            margin, buttonTop, buttonHeight, buttonHeight,
                            dialog, (HMENU)(INT_PTR)CrashDialogCopyId, instance, NULL);
  if (info.m_copyIcon != NULL)
    SendMessageW(copy, BM_SETIMAGE, IMAGE_ICON, (LPARAM)info.m_copyIcon);

  HWND copied = CreateWindowW(L"BUTTON", L"Copy to clipboard",
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                              margin + buttonHeight + 8, buttonTop, 150, buttonHeight,
                              dialog, (HMENU)(INT_PTR)CrashDialogCopiedId, instance, NULL);
  SendMessageW(copied, WM_SETFONT, (WPARAM)font, TRUE);

  const int actionIds[3] = { choices ? IDABORT : IDOK, choices ? IDRETRY : 0, choices ? IDIGNORE : 0 };
  const wchar_t * actionText[3] = { choices ? L"Abort" : L"OK", L"Retry", L"Ignore" };
  int actionCount = choices ? 3 : 1;
  const int actionWidth = 88;
  int actionLeft = client.right - margin - actionCount * actionWidth;
  for (int i = 0; i < actionCount; ++i) {
    HWND button = CreateWindowW(L"BUTTON", actionText[i],
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | (i == 0 ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON),
                                actionLeft, buttonTop, actionWidth - 8, buttonHeight,
                                dialog, (HMENU)(INT_PTR)actionIds[i], instance, NULL);
    SendMessageW(button, WM_SETFONT, (WPARAM)font, TRUE);
    actionLeft += actionWidth;
  }

  SendMessageW(edit, EM_SETSEL, 0, -1);
  SetForegroundWindow(dialog);

  MSG message;
  while (!info.m_finished && GetMessageW(&message, NULL, 0, 0) > 0) {
    if (!IsDialogMessageW(dialog, &message)) {
      TranslateMessage(&message);
      DispatchMessageW(&message);
    }
  }

  if (fixed != NULL)
    DeleteObject(fixed);
  return info.m_result;
}


void PPlatformAssertFunc(const PDebugLocation & PTRACE_PARAM(location), const char * msg, char defaultAction)
{
#if PTRACING
  PTrace::Begin(0, location.m_file, location.m_line, NULL, "PAssert") << msg << PTrace::End;
#endif

  if (defaultAction != '\0' && !AssertAction(defaultAction, msg))
      return;

  else if (PProcess::Current().IsGUIProcess()) {
    PWideString boxMsg(msg != NULL ? msg : "");
    PWideString boxTitle(PProcess::Current().GetName());
    AssertAction(ShowCrashDialog(boxMsg, boxTitle, true), msg);
  }
  else {
    do {
      cerr << msg << '\n' << ActionMessage << "? " << flush;
    } while (AssertAction(cin.get(), msg));
  }
}


LONG WINAPI PExceptionHandler(PEXCEPTION_POINTERS info)
{
  ostringstream strm;
  switch (info->ExceptionRecord->ExceptionCode) {
    case 0x406D1388: // Thread name change
      {
        LPCSTR threadName = ((THREADNAME_INFO *)&info->ExceptionRecord->ExceptionInformation[0])->szName;
        PThread * thread = PThread::Current();
        if (thread != NULL && threadName != NULL)
          thread->SetThreadName(threadName);
      }
      return EXCEPTION_CONTINUE_EXECUTION;
    case EXCEPTION_ACCESS_VIOLATION:
      strm << "Access violation: " << (info->ExceptionRecord->ExceptionInformation[0] ? "Write" : "Read")
           << " at " << (void *)(info->ExceptionRecord->ExceptionInformation[1]);
      break;
    case EXCEPTION_IN_PAGE_ERROR:
      strm << "In page error (" << info->ExceptionRecord->ExceptionInformation[2] << ")"
              ": " << (info->ExceptionRecord->ExceptionInformation[0] ? "Write" : "Read")
           << " at " << (void *)(info->ExceptionRecord->ExceptionInformation[1]);
      break;
    case EXCEPTION_STACK_OVERFLOW :
      strm << "Stack overflow";
      break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO :
    case EXCEPTION_INT_DIVIDE_BY_ZERO :
      strm << "Divide by zero";
      break;
    default:
      strm << "Unhandled exception: code=0x" << hex << info->ExceptionRecord->ExceptionCode << dec;
  }

  strm << ", when=" << PTime().AsString(PTime::LoggingFormat PTRACE_PARAM(, PTrace::GetTimeZone()));

#ifndef __MINGW32__
  if (info->ContextRecord) {
    PDebugDLL debughelp;
    if (debughelp.Initialise(strm))
      debughelp.WalkStack(strm, GetCurrentThread(), *info->ContextRecord, 0);
  }
#endif

  string msg = strm.str();

  if (!PProcess::IsInitialised())
    cerr << msg << endl;
  else if (PProcess::Current().IsServiceProcess())
    PSYSTEMLOG(Fatal, "Caught Exception: " << msg);
  else {
#if PTRACING
    PTrace::Begin(0, NULL, 0, NULL, "PException") << msg << PTrace::End;
#endif
    if (PProcess::Current().IsGUIProcess()) {
      PWideString boxMsg(msg.c_str());
      PWideString boxTitle(PProcess::Current().GetName());
      ShowCrashDialog(boxMsg, boxTitle, false);
    }
    else {
      cerr << msg << endl;
      PProcess::Current().WaitOnExitConsoleWindow();
    }
    ExitProcess(1);
  }

  return EXCEPTION_CONTINUE_SEARCH;
}


// End Of File ///////////////////////////////////////////////////////////////
