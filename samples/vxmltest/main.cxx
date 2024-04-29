/*
 * main.cxx
 *
 * PWLib application source file for vxmltest
 *
 * Main program entry point.
 *
 * Copyright 2002 Equivalence
 *
 */

#include <ptlib.h>
#include <ptlib/sound.h>
#include <ptclib/vxml.h>
#include <ptclib/cli.h>

#include "main.h"

PCREATE_PROCESS(VxmlTest);

#define PTraceModule() "VXMLTest"


#ifdef _WIN32
  #define PREVIEW_WINDOW_DEVICE "MSWIN"
#else
  #define PREVIEW_WINDOW_DEVICE "SDL"
#endif


VxmlTest::VxmlTest()
  : PProcess("Equivalence", "vxmltest", 1, 0, AlphaCode, 1)
{
}

#if P_VXML

void VxmlTest::Main()
{
  PArgList & args = GetArguments();
  if (!args.Parse("-player-driver: Output to sound driver\n"
                  "P-player-device: Output to sound device\n"
                  "-player-rate: Output to sound sample rate\n"
                  "-player-channels: Output to sound channels\n"
                  "-recorder-driver: Output to sound driver\n"
                  "R-recorder-device: Output to sound device\n"
                  "-recorder-rate: Output to sound sample rate\n"
                  "-recorder-channels: Output to sound channels\n"
                  "-http-proxy: Set HTTP proxy\n"
                  "-https-proxy: Set HTTPS proxy\n"
                  "-no-proxy: Set hosts/domains to not use proxy\n"
#if P_SSL
                  "-authority: Set certificate authority file/directory\n"
                  "-certificate: Set client certificate\n"
                  "-private-key: Set client certificate private key\n"
#endif
                  "T-tts: Text to speech method\n"
                  "S-sr: Speech recognition method\n"
                  "c-cache: Text to speech cache directory\n"
                  "-clear-cache. Clear the cache on execution\n"
                  "d-record-dir: Record directory\n"
                  "r-property: Set default property value: name=value\n"
#if P_VXML_VIDEO
                  "V-video. Enabled video support\n"
                  "L-sign-language: Set sign language analyser library (implicit -V)\n"
                  "-input-driver: Video input driver\n"
                  "I-input-device: Video input device\n"
                  "C-input-channel: Video input channel\n"
                  "p-preview. Show preview window for video input\n"
                  "-output-driver: Video output driver\n"
                  "O-output-device: Video output device\n"
                  "-output-channel: Video output channel\n"
#endif
                  PTRACE_ARGLIST)) {
    args.Usage(cerr, "[ options ] <vxml-file> [ [ \"--\" | options ] <vxml-file> ... ]");
    return;
  }

  PTRACE_INITIALISE(args);

  if (args.HasOption("sign-language")) {
    if (!PVXMLSession::SetSignLanguageAnalyser(args.GetOptionString("sign-language"))) {
      cerr << "error: cannot load sign language analyser" << endl;
      return;
    }
    cout << "Using sign language analyser." << endl;
  }

  do {
    PSharedPtr<TestInstance> instance(new TestInstance());
    if (instance->Initialise(m_tests.size(), args))
      m_tests.push_back(instance);
  } while (args.Parse("-player-driver:"
                      "P-player-device:"
                      "-player-rate:"
                      "-player-channels:"
                      "-recorder-driver:"
                      "R-recorder-device:"
                      "-recorder-rate:"
                      "-recorder-channels:"
                      "T-tts:"
#if P_VXML_VIDEO
                      "V-video."
                      "-input-driver:I-input-device:C-input-channel:"
                      "-output-driver:O-output-device:-output-channel:"
#endif
  ));

  PCLIStandard cli("VXML-Test> ");
  cli.SetCommand("input", PCREATE_NOTIFIER(SimulateInput), "Simulate input for VXML instance (1..n)", "<digit> [ <n> ]");
  cli.SetCommand("0", PCREATE_NOTIFIER(SimulateInput), "Simulate input 0 for VXML instance (1..n)", "[ <n> ]");
  cli.SetCommand("1", PCREATE_NOTIFIER(SimulateInput), "Simulate input 1 for VXML instance (1..n)", "[ <n> ]");
  cli.SetCommand("2", PCREATE_NOTIFIER(SimulateInput), "Simulate input 2 for VXML instance (1..n)", "[ <n> ]");
  cli.SetCommand("3", PCREATE_NOTIFIER(SimulateInput), "Simulate input 3 for VXML instance (1..n)", "[ <n> ]");
  cli.SetCommand("4", PCREATE_NOTIFIER(SimulateInput), "Simulate input 4 for VXML instance (1..n)", "[ <n> ]");
  cli.SetCommand("5", PCREATE_NOTIFIER(SimulateInput), "Simulate input 5 for VXML instance (1..n)", "[ <n> ]");
  cli.SetCommand("6", PCREATE_NOTIFIER(SimulateInput), "Simulate input 6 for VXML instance (1..n)", "[ <n> ]");
  cli.SetCommand("7", PCREATE_NOTIFIER(SimulateInput), "Simulate input 7 for VXML instance (1..n)", "[ <n> ]");
  cli.SetCommand("8", PCREATE_NOTIFIER(SimulateInput), "Simulate input 8 for VXML instance (1..n)", "[ <n> ]");
  cli.SetCommand("9", PCREATE_NOTIFIER(SimulateInput), "Simulate input 9 for VXML instance (1..n)", "[ <n> ]");
  cli.SetCommand("set", PCREATE_NOTIFIER(SetVar), "Set variable for VXML instance (1..n)", "<var> <value> [ <n> ]");
  cli.SetCommand("get", PCREATE_NOTIFIER(GetVar), "Get variable for VXML instance (1..n)", "<var> [ <n> ]");
  cli.SetCommand("transfer\nxf", PCREATE_NOTIFIER(TransferStatus), "Set transfer status (1..n)", "<status> [ <n> ]");
  cli.SetCommand("disconnect", PCREATE_NOTIFIER(Disconnect), "Disconnect VXML instance (1..n)", "[ <n> ]");
  cli.Start(false);
  m_tests.clear();
}


