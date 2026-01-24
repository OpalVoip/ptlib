/*
 * speech_aws.cxx
 *
 * AWS SDK interface wrapper for speech
 *
 * Portable Windows Library
 *
 * Copyright (c) 2021 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): Robert Jongbloed.
 */

#include <ptlib.h>

#if P_AWS_SDK

#include <ptclib/speech.h>
#include <ptclib/pwavfile.h>
#include <ptclib/random.h>
#include <ptclib/url.h>
#include <ptclib/cypher.h>
#include <ptclib/http.h>
#include <ptclib/pjson.h>
#include <ptclib/aws_sdk.h>
#include <ptclib/pxml.h>
#include <ptclib/qchannel.h>

P_PUSH_MSVC_WARNINGS(26495)
#define USE_IMPORT_EXPORT
#include <aws/polly/PollyClient.h>
#include <aws/polly/model/DescribeVoicesRequest.h>
#include <aws/polly/model/SynthesizeSpeechRequest.h>
#include <aws/transcribe/TranscribeServiceClient.h>
#include <aws/transcribe/model/CreateVocabularyRequest.h>
#include <aws/transcribe/model/DeleteVocabularyRequest.h>
P_POP_MSVC_WARNINGS()

#define PTraceModule() "AWS-Speech"

#ifdef P_AWS_SDK_LIBDIR
  #pragma comment(lib, P_AWS_SDK_LIBDIR(polly))
  #pragma comment(lib, P_AWS_SDK_LIBDIR(text-to-speech))
  #pragma comment(lib, P_AWS_SDK_LIBDIR(transcribe))
#endif


 ////////////////////////////////////////////////////////////
// Text to speech using AWS

static PConstCaselessString VoiceEngine("engine");
static PConstCaselessString VoiceEngines("engines");
static PConstCaselessString DefaultVoiceLanguage("TTS-Language");
static PConstCaselessString DefaultVoiceGender("TTS-Gender");
static PConstCaselessString DefaultVoiceEngine("TTS-Engine");

static const char * HintToAWS[PTextToSpeech::TextType::NumTextType] = {
  "", // Default,
  NULL, // Literal,
  "digits", // Digits,
  "cardinal", // Number,
  "cardinal", // Currency,
  "time", // Time,
  "date", // Date,
  NULL, // DateAndTime,
  "telephone", // Phone,
  NULL, // IPAddress,
  NULL, // Duration,
  "characters", // Spell,
  NULL, // Boolean
  NULL, // FullSSML
  "" // PartialSSML
};

class PTextToSpeech_AWS : public PTextToSpeech, PAwsClient<Aws::Polly::PollyClient>
{
  PCLASSINFO(PTextToSpeech_AWS, PTextToSpeech);

  Aws::Polly::Model::LanguageCode m_defaultLanguage = Aws::Polly::Model::LanguageCode::NOT_SET;
  Aws::Polly::Model::Gender       m_defaultGender   = Aws::Polly::Model::Gender::NOT_SET;
  Aws::Polly::Model::Engine       m_defaultEngine   = Aws::Polly::Model::Engine::standard;

  typedef PDictionary<PCaselessString, PStringOptions> VoiceDict;
  VoiceDict       m_voices;
  bool            m_sampleRate8k{ true };
  PWAVFile        m_file;
  PChannel      * m_openedChannel{ nullptr };
  PSharedPtr<PChannel> m_streamChannel;
  enum StreamState { e_StreamIdle, e_StreamStarting, e_StreamError, e_StreamStarted };
  std::atomic<StreamState> m_streamState{ e_StreamIdle };
  PTRACE_THROTTLE(m_asyncAbort, 3, 500);

  void SetRequestInfo(Aws::Polly::Model::SynthesizeSpeechRequest & request, const PString & text, TextType hint)
  {
    if (m_voice.empty())
      InitVoiceList();

    request.SetOutputFormat(Aws::Polly::Model::OutputFormat::pcm);
    request.SetSampleRate(m_sampleRate8k ? "8000" : "16000");
    request.SetVoiceId(Aws::Polly::Model::VoiceIdMapper::GetVoiceIdForName(m_voice.Get(VoiceName).c_str()));
    request.SetLanguageCode(GetLanguageCodeForName(m_voice.Get(VoiceLanguage)));
    request.SetEngine(GetEngineForName(m_voice.Get(VoiceEngine)));

    if (hint == PartialSSML)
      request.SetText(text.c_str());
    else {
      PStringStream strm;
      strm << "<speak>";
      if (HintToAWS[hint] == NULL || HintToAWS[hint][0] =='\0')
        strm << PXML::EscapeSpecialChars(text);
      else {
        strm << "<say-as interpret-as=\"" << HintToAWS[hint] << '"';
        if (hint == Date) {
          static const char * DateOrder[] = { "mdy", "dmy", "ymd" };
          strm << " format=\"" << DateOrder[PTime::GetDateOrder()] << '"';
        }
        strm << '>' << PXML::EscapeSpecialChars(text) << "</say-as>";
      }
      strm << "</speak>";
      request.SetText(strm.c_str());
    }
    request.SetTextType(Aws::Polly::Model::TextType::ssml);
  }

