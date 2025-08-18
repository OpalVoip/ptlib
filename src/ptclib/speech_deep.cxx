/*
 * speech_deep.cxx
 *
 * Deepgram interface wrapper for speech
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

#include <ptclib/speech.h>
#include <ptclib/pwavfile.h>
#include <ptclib/random.h>
#include <ptclib/url.h>
#include <ptclib/cypher.h>
#include <ptclib/http.h>
#include <ptclib/pjson.h>

#define PTraceModule() "SR-Deep"


static const PConstCaselessString DeepgramPrefix("Deepgram-");
static const PConstCaselessString EndpointTag("Deepgram-Endpoint");
static const PConstCaselessString ApiKeyTag("Deepgram-API-Key");
static const PConstCaselessString BufferDurationTag("Deepgram-Buffer-Duration");

static const PConstCaselessString SampleRateHeader("sample_rate");
static const PConstCaselessString ChannelsHeader("channels");
static const PConstCaselessString LanguageHeader("language");
static const PConstCaselessString EncodingHeader("encoding");

static PStringSet MakeSkipParameters()
{
  PStringSet set;
  set += EndpointTag;
  set += ApiKeyTag;
  set += BufferDurationTag;
  set += DeepgramPrefix + SampleRateHeader;
  set += DeepgramPrefix + ChannelsHeader;
  set += DeepgramPrefix + LanguageHeader;
  set += DeepgramPrefix + EncodingHeader;
  return set;
};


////////////////////////////////////////////////////////////
// Speech recognition using Deepgram
// https://developers.deepgram.com/reference/speech-to-text-api/listen-streaming

class PSpeechRecognition_Deepgram : public PSpeechRecognition
{
  unsigned m_sampleRate;
  unsigned m_channels;
  PString m_language;
  PString m_endpoint;
  PString m_apiKey;
  PStringToString m_options;
  unsigned m_bufferSize = 800; // Default 100ms

  PWebSocket m_webSocket;
  PThread  * m_eventThread = nullptr;
  std::vector<int16_t> m_buffer;
  PDECLARE_MUTEX(m_mutex);


  void ReadEvents()
  {
    PTRACE(4, "Reading events thread started");
    Transcript transcript;
    while (m_webSocket.IsOpen()) {
      PBYTEArray data;
      if (!m_webSocket.ReadMessage(data)) {
        if (m_webSocket.IsOpen()) {
          PTRACE_IF(2, m_webSocket.GetErrorCode() != PChannel::NoError,
                    "Error reading WebSocket:" << m_webSocket.GetErrorText());
          Close();
        }
        break;
      }

      PTRACE(5, "Read " << data.size() << " bytes");

      PJSON json(PJSON::e_Null);
      if (!json.FromString(data)) {
        PTRACE(3, "Invalid JSON:\n" << ::PHexDump(data));
        continue;
      }
      PTRACE(4, "Received:\n" << json.AsString(4, 2));

      if (!json.IsType(PJSON::e_Object))
        continue;

      const PJSON::Object & result = json.GetObject();
      PCaselessString resultType = result.GetString("type");
      if (resultType == "Results") {
        if (!result.GetBoolean("is_final") || !result.IsType("channel", PJSON::e_Object))
          continue;
        const PJSON::Object & channel = result.GetObject("channel");
        if (!channel.IsType("alternatives", PJSON::e_Array))
          continue;
        const PJSON::Array & alternatives = channel.GetArray("alternatives");
        for (size_t idxAlternative = 0; idxAlternative < alternatives.size(); ++idxAlternative) {
          if (!alternatives.IsType(idxAlternative, PJSON::e_Object))
            continue;
          const PJSON::Object & alternative = alternatives.GetObject(idxAlternative);
          auto partial = alternatives.GetObject(idxAlternative).GetString("transcript");
          if (!partial.empty()) {
            transcript.m_content &= partial;
            auto confidence = (float)alternative.GetNumber("confidence");
            if (confidence > 0 && transcript.m_confidence > confidence)
              transcript.m_confidence = confidence; // Use the lowest confidence level of all the pieces
            PTRACE(4, "Sending notification of " << transcript);
            m_notifier(*this, transcript);
          }
        }
      }
      else if (resultType == "UtteranceEnd") {
        transcript.m_final = true;
        transcript.m_when = PTimeInterval::Seconds((double)result.GetNumber("last_word_end"));
        PTRACE(4, "Sending notification of " << transcript);
        m_notifier(*this, transcript);
        transcript.m_final = false;
        transcript.m_confidence = 1.0f;
        transcript.m_content.MakeEmpty();
      }
    }

    if (!transcript.m_final && !transcript.m_content.empty()) {
      transcript.m_final = true;
      PTRACE(4, "Sending fail safe notification of " << transcript);
      m_notifier(*this, transcript);
    }

    PTRACE(4, "Reading events thread ended");
  }


public:
  PSpeechRecognition_Deepgram()
    : m_sampleRate(8000)
    , m_channels(1)
    , m_language("en-US")
  {
    PConfig env(PConfig::Environment);
    m_apiKey = env.GetString("DEEPGRAM_API_KEY");
    m_endpoint = env.GetString("DEEPGRAM_ENDPOINT", "wss://api.deepgram.com/v1");
    m_options.SetAt("utterance_end_ms", "2000");
    m_options.SetAt("interim_results", "true");
  }


  ~PSpeechRecognition_Deepgram()
  {
    Close();
  }


  virtual bool SetOptions(const PStringOptions & options) override
  {
    PWaitAndSignal lock(m_mutex);
    if (IsOpen())
      return false;

    m_endpoint = options.GetString(EndpointTag, m_endpoint);
    m_apiKey = options.GetString(ApiKeyTag, m_apiKey);

    static PStringSet SkipParams = MakeSkipParameters();
    for (PStringOptions::const_iterator opt = options.begin(); opt != options.end(); ++opt) {
      if (opt->first.starts_with(DeepgramPrefix) && !SkipParams.Contains(opt->first))
        m_options.SetAt(opt->first.Mid(9), opt->second);
    }

    unsigned bufferMilliseconds = options.GetVar(BufferDurationTag, m_bufferSize * 1000 / m_sampleRate);
    if (!PSpeechRecognition::SetOptions(options)) // Set sample rate (maybe)
      return false;

    m_bufferSize = bufferMilliseconds * m_sampleRate / 1000;
    return true;
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
    PWaitAndSignal lock(m_mutex);
    return m_sampleRate;
  }


  virtual bool SetChannels(unsigned channels) override
  {
    PWaitAndSignal lock(m_mutex);
    if (IsOpen())
      return false;

    m_channels = channels;
    return true;
  }


  virtual unsigned GetChannels() const override
  {
    PWaitAndSignal lock(m_mutex);
    return m_channels;
  }


  virtual bool SetLanguage(const PString & language) override
  {
    PWaitAndSignal lock(m_mutex);
    if (IsOpen())
      return false;

    m_language = language;
    return true;
  }


  virtual PString GetLanguage() const override
  {
    PWaitAndSignal lock(m_mutex);
    return m_language;
  }


  virtual bool Open(const Notifier & notifier) override
  {
    PWaitAndSignal lock(m_mutex);
    Close();

    PTRACE(4, "Opening " << *this);

    m_notifier = notifier;
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

    m_notifier = Notifier();
    return true;
  }


  virtual bool Listen(const PFilePath & fn) override
  {
    if (!PAssert(!fn.empty(), PInvalidParameter))
      return false;

    return false;
  }


  virtual bool Listen(const int16_t * samples, size_t sampleCount) override
  {
    PWaitAndSignal lock(m_mutex);
    if (!IsOpen())
      return false;

    if (!m_webSocket.IsOpen()) {

      PURL url(m_endpoint);
      url.AppendPath("listen");
      for (PStringOptions::const_iterator opt = m_options.begin(); opt != m_options.end(); ++opt)
        url.SetQueryVar(opt->first, opt->second);
      url.SetQueryVar(SampleRateHeader, m_sampleRate);
      url.SetQueryVar(ChannelsHeader, m_channels);
      url.SetQueryVar(LanguageHeader, m_language);
      url.SetQueryVar(EncodingHeader, "linear16");

      m_webSocket.SetAuthorization("token " + m_apiKey);
      m_webSocket.SetBinaryMode();
      if (!m_webSocket.Connect(url))
        return false;

      m_eventThread = new PThreadObj<PSpeechRecognition_Deepgram>(*this, &PSpeechRecognition_Deepgram::ReadEvents, false, "AWS-SR");
      PTRACE(3, "Opened speech recognition.");
    }

    m_buffer.insert(m_buffer.end(), samples, samples+sampleCount);
    if (m_buffer.size() < m_bufferSize)
      return true;

    if (!m_webSocket.Write(m_buffer.data(), m_bufferSize*sizeof(int16_t))) {
      PTRACE(2, "Error writing to WebSocket: " << m_webSocket.GetErrorText());
      return false;
    }

    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + m_bufferSize);
    return true;
  }
};

PFACTORY_CREATE(PFactory<PSpeechRecognition>, PSpeechRecognition_Deepgram, P_SPEECH_RECOGNITION_DEEPGRAM, false);