void VxmlTest::SimulateInput(PCLI::Arguments & args, P_INT_PTR)
{
  PINDEX instanceArg = 0;
  PString input = args.GetCommandName();
  if (input.length() != 1 || !isdigit(input[0])) {
    if (args.GetCount() < 1) {
      args.WriteUsage();
      return;
    }
    input = args[0];
    instanceArg = 1;
  }

  unsigned num;
  if (args.GetCount() <= instanceArg)
    num = 1;
  else if ((num = args[instanceArg].AsUnsigned()) == 0) {
    args.WriteError("Invalid instance number");
    return;
  }

  if (num > m_tests.size())
    args.WriteError("No such instance");
  else
    m_tests[num - 1]->SendInput(input);
}


void VxmlTest::SetVar(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 2) {
    args.WriteUsage();
    return;
  }

  unsigned num;
  if (args.GetCount() < 3)
    num = 1;
  else if ((num = args[2].AsUnsigned()) == 0) {
    args.WriteError("Invalid instance number");
    return;
  }

  if (num > m_tests.size())
    args.WriteError("No such instance");
  else
    m_tests[num - 1]->SetVar(args[0], args[1]);
}


void VxmlTest::GetVar(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 1) {
    args.WriteUsage();
    return;
  }

  unsigned num;
  if (args.GetCount() < 2)
    num = 1;
  else if ((num = args[1].AsUnsigned()) == 0) {
    args.WriteError("Invalid instance number");
    return;
  }

  if (num > m_tests.size())
    args.WriteError("No such instance");
  else
    args.GetContext() << m_tests[num - 1]->GetVar(args[0]) << endl;
}


void VxmlTest::TransferStatus(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 1) {
    args.WriteUsage();
    return;
  }

  unsigned num;
  if (args.GetCount() == 1)
    num = 1;
  else if ((num = args[1].AsUnsigned()) == 0) {
    args.WriteError("Invalid instance number");
    return;
  }

  if (num > m_tests.size()) {
    args.WriteError("No such instance");
    return;
  }

  PVXMLSession::TransferStatus status;
  PString transferError;

  PCaselessString arg = args[0];
  if (PConstCaselessString("success").starts_with(arg))
    status = PVXMLSession::TransferSuccessful;
  else if (PConstCaselessString("busy").starts_with(arg))
    status = PVXMLSession::TransferBusy;
  else if (PConstCaselessString("congestion").starts_with(arg))
    status = PVXMLSession::TransferCongestion;
  else if (PConstCaselessString("noanswer").starts_with(arg))
    status = PVXMLSession::TransferNoAnswer;
  else if (PConstCaselessString("unknown").starts_with(arg))
    status = PVXMLSession::TransferUnknown;
  else if (PConstCaselessString("failed").starts_with(arg)) {
    status = PVXMLSession::TransferFailed;
    transferError = "custom.123";
  }
  else {
    args.WriteError("Invalid status");
    return;
  }

  m_tests[num - 1]->SetTransferStatus(status, transferError);
}