  // We have all our own lookup functions as the default AWS code (IMHO stupidly)
  // adds whatever new string you provide and returns an illegal enum.
  Aws::Polly::Model::LanguageCode GetLanguageCodeForName(const PString & name) const
  {
    auto code = Aws::Polly::Model::LanguageCodeMapper::GetLanguageCodeForName(name);
    if (code > Aws::Polly::Model::LanguageCode::NOT_SET && static_cast<int>(code) < 100)
      return code;
    PTRACE(3, "Invalid language: \"" << name << '"');
    return Aws::Polly::Model::LanguageCode::NOT_SET;
  }

  Aws::Polly::Model::Gender GetGenderForName(const PString & name) const
  {
    auto code = Aws::Polly::Model::GenderMapper::GetGenderForName(name);
    if (code > Aws::Polly::Model::Gender::NOT_SET && static_cast<int>(code) < 3)
      return code;
    PTRACE(3, "Invalid gender: \"" << name << '"');
    return Aws::Polly::Model::Gender::NOT_SET;
  }

  Aws::Polly::Model::Engine GetEngineForName(const PString & name) const
  {
    auto code = Aws::Polly::Model::EngineMapper::GetEngineForName(name);
    if (code > Aws::Polly::Model::Engine::NOT_SET && static_cast<int>(code) < 10)
      return code;
    PTRACE(3, "Invalid engine: \"" << name << '"');
    return Aws::Polly::Model::Engine::NOT_SET;
  }

  bool InitVoiceList()
  {
    if (!m_voices.empty())
      return true;

    //Enumerate the available voices
    auto outcome = GetClient()->DescribeVoices(Aws::Polly::Model::DescribeVoicesRequest());
    if (!outcome.IsSuccess()) {
      PTRACE(2, "Could not describe voices: " << outcome.GetError().GetMessage());
      return false;
    }

    for (const auto & voice : outcome.GetResult().GetVoices()) {
      PCaselessString voiceName = Aws::Polly::Model::VoiceIdMapper::GetNameForVoiceId(voice.GetId());
      PStringOptions * opts = new PStringOptions;
      opts->Set(VoiceName, voiceName);
      opts->Set(VoiceLanguage, Aws::Polly::Model::LanguageCodeMapper::GetNameForLanguageCode(voice.GetLanguageCode()));
      opts->Set(VoiceGender, Aws::Polly::Model::GenderMapper::GetNameForGender(voice.GetGender()));

      PStringStream strm;
      for (auto engine : voice.GetSupportedEngines()) {
        if (engine == Aws::Polly::Model::Engine::NOT_SET)
          continue;
        if (!strm.empty())
          strm << '+';
        strm << Aws::Polly::Model::EngineMapper::GetNameForEngine(engine);
      }
      opts->Set(VoiceEngines, strm);

      m_voices.SetAt(voiceName, opts);
    }

    if (m_voice.empty()) {
      PStringStream defaultVoice;
      defaultVoice = "language=";
      if (m_defaultLanguage != Aws::Polly::Model::LanguageCode::NOT_SET)
        defaultVoice << Aws::Polly::Model::LanguageCodeMapper::GetNameForLanguageCode(m_defaultLanguage);
      else
        defaultVoice << "en-US";
      defaultVoice << ";gender=";
      if (m_defaultGender != Aws::Polly::Model::Gender::NOT_SET)
        defaultVoice << Aws::Polly::Model::GenderMapper::GetNameForGender(m_defaultGender);
      else
        defaultVoice << "Female";
      if (!SetVoice(defaultVoice))
        return false;
    }

    return true;
  }

public:
  ~PTextToSpeech_AWS()
  {
    Close();
  }

