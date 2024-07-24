/*
 * Test for PTLib integration of Speech synthesis and recognition
 *
 * Portable Tools Library
 *
 * Copyright (C) 2021 by Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): Robert Jongbloed
 */

#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <ptclib/speech.h>
#include <ptclib/pwavfile.h>
#include <ptlib/sound.h>

/*
Sample command lines:
  -ttttodebugstream --play This is a test
  -ttttodebugstream --recognise yes no back bye
*/

const char UsageStr[] = "[ options ... ] --play [ --hint <hint> ] text [ text ... ]\n"
                        "[ options ... ] --voices"
                        "[ options ... ] --recognise [ word ... ]";


class MyProcess : public PProcess
{
  PCLASSINFO(MyProcess, PProcess)

    bool m_interrupted;
    unsigned m_recogisedTextCount;
    MyProcess() : m_interrupted(false), m_recogisedTextCount(0) { }

    virtual void Main() override;
    virtual bool OnInterrupt(bool) { m_interrupted = true; return true; }

    void Play(const PArgList & args);
    void Voices(const PArgList & args);
    void Recognise(const PArgList & args);
    PDECLARE_SpeechRecognitionNotifier(MyProcess, OnRecognition);
};

PCREATE_PROCESS(MyProcess)


#if P_TEXT_TO_SPEECH || P_SPEECH_RECOGNITION

static PSoundChannel * OpenSoundChannel(const PArgList & args, PSoundChannel::Directions dir)
{
  PSoundChannel::Params audioParams;
  audioParams.m_direction = dir;
  audioParams.m_driver = args.GetOptionString("sound-driver");
  audioParams.m_device = args.GetOptionString("sound-device");
  audioParams.m_sampleRate = args.GetOptionAs("sound-rate", 8000);
  audioParams.m_channels = args.GetOptionAs("sound-channels", 1);
  return PSoundChannel::CreateOpenedChannel(audioParams);
}


static PSharedPtr<PTextToSpeech> OpenTextToSpeech(const PArgList & args)
{
  PSharedPtr<PTextToSpeech> tts(PTextToSpeech::Create(args.GetOptionString("engine")));
  if (tts.get() == NULL)
    cerr << "No such text to speech engine!\n";
  else if (!tts->SetVoice(args.GetOptionString("voice")))
    cerr << "No such voice for text to speech engine!\n";
  else if (!tts->SetSampleRate(args.GetOptionAs("sound-rate", 8000)))
    cerr << "Cannot set sample rate on text to speech engine!\n";
  else if (!tts->SetChannels(args.GetOptionAs("sound-channels", 1)))
    cerr << "Cannot set channels on text to speech engine!\n";
  else
    return tts;

  return PSharedPtr<PTextToSpeech>();
}


void MyProcess::Play(const PArgList & args)
{
  if (args.GetCount() == 0) {
    args.Usage(cerr, UsageStr) << "\nNothing to play!\n";
    return;
  }

  PSharedPtr<PTextToSpeech> tts(OpenTextToSpeech(args));
  if (tts.get() == NULL)
    return;

  PSharedPtr<PSoundChannel> player;
  PFilePath fn = args.GetOptionString("file");
  if (fn.empty()) {
    player.reset(OpenSoundChannel(args, PSoundChannel::Player));
    if (player.get() == NULL) {
      cerr << "Cound not create sound player!\n";
      return;
    }

    if (!tts->OpenChannel(player.get())) {
      cerr << "Cound not connect sound player to text to speech engine!\n";
      return;
    }
  }
  else {
    if (!tts->OpenFile(fn)) {
      cerr << "Text to speech engine could not output to file \"" << fn << '"' << endl;
      return;
    }
  }

  PTextToSpeech::TextType hint = PTextToSpeech::TextTypeFromString(args.GetOptionString("hint", PTextToSpeech::TextTypeToString(PTextToSpeech::Default)));
  if (hint == PTextToSpeech::EndTextType) {
    cerr << "Invalid text to speech hint.\n";
    return;
  }

  PStringStream text;
  for (PINDEX i = 0; i < args.GetCount(); ++i)
    text << args[i] << ' ';

  if (tts->Speak(text, hint))
    cout << "Text to speech output successful\n";
  else
    cerr << "Error speking text!\n";
}