void VxmlTest::Disconnect(PCLI::Arguments & args, P_INT_PTR)
{
  unsigned num;
  if (args.GetCount() < 1)
    num = 1;
  else if ((num = args[0].AsUnsigned()) == 0) {
    args.WriteError("Invalid instance number");
    return;
  }

  if (num > m_tests.size())
    args.WriteError("No such instance");
  else
    m_tests[num - 1]->Close();
}


TestInstance::TestInstance()
  : m_instance(0)
  , m_player(NULL)
  , m_recorder(NULL)
  , m_grabber(NULL)
  , m_preview(NULL)
  , m_viewer(NULL)
  , m_playerThread(NULL)
  , m_recorderThread(NULL)
  , m_videoSenderThread(NULL)
  , m_videoReceiverThread(NULL)
{
}


TestInstance::~TestInstance()
{
  if (m_player != NULL) {
    m_player->Close();
    PThread::WaitAndDelete(m_playerThread, PMaxTimeInterval);
    delete m_player;
  }

  if (m_recorder != NULL) {
    m_recorder->Close();
    PThread::WaitAndDelete(m_recorderThread, PMaxTimeInterval);
    delete m_recorder;
  }

#if P_VXML_VIDEO
  if (m_viewer != NULL) {
    m_viewer->Close();
    PThread::WaitAndDelete(m_videoSenderThread, PMaxTimeInterval);
    delete m_viewer;
  }

  if (m_grabber != NULL) {
    m_grabber->Close();
    PThread::WaitAndDelete(m_videoReceiverThread, PMaxTimeInterval);
    delete m_grabber;
  }
#endif // P_VXML_VIDEO
}