  virtual bool SetOptions(const PStringOptions & options) override
  {
    PString dflt = options.Get(DefaultVoiceLanguage);
    if (!dflt.empty()) {
      auto language = GetLanguageCodeForName(dflt);
      if (language != Aws::Polly::Model::LanguageCode::NOT_SET) {
        m_defaultLanguage = language;
        PTRACE(3, "Using default language: " << dflt);
      }
    }

    dflt = options.Get(DefaultVoiceGender);
    if (!dflt.empty()) {
      auto gender = GetGenderForName(dflt);
      if (gender != Aws::Polly::Model::Gender::NOT_SET) {
        m_defaultGender = gender;
        PTRACE(3, "Using default gender: " << dflt);
      }
    }

    dflt = options.Get(DefaultVoiceEngine);
    if (!dflt.empty()) {
      auto engine = GetEngineForName(dflt);
      if (engine != Aws::Polly::Model::Engine::NOT_SET) {
        m_defaultEngine = engine;
        PTRACE(3, "Using default engine: " << dflt);
      }
    }

    return PAwsClient<Aws::Polly::PollyClient>::SetOptions(options) && PTextToSpeech::SetOptions(options);
  }


  virtual PStringArray GetVoiceList() override
  {
    PStringArray voices;
    if (m_voices.empty() && !InitVoiceList())
      return voices;

    for (const auto & voice : m_voices) {
      PStringStream strm;
      PURL::OutputVars(strm, voice.second);
      voices.AppendString(strm.Mid(1));
    }

    PTRACE(3, "Voices:\n" << setfill('\n') << voices);
    return voices;
  }


  bool ValidateEngine(PStringOptions & voice) const
  {
    PCaselessString engine = voice.Get(VoiceEngine);
    if (engine.empty()) {
      engine = Aws::Polly::Model::EngineMapper::GetNameForEngine(
        m_defaultEngine != Aws::Polly::Model::Engine::NOT_SET ? m_defaultEngine : Aws::Polly::Model::Engine::standard
      );
      voice.Set(VoiceEngine, engine);
    }
    else {
      auto code = GetEngineForName(engine);
      if (code <= Aws::Polly::Model::Engine::NOT_SET)
        return false;
    }

    auto engines = voice.Get(VoiceEngines).ToLower().Tokenise('+');
    if (engines.GetStringsIndex(engine) != P_MAX_INDEX)
      return true;

    static Aws::Polly::Model::Engine const FallbackEngines[] = {
      Aws::Polly::Model::Engine::generative,
      Aws::Polly::Model::Engine::neural,
      Aws::Polly::Model::Engine::long_form,
      Aws::Polly::Model::Engine::standard
    };
    for (size_t fallback = 0; fallback < PARRAYSIZE(FallbackEngines); ++fallback) {
      auto fallbackEngine = Aws::Polly::Model::EngineMapper::GetNameForEngine(FallbackEngines[fallback]);
      if (engines.GetStringsIndex(fallbackEngine) != P_MAX_INDEX) {
        PTRACE(4, "Falling back to engine \"" << fallbackEngine << "\" from \"" << engine << '"');
        voice.Set(VoiceEngine, fallbackEngine);
        return true;
      }
    }

    PTRACE(2, "No supported engine found in \"" << voice.Get(VoiceEngines) << '"');
    return false;
  }

  virtual bool InternalSetVoice(PStringOptions & voice) override
  {
    if (voice == m_voice)
      return ValidateEngine(voice);

    if (!InitVoiceList())
      return false;

    PCaselessString language = voice.Get(VoiceLanguage);
    if (!language.empty() && GetLanguageCodeForName(language) == Aws::Polly::Model::LanguageCode::NOT_SET)
      return false;

    PCaselessString gender = voice.Get(VoiceGender);
    if (!gender.empty() && GetGenderForName(gender) == Aws::Polly::Model::Gender::NOT_SET)
      return false;

    PCaselessString voiceName = voice.Get(VoiceName);
    if (!voiceName.empty()) {
      PStringOptions * found = m_voices.GetAt(voiceName);
      if (!found) {
        PTRACE(2, "Unsupported voice \"" << voiceName << '"');
        return false;
      }
      if (!language.empty() && language != found->Get(VoiceLanguage)) {
        PTRACE(2, "Voice \"" << voiceName << "\" has incorrect language: "
               << language << " != " << found->Get(VoiceLanguage));
        return false;
      }
      if (!gender.empty() && gender != found->Get(VoiceGender)) {
        PTRACE(2, "Voice \"" << voiceName << "\" has incorrect gender: "
               << gender << " != " << found->Get(VoiceGender));
        return false;
      }

      voice.Merge(*found, PStringToString::e_MergeIgnore);
      return ValidateEngine(voice);
    }

    if (language.empty() && m_defaultLanguage != Aws::Polly::Model::LanguageCode::NOT_SET)
      language = Aws::Polly::Model::LanguageCodeMapper::GetNameForLanguageCode(m_defaultLanguage);

    if (gender.empty() && m_defaultGender != Aws::Polly::Model::Gender::NOT_SET)
      gender = Aws::Polly::Model::GenderMapper::GetNameForGender(m_defaultGender);

    PCaselessString engine = voice.Get(VoiceEngine);

    for (const auto & it : m_voices) {
      if ((language.empty() || language == it.second.Get(VoiceLanguage)) &&
          (gender.empty() || gender == it.second.Get(VoiceGender)) &&
          (engine.empty() || engine == it.second.Get(VoiceEngine) || it.second.Get(VoiceEngines).Find(engine) != P_MAX_INDEX)) {
        voice.Merge(it.second, PStringToString::e_MergeOverwrite);
        return ValidateEngine(voice);
      }
    }

    PTRACE(2, "Could not find voice using:\n" << voice);
    return false;
  }


