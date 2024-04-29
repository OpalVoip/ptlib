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
static PConstCaselessString DefaultVoiceEngine("TTS-Engine");

class PTextToSpeech_AWS : public PTextToSpeech, PAwsClient<Aws::Polly::PollyClient>
{
  PCLASSINFO(PTextToSpeech_AWS, PTextToSpeech);

  typedef PDictionary<PCaselessString, PStringOptions> VoiceDict;
  VoiceDict    m_voices;
  PString      m_defaultEngine;
  bool         m_sampleRate8k{ true };
  PWAVFile     m_file;
  PChannel   * m_channel{ NULL };

public:
  ~PTextToSpeech_AWS()
  {
    Close();
  }


  virtual bool SetOptions(const PStringOptions & options) override
  {
    PString engineStr = options.Get(DefaultVoiceEngine);
    if (!engineStr.empty()) {
      auto engine = Aws::Polly::Model::EngineMapper::GetEngineForName(engineStr.c_str());
      if (engine != Aws::Polly::Model::Engine::NOT_SET) {
        m_defaultEngine = Aws::Polly::Model::EngineMapper::GetNameForEngine(engine);
        PTRACE(3, "Using default engine: " << m_defaultEngine);
      }
    }

    return PAwsClient<Aws::Polly::PollyClient>::SetOptions(options) && PTextToSpeech::SetOptions(options);
  }