bool TestInstance::Initialise(unsigned instance, const PArgList & args)
{
  m_instance = instance;

  PArray<PStringToString> redirects;
  redirects.Append(new PStringToString("uri=vxml:origin\nreason=Unknown"));
  redirects.Append(new PStringToString("uri=vxml:redirect\nreason=Unknown"));
  SetConnectionVars(
    "vxml:local",
    "vxml:remote",
    "vxml",
    "Unknown",
    redirects,
    PStringToString("mode=test"),
    true
  );

  PSoundChannel::Params audioParams;
  audioParams.m_direction = PSoundChannel::Player;
  audioParams.m_driver = args.GetOptionString("player-driver");
  audioParams.m_device = args.GetOptionString("player-device");
  if (args.HasOption("player-rate"))
    audioParams.m_sampleRate = args.GetOptionString("player-rate").AsUnsigned();
  if (args.HasOption("player-channels"))
    audioParams.m_channels = args.GetOptionString("player-channels").AsUnsigned();
  m_player = PSoundChannel::CreateOpenedChannel(audioParams);
  if (m_player == NULL) {
    cerr << "Instance " << m_instance << " error: cannot open audio player device \"" << audioParams.m_device << "\"\n"
            "Available:\n" << setfill('\n') << PSoundChannel::GetDeviceNames(audioParams.m_driver, PSoundChannel::Player);
    return false;
  }
  cout << "Instance " << m_instance << " using audio player device \"" << m_player->GetName() << "\"" << endl;

  audioParams.m_direction = PSoundChannel::Recorder;
  audioParams.m_driver = args.GetOptionString("recorder-driver");
  audioParams.m_device = args.GetOptionString("recorder-device");
  if (args.HasOption("recorder-rate"))
    audioParams.m_sampleRate = args.GetOptionString("recorder-rate").AsUnsigned();
  if (args.HasOption("recorder-channels"))
    audioParams.m_channels = args.GetOptionString("recorder-channels").AsUnsigned();
  m_recorder = PSoundChannel::CreateOpenedChannel(audioParams);
  if (m_recorder == NULL) {
    cerr << "Instance " << m_instance << " error: cannot open audio recorder device \"" << audioParams.m_device << "\"\n"
            "Available:\n" << setfill('\n') << PSoundChannel::GetDeviceNames(audioParams.m_driver, PSoundChannel::Recorder);
    return false;
  }
  cout << "Instance " << m_instance << " using audio recording device \"" << m_recorder->GetName() << "\"" << endl;


#if P_VXML_VIDEO
  if (args.HasOption("video") || args.HasOption("sign-language")) {
    PVideoOutputDevice::OpenArgs videoArgs;
    videoArgs.driverName = args.GetOptionString("input-driver");
    videoArgs.deviceName = args.GetOptionString("input-device");
    videoArgs.channelNumber = args.GetOptionString("input-channel", "-1").AsInteger();
    m_grabber = PVideoInputDevice::CreateOpenedDevice(videoArgs);
    if (m_grabber == NULL) {
      cerr << "Instance " << m_instance << " error: cannot open video device \"" << videoArgs.deviceName << "\"" << endl;
      return false;
    }
    cout << "Instance " << m_instance << " using input video device \"" << m_grabber->GetDeviceName() << "\"" << endl;

    if (args.HasOption("preview"))
      m_preview = PVideoOutputDevice::CreateOpenedDevice(PREVIEW_WINDOW_DEVICE "TITLE=Preview");

    videoArgs.driverName = args.GetOptionString("output-driver");
    videoArgs.deviceName = args.GetOptionString("output-device");
    videoArgs.channelNumber = args.GetOptionString("output-channel", "-1").AsInteger();
    m_viewer = PVideoOutputDevice::CreateOpenedDevice(videoArgs);
    if (m_viewer == NULL) {
      cerr << "Instance " << m_instance << " error: cannot open video device \"" << videoArgs.deviceName << "\"" << endl;
      return false;
    }
    cout << "Instance " << m_instance << " using output video device \"" << m_viewer->GetDeviceName() << "\"" << endl;
  }
#endif

  SetProxies(PHTTP::Proxies(args.GetOptionString("http-proxy"),
                            args.GetOptionString("https-proxy", args.GetOptionString("http-proxy")),
                            args.GetOptionString("no-proxy")));
#if P_SSL
  SetSSLCredentials(args.GetOptionString("authority"),
                    args.GetOptionString("certificate"),
                    args.GetOptionString("private-key"));
#endif

  if (SetTextToSpeech(args.GetOptionString("tts")) == NULL) {
    PFactory<PTextToSpeech>::KeyList_T engines = PFactory<PTextToSpeech>::GetKeyList();
    if (!engines.empty()) {
      cerr << "Instance " << m_instance << " error: cannot select text to speech engine, use one of:\n";
      for (PFactory<PTextToSpeech>::KeyList_T::iterator it = engines.begin(); it != engines.end(); ++it)
        cerr << "  " << *it << '\n';
      return false;
    }
    cerr << "Instance " << m_instance << " warning: no text to speech engine available" << endl;
  }
  cout << "Instance " << m_instance << " using text to speech:\n" << GetTextToSpeech()->GetVoiceList()[0] << endl;

  if (!SetSpeechRecognition(args.GetOptionString("sr"))) {
    PFactory<PTextToSpeech>::KeyList_T engines = PFactory<PSpeechRecognition>::GetKeyList();
    if (!engines.empty()) {
      cerr << "Instance " << m_instance << " error: cannot select speech recognition engine, use one of:\n";
      for (PFactory<PTextToSpeech>::KeyList_T::iterator it = engines.begin(); it != engines.end(); ++it)
        cerr << "  " << *it << '\n';
      return false;
    }
    cerr << "Instance " << m_instance << " warning: no speech recognition engine available" << endl;
  }
  cout << "Instance " << m_instance << " using speech recognition \"" << GetSpeechRecognition() << '"' << endl;

  if (!Load(args[0])) {
    cerr << "Instance " << m_instance << " error: cannot loading VXML document \"" << args[0] << "\" - " << GetXMLError() << endl;
    return false;
  }

  PStringToString properties(args.GetOptionString("property"));
  for (PStringToString::iterator it = properties.begin(); it != properties.end(); ++it)
    SetProperty(it->first, it->second);

  if (args.HasOption("cache"))
    GetCache().SetDirectory(args.GetOptionString("cache"));
  if (args.HasOption("clear-cache"))
    PDirectory::RemoveTree(GetCache().GetDirectory());

  if (args.HasOption("record-dir"))
    SetRecordDirectory(args.GetOptionString("record-dir"));

  if (!Open(VXML_PCM16, audioParams.m_sampleRate, audioParams.m_channels)) {
    cerr << "Instance " << m_instance << " error: cannot open VXML device in PCM mode" << endl;
    return false;
  }

#if P_VXML_VIDEO
  if (m_viewer != NULL)
    m_videoSenderThread = new PThreadObj<TestInstance>(*this, &TestInstance::CopyVideoSender, false, "CopyVideoSender");
  if (m_grabber != NULL)
    m_videoReceiverThread = new PThreadObj<TestInstance>(*this, &TestInstance::CopyVideoReceiver, false, "CopyVideoReceiver");
#endif

  m_playerThread = new PThreadObj<TestInstance>(*this, &TestInstance::PlayAudio, false, "PlayAudio");
  m_recorderThread = new PThreadObj<TestInstance>(*this, &TestInstance::RecordAudio, false, "RecordAudio");

  cout << "Started test instance " << m_instance << " on " << args[0].ToLiteral() << endl;
  return true;
}