  virtual bool SetSampleRate(unsigned rate) override
  {
    if (rate != 8000 && rate != 16000)
      return false;

    m_sampleRate8k = rate == 8000;
    return true;
  }


  virtual unsigned GetSampleRate() const override
  {
    return m_sampleRate8k ? 8000 : 16000;
  }


  virtual bool SetChannels(unsigned channels) override
  {
    return channels == 1;
  }


  virtual unsigned GetChannels() const override
  {
    return 1;
  }


  bool SetVolume(unsigned) override
  {
    return false;
  }


  unsigned GetVolume() const override
  {
    return 100;
  }


  bool Supports(TextType type) const override
  {
    return HintToAWS[type] != NULL;
  }


  bool OpenFile(const PFilePath & fn) override
  {
    Close();

    if (m_file.Open(fn, PFile::WriteOnly))
      return true;

    PTRACE(2, "Could not open \"" << fn << "\" - " << m_file.GetErrorText());
    return false;
  }


  bool OpenChannel(PChannel * channel) override
  {
    Close();
    m_openedChannel = channel;
    return true;
  }


  bool IsOpen() const override
  {
    return m_streamChannel || m_openedChannel || m_file.IsOpen();
  }


  bool Close() override
  {
    m_openedChannel= nullptr;
    m_file.Close();

    auto channel = m_streamChannel;
    if (!channel)
      return true;

    channel->Close();

    PSimpleTimer failsafe(0, 10);
    while (failsafe.IsRunning()) {
      if (!m_streamChannel)
        return true;
      PThread::Sleep(100);
    }

    PTRACE(2, "Timeout waiting for SynthesizeSpeechAsync to stop.");
    return true;
  }


  bool Speak(const PString & text, TextType hint) override
  {
    if (!IsOpen())
      return false;

    Aws::Polly::Model::SynthesizeSpeechRequest request;
    SetRequestInfo(request, text, hint);

    Aws::Polly::Model::SynthesizeSpeechOutcome outcome = GetClient()->SynthesizeSpeech(request);
    if (!outcome.IsSuccess()) {
      PTRACE(2, "Could not synthesize speech: " << outcome.GetError().GetMessage());
      return false;
    }

    PChannel & channel = m_openedChannel ? *m_openedChannel : m_file;
    auto & strm = outcome.GetResult().GetAudioStream();
    for (;;) {
      char buf[1000];
      strm.read(buf, sizeof(buf));
      auto count = strm.gcount();
      if (count == 0)
        break;

      if (channel.Write(buf, count))
        continue;

      PTRACE(2, "Write failure to \"" << channel.GetName() << "\" - " << channel.GetErrorText());
      return false;
    }

    return true;
  }