void MyProcess::Voices(const PArgList & args)
{
  PSharedPtr<PTextToSpeech> tts(OpenTextToSpeech(args));
  if (tts.get() != NULL)
    cout << setfill('\n') << tts->GetVoiceList() << endl;
}


void MyProcess::Recognise(const PArgList & args)
{
  PSharedPtr<PSpeechRecognition> sr(PSpeechRecognition::Create(args.GetOptionString("engine")));
  if (sr.get() == NULL) {
    cerr << "No such speech recognition engine!\n";
    return;
  }

  if (args.GetCount() > 0) {
    PString vocabulary = "TestVocab";
    if (!sr->CreateVocabulary(vocabulary, args.GetParameters())) {
      cerr << "Could not create vocabulary!\n";
      return;
    }
    if (!sr->ActivateVocabulary(vocabulary)) {
      cerr << "Could not activate vocabulary!\n";
      return;
    }
  }

  if (!sr->Open(PCREATE_NOTIFIER(OnRecognition))) {
    cerr << "Could not open speech recognition!\n";
    return;
  }

  PFilePath fn = args.GetOptionString("file");
  if (fn.empty()) {
    PSoundChannel * recorder = OpenSoundChannel(args, PSoundChannel::Recorder);
    if (recorder == NULL) {
      cerr << "Cound not create sound recorder!\n";
      return;
    }

    while (recorder->IsOpen()) {
      short data[160]; // 20ms at 8kHz
      if (!recorder->Read(data, sizeof(data))) {
        if (recorder->GetErrorCode() == PChannel::NoError)
          break; // End of file
        cerr << "Cound not create sound recorder!\n";
        return;
      }
      if (!sr->Listen(data, recorder->GetLastReadCount()/sizeof(short))) {
        cerr << "Speech recognition listen failed!\n";
        return;
      }
      if (m_interrupted)
        break;
    }
  }
  else {
    if (!sr->Listen(fn)) {
      cerr << "Speech recognition listen failed!\n";
      return;
    }
    PThread::Sleep(10000); // Wait a while for responses to come in
  }
  sr->Close();

  if (m_recogisedTextCount == 0)
    cout << "Nothing recognised.\n";
}


void MyProcess::OnRecognition(PSpeechRecognition &, PSpeechRecognition::Transcript transcript)
{
  cout << transcript << endl;
  ++m_recogisedTextCount;
}


void MyProcess::Main()
{
  cout << "Speech Test Utility" << endl;

  PArgList & args = GetArguments();
  args.Parse("h-help. Display usage\n"
             "e-engine: Speech engine to use\n"
             "-sound-driver: Use sound driver\n"
             "S-sound-device: Use sound device\n"
             "-sound-rate: Use sound sample rate\n"
             "-sound-channels: Use sound channels\n"
             "f-file: Use WAV file\n"
             "p-play. Play text to speech\n"
             "H-hint: Hint for playing\n"
             "v-voice. Voice to use with play\n"
             "V-voices. List voices that can be used to play.\n"
             "r-recognise. Recognise speech\n"
             PTRACE_ARGLIST);
  if (!args.IsParsed() || args.HasOption('h')) {
    args.Usage(cerr, UsageStr);
    return;
  }

  PTRACE_INITIALISE(args);

  if (args.HasOption("play"))
    Play(args);
  else if (args.HasOption("voices"))
    Voices(args);
  else if (args.HasOption("recognise"))
    Recognise(args);
  else
    args.Usage(cerr, UsageStr) << "\nMust have one of --play or --recognise.\n";
}


#else
#pragma message("Cannot compile test program without speech support!")

void MyProcess::Main()
{
}
#endif // P_TEXT_TO_SPEECH || P_SPEECH_RECOGNITION