void TestInstance::OnEndDialog()
{
  cout << "VXML Dialog ended." << endl;
}


void TestInstance::OnEndSession()
{
  cout << "VXML Session ended." << endl;
}


PVXMLSession::TransferStatus TestInstance::OnTransfer(const PString & destination, TransferType type)
{
  cout << "VXML Session " << type << " to " << destination << endl;
  return TransferInProgress;
}


void TestInstance::PlayAudio()
{
  PBYTEArray audioPCM(8*320);
  m_player->SetBuffers(8, 320);

  for (;;) {
    if (!Read(audioPCM.GetPointer(), audioPCM.GetSize())) {
      if (GetErrorCode(PChannel::LastReadError) != PChannel::NotOpen) {
        PTRACE(2, "Instance " << m_instance << " audio player read error " << GetErrorText());
      }
      break;
    }

    if (!m_player->Write(audioPCM, GetLastReadCount())) {
      PTRACE_IF(2, m_player->IsOpen(), "Instance " << m_instance << " audio player write error " << m_player->GetErrorText());
      break;
    }
  }
}


void TestInstance::RecordAudio()
{
  PBYTEArray audioPCM(8*320);
  m_recorder->SetBuffers(8, 320);

  for (;;) {
    if (!m_recorder->Read(audioPCM.GetPointer(), audioPCM.GetSize())) {
      if (GetErrorCode(PChannel::LastReadError) != PChannel::NotOpen) {
        PTRACE(2, "Instance " << m_instance << " audio player recorder error " << GetErrorText());
      }
      break;
    }

    if (!Write(audioPCM, m_recorder->GetLastReadCount())) {
      PTRACE_IF(2, m_player->IsOpen(), "Instance " << m_instance << " audio player write error " << m_player->GetErrorText());
      break;
    }
  }
}


#if P_VXML_VIDEO
void TestInstance::CopyVideoSender()
{
  PBYTEArray frame;
  PVideoOutputDevice::FrameData frameData;
  PVideoInputDevice & sender = GetVideoSender();
  sender.SetColourFormatConverter(PVideoFrameInfo::YUV420P());
  m_viewer->SetColourFormatConverter(PVideoFrameInfo::YUV420P());

  while (m_viewer->IsOpen()) {
    if (!sender.GetFrame(frame, frameData.width, frameData.height)) {
      PTRACE(2, "Instance " << m_instance << " vxml video preview failed");
      break;
    }

    if (frame.IsEmpty())
      continue;

    m_viewer->SetFrameSize(frameData.width, frameData.height);
    frameData.pixels = frame;

    if (!m_viewer->SetFrameData(frameData)) {
      PTRACE_IF(2, m_viewer->IsOpen(), "Instance " << m_instance << " output video failed");
      break;
    }
  }
}

void TestInstance::CopyVideoReceiver()
{
  PTime start;
  PBYTEArray frame;
  PVideoOutputDevice::FrameData frameData;
  PVideoOutputDevice & receiver = GetVideoReceiver();

  receiver.SetColourFormatConverter(m_grabber->GetColourFormat());
  if (m_preview)
    m_preview->SetColourFormatConverter(m_grabber->GetColourFormat());

  while (m_grabber != NULL) {
    if (!m_grabber->GetFrame(frame, frameData.width, frameData.height)) {
      PTRACE(2, "Instance " << m_instance << " grab video failed");
      break;
    }

    if (frame.IsEmpty())
      continue;

    receiver.SetFrameSize(frameData.width, frameData.height);
    frameData.pixels = frame;
    frameData.sampleTime = start.GetElapsed();

    if (m_preview)
      m_preview->SetFrameData(frameData);

    if (!receiver.SetFrameData(frameData)) {
      PTRACE(2, "Instance " << m_instance << " vxml video feed failed");
      break;
    }
  }
}
#endif

#else
#pragma message("Cannot compile test application without VXML support!")

void VxmlTest::Main()
{
}
#endif


// End of File ///////////////////////////////////////////////////////////////