  PSharedPtr<PChannel> SpeakToChannel(const PString & text, TextType hint) override
  {
    if (IsOpen())
      return nullptr;

    m_streamChannel.reset(new PQueueChannel(m_sampleRate8k ? 16000 : 32000)); // One second of PCM
    m_streamState = e_StreamStarting;

    Aws::Polly::Model::SynthesizeSpeechRequest request;
    SetRequestInfo(request, text, hint);

    request.SetDataReceivedEventHandler(
      [this](const Aws::Http::HttpRequest*, Aws::Http::HttpResponse* response, long long)
      {
        auto buffer = (stringstream() << response->GetResponseBody().rdbuf()).str();

        StreamState expect = e_StreamStarting;
        m_streamState.compare_exchange_strong(expect,
            buffer.compare(0, 10, "{\"message\"") == 0 ? e_StreamError : e_StreamStarted);

        if (buffer.empty() || m_streamChannel->Write(buffer.data(), buffer.length()))
          PTRACE(5, "SynthesizeSpeechAsync: transferred " << buffer.length() << " bytes");
        else if (m_streamChannel->GetErrorCode() != PChannel::NotOpen)
          PTRACE(2, "SynthesizeSpeechAsync: write error - " << m_streamChannel->GetErrorText());
        else
          PTRACE(m_asyncAbort, "SynthesizeSpeechAsync: aborted");
      });

    GetClient()->SynthesizeSpeechAsync(
      request,
      [this](const Aws::Polly::PollyClient *,
         const Aws::Polly::Model::SynthesizeSpeechRequest &,
         Aws::Polly::Model::SynthesizeSpeechOutcome outcome,
         const std::shared_ptr<const Aws::Client::AsyncCallerContext> &)
      {
        PQueueChannel * channel = dynamic_cast<PQueueChannel *>(m_streamChannel.get());
        auto prevLength = channel->GetLength();

        if (!outcome.IsSuccess())
          PTRACE(2, "SynthesizeSpeechAsync: error - " << outcome.GetError() << '\n' << channel->ReadString(prevLength));
        else {
          PTRACE_IF(5, prevLength > 0, "SynthesizeSpeechAsync: draining queue of " << prevLength << " bytes");

          unsigned noDrainCount = 0;
          while (prevLength > 0) {
            PThread::Sleep(100);

            auto nextLength = channel->GetLength();
            if (nextLength != prevLength)
              noDrainCount = 0;
            else if (noDrainCount++ > 4) {
              PTRACE(5, "SynthesizeSpeechAsync: nothing draining queue.");
              break;
            }

            prevLength = nextLength;
          }
          PTRACE(4, "SynthesizeSpeechAsync: successful.");
        }

        m_streamChannel->Close();
        m_streamChannel.reset();
        m_streamState = e_StreamIdle;
      });

    PTRACE(4, "SynthesizeSpeechAsync: starting.");
    PSimpleTimer timer(0, 2);
    while (timer.IsRunning()) {
      switch (m_streamState) {
        case e_StreamStarted :
          PTRACE(4, "SynthesizeSpeechAsync: started.");
          //Do next case
        case e_StreamIdle:
          return m_streamChannel;
        case e_StreamStarting:
        case e_StreamError:
          PThread::Sleep(10);
      }
    }

    PTRACE(2, "SynthesizeSpeechAsync: failed to start.");
    Close();
    m_streamState = e_StreamIdle;
    return m_streamChannel;
  }
};


PFACTORY_CREATE(PFactory<PTextToSpeech>, PTextToSpeech_AWS, P_TEXT_TO_SPEECH_AWS, false);


////////////////////////////////////////////////////////////
// Speech recognition using Microsoft's Speech API (SAPI)

class PSpeechRecognition_AWS : public PSpeechRecognition, PAwsClient<Aws::TranscribeService::TranscribeServiceClient>
{
  PCLASSINFO(PSpeechRecognition_AWS, PSpeechRecognition);

  unsigned m_sampleRate = 8000;
  unsigned m_bufferSize = 800; // Default 100ms
  Aws::TranscribeService::Model::LanguageCode m_language = Aws::TranscribeService::Model::LanguageCode::en_US;
  PStringSet m_createdVocabularies;
  PStringSet m_activeVocabularies;
  PStringSet m_activeLanguages;
  HeaderMap  m_additionalHeaders;

  PWebSocket m_webSocket;
  PThread  * m_eventThread = nullptr;
  bool m_openingWebSocket = false;
  std::vector<int16_t> m_buffer;

  #pragma pack(1)
  struct Prelude
  {
    PUInt32b m_totalLen;
    PUInt32b m_headerLen;
    PUInt32b m_preludeCRC;
  };
  #pragma pack()

  bool WriteEvent(const PBYTEArray & contents, const PStringOptions & headers)
  {
    size_t headerLen = 0;
    for (PStringOptions::const_iterator it = headers.begin(); it != headers.end(); ++it)
      headerLen += 1 + it->first.length() + 1 + 2 + it->second.length();

    PBYTEArray message(sizeof(Prelude) + headerLen + contents.size() + sizeof(PUInt32b));
    BYTE * ptr = message.GetPointer();

    Prelude & prelude = *reinterpret_cast<Prelude *>(ptr);
    ptr += sizeof(Prelude);

    prelude.m_headerLen = headerLen;
    prelude.m_totalLen = message.size();
    prelude.m_preludeCRC = PCRC32::Calculate(&prelude, sizeof(Prelude) - sizeof(prelude.m_preludeCRC));

    for (PStringOptions::const_iterator it = headers.begin(); it != headers.end(); ++it) {
      PString key = it->first;
      PString value = it->second;
      *ptr++ = key.length();
      memcpy(ptr, key.c_str(), key.length());
      ptr += key.length();
      *ptr++ = 7;  // String
      *(PUInt16b *)ptr = value.length();
      ptr += 2;
      memcpy(ptr, value.c_str(), value.length());
      ptr += value.length();
    }

    memcpy(ptr, contents, contents.size());
    ptr += contents.size();

    *(PUInt32b *)ptr = PCRC32::Calculate(message, message.size() - sizeof(PUInt32b));

    return m_webSocket.Write(message, message.size());
  }