  virtual PStringArray GetVoiceList() override
  {
    PStringArray voices;
    if (m_voices.empty()) {
      //Enumerate the available voices
      auto outcome = GetClient()->DescribeVoices(Aws::Polly::Model::DescribeVoicesRequest());
      if (!outcome.IsSuccess()) {
        PTRACE(2, "Could not describe voices: " << outcome.GetError().GetMessage());
        return voices;
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
    }

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
    if (voice.Contains(VoiceEngine))
      return true;

    PStringArray engines = voice.Get(VoiceEngines, m_defaultEngine).ToLower().Tokenise("+", false);
    if (m_defaultEngine.empty())
      voice.Set(VoiceEngine, engines[0]);
    else if (engines.GetValuesIndex(m_defaultEngine) != P_MAX_INDEX)
      voice.Set(VoiceEngine, m_defaultEngine);
    else {
      PTRACE(2, "Cannot use engine \"" << m_defaultEngine << "\" with voice \"" << voice.Get(VoiceName) << '"');
      return false;
    }

    PTRACE(4, "Using voice \"" << voice.Get(VoiceName) << '"');
    return true;
  }


  virtual bool InternalSetVoice(PStringOptions & voice) override
  {
    if (voice == m_voice)
      return ValidateEngine(voice);

    if (m_voices.empty())
      GetVoiceList();

    PCaselessString language = voice.Get(VoiceLanguage);
    PCaselessString gender = voice.Get(VoiceGender);
    PCaselessString engine = voice.Get(VoiceEngine);

    if (voice.Contains(VoiceName)) {
      PCaselessString voiceName = voice.Get(VoiceName);
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
      if (!engine.empty() && engine != found->Get(VoiceEngine)) {
        PTRACE(2, "Voice \"" << voiceName << "\" has incorrect engine: "
               << engine << " != " << found->Get(VoiceEngine));
        return false;
      }
      voice = *found;
      voice.MakeUnique();
      return ValidateEngine(voice);
    }

    for (const auto & it : m_voices) {
      if ((language.empty() || language == it.second.Get(VoiceLanguage)) &&
          (gender.empty() || gender == it.second.Get(VoiceGender)) &&
          (engine.empty() || engine == it.second.Get(VoiceEngine) || it.second.Get(VoiceEngines).Find(engine) != P_MAX_INDEX)) {
        voice = it.second;
        voice.MakeUnique();
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


  bool OpenFile(const PFilePath & fn) override
  {
    if (m_file.Open(fn, PFile::WriteOnly))
      return true;

    PTRACE(2, "Could not open \"" << fn << "\" - " << m_file.GetErrorText());
    return false;
  }


  bool OpenChannel(PChannel * channel) override
  {
    Close();
    return m_channel = channel;
  }


  bool IsOpen() const override
  {
    return m_channel != NULL || m_file.IsOpen();
  }


  bool Close() override
  {
    m_file.Close();
    m_channel = NULL;
    return true;
  }


  bool Speak(const PString & text, TextType hint) override
  {
    if (!IsOpen() || !SetVoice(PString::Empty()))
      return false;

    Aws::Polly::Model::SynthesizeSpeechRequest request;

    request.SetOutputFormat(Aws::Polly::Model::OutputFormat::pcm);
    request.SetSampleRate(m_sampleRate8k ? "8000" : "16000");
    request.SetVoiceId(Aws::Polly::Model::VoiceIdMapper::GetVoiceIdForName(m_voice.Get(VoiceName).c_str()));
    request.SetLanguageCode(Aws::Polly::Model::LanguageCodeMapper::GetLanguageCodeForName(m_voice.Get(VoiceLanguage).c_str()));
    request.SetEngine(Aws::Polly::Model::EngineMapper::GetEngineForName(m_voice.Get(VoiceEngine).c_str()));

    static const char * HintToAWS[TextType::NumTextType] = {
      NULL, // Default,
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
    };
    if (HintToAWS[hint] == NULL)
      request.SetText(text.c_str());
    else {
      PStringStream strm;
      strm << "<say-as interpret-as=\"" << HintToAWS[hint] << '"';
      if (hint == Date) {
        static const char * DateOrder[] = { "mdy", "dmy", "ymd" };
        strm << " format=\"" << DateOrder[PTime::GetDateOrder()] << '"';
      }
      strm << '>'
           << PXML::EscapeSpecialChars(text)
           << "</say-as>";
      request.SetText(strm.c_str());
      request.SetTextType(Aws::Polly::Model::TextType::ssml);
    }

    auto outcome = GetClient()->SynthesizeSpeech(request);
    if (!outcome.IsSuccess()) {
      PTRACE(2, "Could not synthesize speech: " << outcome.GetError().GetMessage());
      return false;
    }

    auto & strm = outcome.GetResult().GetAudioStream();
    for (;;) {
      char buf[1000];
      strm.read(buf, sizeof(buf));
      auto count = strm.gcount();
      if (count == 0)
        break;

      if (m_channel != NULL ? m_channel->Write(buf, count) : m_file.Write(buf, count))
        continue;

      PTRACE(2, "Write failure to \"" << m_file.GetFilePath() << "\" - " << m_file.GetErrorText());
      return false;
    }

    return true;
  }
};


PFACTORY_CREATE(PFactory<PTextToSpeech>, PTextToSpeech_AWS, P_TEXT_TO_SPEECH_AWS, false);


////////////////////////////////////////////////////////////
// Speech recognition using Microsoft's Speech API (SAPI)

class PSpeechRecognition_AWS : public PSpeechRecognition, PAwsClient<Aws::TranscribeService::TranscribeServiceClient>
{
  PCLASSINFO(PSpeechRecognition_AWS, PSpeechRecognition);

  unsigned m_sampleRate;
  Aws::TranscribeService::Model::LanguageCode m_language;
  PStringSet m_createdVocabularies;
  PStringSet m_activeVocabularies;
  PStringSet m_activeLanguages;

  PWebSocket m_webSocket;
  PThread  * m_eventThread;
  PDECLARE_MUTEX(m_openMutex);
  PDECLARE_MUTEX(m_eventMutex);

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
    if (!m_webSocket.ReadMessage(message))
      return false;

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
    PTRACE(4, "Reading events thread started");
    std::set<Transcript> sentTranscripts;
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
              json.IsType(PJSON::e_Object) &&
              json.GetObject().IsType("Transcript", PJSON::e_Object) &&
              json.GetObject().GetObject("Transcript").IsType("Results", PJSON::e_Array)) {
            PJSON::Array const & results = json.GetObject().GetObject("Transcript").GetArray("Results");
            PTRACE_IF(4, !results.empty(), "Received " << type << ":\n" << json.AsString(0, 2));
            for (size_t idxResult = 0; idxResult < results.size(); ++idxResult) {
              if (results.IsType(idxResult, PJSON::e_Object)) {
                PJSON::Object const & result = results.GetObject(idxResult);
                if (result.IsType("Alternatives", PJSON::e_Array)) {
                  PJSON::Array const & alternatives = result.GetArray("Alternatives");
                  for (size_t idxAlternative = 0; idxAlternative < alternatives.size(); ++idxAlternative) {
                    if (alternatives.GetObject(idxAlternative).IsType("Items", PJSON::e_Array)) {
                      PJSON::Array const & items = alternatives.GetObject(idxAlternative).GetArray("Items");
                      for (size_t idxItem = 0; idxItem < items.size(); ++idxItem) {
                        if (items.IsType(idxItem, PJSON::e_Object)) {
                          PJSON::Object const & item = items.GetObject(idxItem);
                          if (item.GetString("Type") == "pronunciation") {
                            Transcript transcript(false,
                                                  PTimeInterval::Seconds((double)item.GetNumber("StartTime")),
                                                  item.GetString("Content"),
                                                  (float)item.GetNumber("Confidence"));
                            if (!transcript.m_content.empty() && sentTranscripts.find(transcript) == sentTranscripts.end()) {
                              sentTranscripts.insert(transcript);
                              PTRACE(4, "Sending notification of " << transcript);
                              m_notifier(*this, transcript);
                            }
                          }
                        }
                      }
                    }
                  }
                  if (!alternatives.empty() && !result.GetBoolean("IsPartial")) {
                    Transcript transcript(true,
                                          PTimeInterval::Seconds((double)result.GetNumber("StartTime")),
                                          alternatives.GetObject(0).GetString("Transcript"),
                                          1);
                    PTRACE(4, "Sending notification of " << transcript);
                    m_notifier(*this, transcript);
                  }
                }
              }
            }
          }
          else {
            PTRACE(4, "Received " << type << ":\n" << json.AsString(0, 2));
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
            if (json.IsType(PJSON::e_Object))
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
  PSpeechRecognition_AWS()
    : m_sampleRate(8000)
    , m_language(Aws::TranscribeService::Model::LanguageCode::en_US)
    , m_eventThread(NULL)
  { }


  ~PSpeechRecognition_AWS()
  {
    Close();

    while (!m_createdVocabularies.empty())
      DeleteVocabulary(*m_createdVocabularies.begin());
  }


  virtual bool SetOptions(const PStringOptions & options) override
  {
    m_webSocket.SetProxies(options);
    return PAwsClient<Aws::TranscribeService::TranscribeServiceClient>::SetOptions(options) && PSpeechRecognition::SetOptions(options);
  }


  virtual bool SetSampleRate(unsigned rate) override
  {
    PWaitAndSignal lock(m_openMutex);
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
    PWaitAndSignal lock(m_openMutex);
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

    PTRACE(2, "Could not create vocubulary: " << outcome.GetResult().GetFailureReason());
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
    PWaitAndSignal lock(m_openMutex);
    Close();

    PTRACE(4, "Opening speech recognition.");

    m_notifier = notifier;

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

    PURL url = SignURL("wss://transcribestreaming:8443/stream-transcription-websocket", "transcribe", headers);

    if (!m_webSocket.Connect(url))
      return false;

    m_webSocket.SetBinaryMode();
    m_eventThread = new PThreadObj<PSpeechRecognition_AWS>(*this, &PSpeechRecognition_AWS::ReadEvents, false, "AWS-SR");
    PTRACE(3, "Opened speech recognition.");
    return true;
  }


  virtual bool IsOpen() const override
  {
    return m_webSocket.IsOpen();
  }


  virtual bool Close() override
  {
    PWaitAndSignal lock(m_openMutex);
    if (!IsOpen())
      return false;

    PTRACE(3, "Closing speech recognition.");
    m_webSocket.Close();
    PThread::WaitAndDelete(m_eventThread);

    return true;
  }


  virtual bool Listen(const PFilePath & fn) override
  {
    if (!PAssert(!fn.empty(), PInvalidParameter))
      return false;

    PWaitAndSignal lock(m_openMutex);
    if (!m_webSocket.IsOpen())
      return false;

    PWAVFile wavFile;
    if (!wavFile.Open(fn, PFile::ReadOnly))
      return false;

    vector<int16_t> buffer(4096);
    while (wavFile.Read(buffer.data(), buffer.size()*sizeof(int16_t))) {
      if (!WriteAudioEvent(buffer.data(), wavFile.GetLastReadCount()/sizeof(int16_t)))
        return false;
    }

    WriteAudioEvent(NULL, 0); // Empty event ends the stream
    PTRACE(4, "Written " << fn);
    return wavFile.GetErrorCode() == PChannel::NoError;
  }


  virtual bool Listen(const int16_t * samples, size_t sampleCount) override
  {
    PWaitAndSignal lock(m_openMutex);
    return m_webSocket.IsOpen() && WriteAudioEvent(samples, sampleCount);
  }
};

PFACTORY_CREATE(PFactory<PSpeechRecognition>, PSpeechRecognition_AWS, P_SPEECH_RECOGNITION_AWS, false);


#endif // P_AWS_SDK