  bool WriteAudioEvent(const int16_t * samples, size_t sampleCount)
  {
    PStringOptions headers;
    headers.Set(":message-type", "event");
    headers.Set(":event-type", "AudioEvent");
    headers.Set(":content-type", "application/octet-stream");

    return WriteEvent(PBYTEArray((const BYTE *)samples, sampleCount*sizeof(uint16_t), false), headers);
  }

  bool ReadEvent(PBYTEArray & contents, PStringOptions & headers)
  {
    PBYTEArray message;
    if (!m_webSocket.ReadMessage(message)) {
      PTRACE_IF(2, m_webSocket.GetErrorCode() != PChannel::NoError,
                "Error reading WebSocket:" << m_webSocket.GetErrorText());
      m_webSocket.Close();
      return false;
    }

    if (message.size() < sizeof(Prelude)) {
      PTRACE(2, "Received message ridiculously small");
      return false;
    }

    Prelude prelude = message.GetAs<Prelude>();
    if (message.size() < prelude.m_totalLen) {
      PTRACE(2, "Received message too small");
      return false;
    }

    uint32_t calculatedCRC = PCRC32::Calculate(message, sizeof(Prelude) - sizeof(prelude.m_preludeCRC));
    uint32_t providedCRC = prelude.m_preludeCRC;
    if (calculatedCRC != providedCRC) {
      PTRACE(2, "Received message header CRC failed: " << calculatedCRC << "!=" << providedCRC);
      return false;
    }

    size_t crcPos = message.size() - sizeof(uint32_t);
    calculatedCRC = PCRC32::Calculate(message, crcPos);
    providedCRC = message.GetAs<PUInt32b>(crcPos);
    if (calculatedCRC != providedCRC) {
      PTRACE(2, "Received message CRC failed: " << calculatedCRC << "!=" << providedCRC);
      return false;
    }

    const BYTE * ptr = message.GetPointer() + sizeof(prelude);
    const BYTE * endHeaders = ptr + prelude.m_headerLen;
    while (ptr < endHeaders) {
      PString key(reinterpret_cast<const char *>(ptr)+1, *ptr);
      ptr += key.length() + 1;
      switch (*ptr++) {
        default:
          PTRACE(2, "Received message with invalid header");
          return false;
        case 0:
          headers.SetBoolean(key, true);
          break;
        case 1:
          headers.SetBoolean(key, false);
          break;
        case 2:
          headers.SetInteger(key, *ptr++);
          break;
        case 3:
          headers.SetInteger(key, *reinterpret_cast<const PUInt16b *>(ptr));
          ptr += sizeof(PUInt16b);
          break;
        case 4:
          headers.SetInteger(key, *reinterpret_cast<const PUInt32b *>(ptr));
          ptr += sizeof(PUInt32b);
          break;
        case 5:
          headers.SetVar(key, *reinterpret_cast<const PUInt64b *>(ptr));
          ptr += sizeof(PUInt64b);
          break;
        case 6:
        case 7:
        case 8:
          uint16_t len = *reinterpret_cast<const PUInt16b *>(ptr);
          headers.SetString(key, PString(reinterpret_cast<const char *>(ptr)+sizeof(PUInt16b), len));
          ptr += len + sizeof(PUInt16b);
          break;
      }
    }

    contents = PBYTEArray(ptr, message.size() - sizeof(prelude) - prelude.m_headerLen - sizeof(uint32_t));
    return true;
  }

  void ReadEvents()
  {
    PTRACE_CONTEXT_ID_PUSH_THREAD(*this);
    PTRACE(4, "Reading events thread started");

    {
      PWaitAndSignal lock(m_mutex);

      HeaderMap headers;
      headers["media-encoding"] = "pcm";
      headers["sample-rate"] = GetSampleRate();

      if (m_activeVocabularies.empty())
        headers["language-code"] = GetLanguage();
      else {
        if (m_activeVocabularies.size() > 1)
          headers["vocabulary-names"] = PSTRSTRM(setfill(',') << m_activeVocabularies);
        else
          headers["vocabulary-name"] = *m_activeVocabularies.begin();

        if (m_activeLanguages.empty())
          headers["language-code"] = GetLanguage();
        else if (m_activeLanguages.size() == 1)
          headers["language-code"] = *m_activeLanguages.begin();
        else {
          headers["identify-language"] = "true";
          headers["language-options"] = PSTRSTRM(setfill(',') << m_activeLanguages);
        }
      }

      headers.insert(m_additionalHeaders.begin(), m_additionalHeaders.end());

      m_webSocket.SetBinaryMode();

      PURL url = SignURL("wss://transcribestreaming:8443/stream-transcription-websocket", "transcribe", headers);
      PTRACE(3, "Opening " << url);
      if (!m_webSocket.Connect(url))
        PTRACE(2, "Error opening WebSocket: " << m_webSocket.GetErrorText());
      PTRACE(4, "Opened: " << m_webSocket.GetName());
      m_openingWebSocket = false;
    }

    Transcript transcript;
    while (m_webSocket.IsOpen()) {
      PBYTEArray data;
      PStringOptions headers;
      if (ReadEvent(data, headers)) {
        PTRACE(5, "Read " << data.size() << " bytes, headers:\n" << headers);

        PJSON json(PJSON::e_Null);
        if (headers.Get(":content-type") == PMIMEInfo::ApplicationJSON())
          json.FromString(data);

        PString messageType = headers.Get(":message-type");
        if (messageType == "event") {
          PString type = headers.Get(":event-type");
          if (type == "TranscriptEvent" &&
              json.IsObject() &&
              json.GetObject().IsObject("Transcript") &&
              json.GetObject().GetObject("Transcript").IsObject("Results")) {
            PJSON::Array const & results = json.GetObject().GetObject("Transcript").GetArray("Results");
            PTRACE_IF(4, !results.empty(), "Received " << type << ":\n" << json.AsString(4, 2));
            for (size_t idxResult = 0; idxResult < results.size(); ++idxResult) {
              if (results.IsObject(idxResult)) {
                PJSON::Object const & result = results.GetObject(idxResult);
                if (result.IsArray("Alternatives")) {
                  bool atLeastOnePronunciation = false;
                  PJSON::Array const & alternatives = result.GetArray("Alternatives");
                  for (size_t idxAlternative = 0; idxAlternative < alternatives.size(); ++idxAlternative) {
                    if (alternatives.GetObject(idxAlternative).IsArray("Items")) {
                      PJSON::Array const & items = alternatives.GetObject(idxAlternative).GetArray("Items");
                      for (size_t idxItem = 0; idxItem < items.size(); ++idxItem) {
                        if (items.IsObject(idxItem)) {
                          PJSON::Object const & item = items.GetObject(idxItem);
                          transcript.m_final = false;
                          transcript.m_when = PTimeInterval::Seconds((double)item.GetNumber("StartTime"));
                          transcript.m_content = item.GetString("Content");
                          if (item.GetString("Type") != "pronunciation")
                            transcript.m_confidence = 0;
                          else {
                            atLeastOnePronunciation = true;
                            transcript.m_confidence = (float)item.GetNumber("Confidence");
                            if (transcript.m_confidence <= 0)
                              transcript.m_confidence = 0.1f; // Need _some_ confidence in it!
                          }
                          if (!transcript.m_content.empty()) {
                            PTRACE(4, "Sending notification of " << transcript);
                            m_notifier(*this, transcript);
                          }
                        }
                      }
                    }
                  }
                  if (atLeastOnePronunciation && !result.GetBoolean("IsPartial")) {
                    transcript.m_final = true;
                    transcript.m_when = PTimeInterval::Seconds((double)result.GetNumber("StartTime"));
                    transcript.m_content = alternatives.GetObject(0).GetString("Transcript");
                    transcript.m_confidence = 1;
                    PTRACE(4, "Sending final notification of " << transcript);
                    m_notifier(*this, transcript);
                  }
                }
              }
            }
          }
          else {
            PTRACE(4, "Received " << type << ":\n" << json.AsString(4, 2));
          }
        }
#if PTRACING
        else if (PTrace::CanTrace(2) && messageType == "exception") {
          ostream & log = PTRACE_BEGIN(2);
          log << "Received exception " << headers.Get(":exception-type", "Unknown") << ", details:";
          if (json.IsType(PJSON::e_Null))
            log << '\n' << data;
          else {
            PString msg;
            if (json.IsObject())
              msg = json.GetObject().GetString("Message");
            else if (json.IsType(PJSON::e_String))
              msg = json.GetString();
            if (msg.empty())
              log << '\n' << json.AsString(0, 2);
            else
              log << (msg.Find('\n') != P_MAX_INDEX ? '\n' : ' ') << msg;
          }
          log << PTrace::End;
        }
#endif // PTRACING
      }
    }

    if (!transcript.m_final && !transcript.m_content.empty()) {
      transcript.m_final = true;
      PTRACE(4, "Sending fail safe notification of " << transcript);
      m_notifier(*this, transcript);
    }

    PTRACE(4, "Reading events thread ended");
  }


  void DeleteVocabulary(const PString & name)
  {
    if (name.empty() || !m_createdVocabularies.Contains(name))
      return;

    GetClient()->DeleteVocabulary(Aws::TranscribeService::Model::DeleteVocabularyRequest()
                                    .WithVocabularyName(name.c_str()));
    m_createdVocabularies -= name;
  }


public:
  ~PSpeechRecognition_AWS()
  {
    Close();

    while (!m_createdVocabularies.empty())
      DeleteVocabulary(*m_createdVocabularies.begin());
  }


  virtual bool SetOptions(const PStringOptions & options) override
  {
    // Get buffer size in milliseconds before the sample rate changes
    unsigned bufferMilliseconds = options.GetVar("AWS-Buffer-Duration", m_bufferSize * 1000 / m_sampleRate);
    bool ok1 = PSpeechRecognition::SetOptions(options); // Set sample rate (maybe)
    m_bufferSize = bufferMilliseconds * m_sampleRate / 1000;

    m_webSocket.SetProxies(options);

    m_additionalHeaders.clear();
    for (const auto & hdr : options.GetString("AWS-Header").Lines()) {
      PString name, value;
      if (hdr.Split(':', name, value))
        m_additionalHeaders[name] = value;
    }

    return PAwsClient<Aws::TranscribeService::TranscribeServiceClient>::SetOptions(options) && ok1;
  }


  virtual bool SetSampleRate(unsigned rate) override
  {
    PWaitAndSignal lock(m_mutex);
    if (IsOpen())
      return false;

    m_sampleRate = rate;
    return true;
  }


  virtual unsigned GetSampleRate() const override
  {
    return m_sampleRate;
  }


  virtual bool SetChannels(unsigned channels) override
  {
    return channels == 1;
  }


  virtual unsigned GetChannels() const override
  {
    return 1;
  }


  virtual bool SetLanguage(const PString & language) override
  {
    PWaitAndSignal lock(m_mutex);
    if (IsOpen())
      return false;

    auto code = Aws::TranscribeService::Model::LanguageCodeMapper::GetLanguageCodeForName(language.c_str());
    if (code ==  Aws::TranscribeService::Model::LanguageCode::NOT_SET)
      return false;

    m_language = code;
    return true;
  }


  virtual PString GetLanguage() const override
  {
    return Aws::TranscribeService::Model::LanguageCodeMapper::GetNameForLanguageCode(m_language);
  }


  virtual bool CreateVocabulary(const PString & name, const PStringArray & words) override
  {
    DeleteVocabulary(name);

    if (words.empty())
      return true;

    Aws::Vector<Aws::String> phrases(words.size());
    for (size_t i = 0; i < words.size(); ++i)
      phrases[i] = words[i].c_str();

    auto outcome = GetClient()->CreateVocabulary(Aws::TranscribeService::Model::CreateVocabularyRequest()
                                                  .WithVocabularyName(name.c_str())
                                                  .WithLanguageCode(m_language)
                                                  .WithPhrases(phrases));
    if (outcome.IsSuccess()) {
      m_createdVocabularies += name;
      return true;
    }

    PTRACE(2, "Could not create vocabulary: " << outcome.GetResult().GetFailureReason());
    return false;
  }


  virtual bool ActivateVocabulary(const PStringSet & names, const PStringSet & languages) override
  {
    m_activeVocabularies = names;
    m_activeLanguages = languages;
    return true;
  }


  virtual bool Open(const Notifier & notifier) override
  {
    PWaitAndSignal lock(m_mutex);
    Close();

    m_notifier = notifier;

    m_openingWebSocket = true;
    m_eventThread = new PThreadObj<PSpeechRecognition_AWS>(*this, &PSpeechRecognition_AWS::ReadEvents, false, "AWS-SR");
    PTRACE(3, "Opened speech recognition.");
    return true;
  }


  virtual bool IsOpen() const override
  {
    return m_notifier != Notifier();
  }


  virtual bool Close() override
  {
    PWaitAndSignal lock(m_mutex);
    if (!IsOpen())
      return false;

    PTRACE(3, "Closing " << *this);
    m_webSocket.CloseBaseReadChannel();
    PThread::WaitAndDelete(m_eventThread);

    return true;
  }


  virtual bool Listen(const int16_t * samples, size_t sampleCount) override
  {
    PWaitAndSignal lock(m_mutex);
    if (!IsOpen())
      return false;

    m_buffer.insert(m_buffer.end(), samples, samples+sampleCount);
    if (m_buffer.size() < m_bufferSize || m_openingWebSocket)
      return true;

    if (!WriteAudioEvent(m_buffer.data(), m_bufferSize))
      return false;

    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + m_bufferSize);
    return true;
  }
};

PFACTORY_CREATE(PFactory<PSpeechRecognition>, PSpeechRecognition_AWS, P_SPEECH_RECOGNITION_AWS, false);


#endif // P_AWS_SDK
