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
static const PConstCaselessString DeepgramFluxPrefix("DeepgramFlux-");
static const PConstCaselessString EndpointTag("Endpoint");
static const PConstCaselessString ApiKeyTag("API-Key");
static const PConstCaselessString BufferDurationTag("Buffer-Duration");
static const PConstCaselessString ShortInputMaxLenTag("shortinput-maxlen");
static const PConstCaselessString ShortInputTimeoutTag("shortinput-timeout");
static const PConstCaselessString FinalHeuristicTag("Deepgram-IsFinal-Heuristic");
static const PConstCaselessString LanguageHintTag("LanguageHint");

static const PConstCaselessString SampleRateHeader("sample_rate");
static const PConstCaselessString ChannelsHeader("channels");
static const PConstCaselessString LanguageHeader("language");
static const PConstCaselessString EncodingHeader("encoding");
static const PConstCaselessString ModelHeader("model");
static const PConstCaselessString DefaultModel("flux-general-en");

static PStringSet MakeSkipParameters()
{
  PStringSet set;
  set += DeepgramPrefix + EndpointTag;
  set += DeepgramPrefix + ApiKeyTag;
  set += DeepgramPrefix + BufferDurationTag;
  set += DeepgramPrefix + SampleRateHeader;
  set += DeepgramPrefix + ChannelsHeader;
  set += DeepgramPrefix + LanguageHeader;
  set += DeepgramPrefix + EncodingHeader;
  set += DeepgramPrefix + ShortInputMaxLenTag;
  set += DeepgramPrefix + ShortInputTimeoutTag;
  set += DeepgramFluxPrefix + EndpointTag;
  set += DeepgramFluxPrefix + ApiKeyTag;
  set += DeepgramFluxPrefix + BufferDurationTag;
  set += DeepgramFluxPrefix + SampleRateHeader;
  set += DeepgramFluxPrefix + ChannelsHeader;
  set += DeepgramFluxPrefix + LanguageHeader;
  set += DeepgramFluxPrefix + LanguageHintTag;
  set += DeepgramFluxPrefix + EncodingHeader;
  set += DeepgramFluxPrefix + ShortInputMaxLenTag;
  set += DeepgramFluxPrefix + ShortInputTimeoutTag;
  set += FinalHeuristicTag;
  return set;
};


////////////////////////////////////////////////////////////
// Speech recognition using Deepgram
// https://developers.deepgram.com/reference/speech-to-text-api/listen-streaming

class PSpeechRecognition_Deepgram : public PSpeechRecognition
{
  const bool m_flux;
  const PString m_prefix;

  unsigned m_sampleRate;
  unsigned m_channels;
  PString m_language;
  bool m_languageHint = true;
  PString m_endpoint;
  PString m_apiKey;
  unsigned m_shortInputMaxLen;
  PTimeInterval m_shortInputTimeout;
  bool m_isFinalHeuristic;
  PHTTP::Proxies m_httpProxies;
  PStringToString m_options;
  unsigned m_bufferSize = 800; // Default 100ms

  PWebSocket m_webSocket;
  PThread  * m_eventThread = nullptr;
  bool m_openingWebSocket = false;
  std::vector<int16_t> m_buffer;


  void ReadEvents()
  {
    PTRACE_CONTEXT_ID_PUSH_THREAD(*this);
    PTRACE(4, "Reading events thread started");

    {
      PWaitAndSignal lock(m_mutex);

      if (!m_openingWebSocket) {
        PTRACE(4, "Closed before we opened WebSocket, thread ended.");
        return;
      }

      PURL url(m_endpoint, "wss");
      url.AppendPath("listen");
      for (PStringOptions::const_iterator opt = m_options.begin(); opt != m_options.end(); ++opt)
        url.SetQueryVar(opt->first, opt->second);
      url.SetQueryVar(SampleRateHeader, m_sampleRate);
      if (!m_flux)
        url.SetQueryVar(ChannelsHeader, m_channels);
      if (m_languageHint)
        url.SetQueryVar(LanguageHeader, m_language);

      m_webSocket.SetAuthorization("token " + m_apiKey);
      m_webSocket.SetProxies(m_httpProxies);
      m_webSocket.SetBinaryMode();

      PTRACE(3, "Opening " << url);
      if (!m_webSocket.Connect(url))
        PTRACE(2, "Error opening WebSocket: " << m_webSocket.GetErrorText());
      PTRACE(4, "Opened: " << m_webSocket.GetName());
      m_openingWebSocket = false;
    }

    Transcript transcript;
    while (m_webSocket.IsOpen()) {
      PBYTEArray data;
      if (!m_webSocket.ReadMessage(data)) {
        if (m_webSocket.IsOpen()) {
          PTRACE_IF(2, m_webSocket.GetErrorCode() != PChannel::NoError,
                    "Error reading WebSocket: " << m_webSocket.GetErrorText());
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

      if (!json.IsObject()) {
        PTRACE(2, "Unexpected JSON:\n" << json.AsString(4, 2));
        continue;
      }

      PTRACE(4, "Received:\n" << json.AsString(4, 2));

      const PJSON::Object & result = json.GetObject();
      PCaselessString resultType = result.GetString("type");
      if (resultType == "Results") {
        if (!result.IsObject("channel")) {
          PTRACE(2, "Unexpected JSON:\n" << json.AsString(4, 2));
          continue;
        }
        const PJSON::Object & channel = result.GetObject("channel");
        if (!channel.IsArray("alternatives")) {
          PTRACE(2, "Unexpected JSON:\n" << json.AsString(4, 2));
          continue;
        }
        const PJSON::Array & alternatives = channel.GetArray("alternatives");
        for (size_t idxAlternative = 0; idxAlternative < alternatives.size(); ++idxAlternative) {
          if (!alternatives.IsObject(idxAlternative)) {
            PTRACE(2, "Unexpected JSON:\n" << json.AsString(4, 2));
            continue;
          }
          const PJSON::Object & alternative = alternatives.GetObject(idxAlternative);
          auto partial = alternatives.GetObject(idxAlternative).GetString("transcript");
          if (!partial.empty()) {
            bool isFinal = result.GetBoolean("is_final");
            if (isFinal)
              transcript.m_content &= partial;
            transcript.m_when = PTimeInterval::Seconds((double)result.GetNumber("start"));
            if (transcript.m_content.length() > m_shortInputMaxLen || transcript.m_when > m_shortInputTimeout)
              transcript.m_final = m_isFinalHeuristic ? isFinal : result.GetBoolean("speech_final");
            else
              PTRACE_IF(4, m_shortInputMaxLen > 0, "Ignoring short partial result for " << m_shortInputTimeout << 's');

            auto confidence = (float)alternative.GetNumber("confidence");
            if (confidence > 0 && transcript.m_confidence > confidence)
              transcript.m_confidence = confidence; // Use the lowest confidence level of all the pieces

            PTRACE(4, "Sending notification of " << transcript);
            m_notifier(*this, transcript);

            if (transcript.m_final) {
              transcript.m_final = false;
              transcript.m_confidence = 1.0f;
              transcript.m_content.MakeEmpty();
            }
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
      else if (resultType == "TurnInfo") {
        PCaselessString event = result.GetString("event");
        transcript.m_final = event =="EndOfTurn";
        transcript.m_when = PTimeInterval::Seconds((double)result.GetNumber("audio_window_end"));
        transcript.m_content = result.GetString("transcript");
        transcript.m_confidence = (float)result.GetNumber("end_of_turn_confidence");
        if (!transcript.m_content.empty()) {
          PTRACE(4, "Sending notification of " << transcript);
          m_notifier(*this, transcript);
        }
      }
    }

    if (!transcript.m_final && !transcript.m_content.empty()) {
      transcript.m_final = true;
      PTRACE(4, "Sending fail safe notification of " << transcript);
      m_notifier(*this, transcript);
    }

    PTRACE(4, "Reading events thread ended");
  }

protected:
  PSpeechRecognition_Deepgram(bool flux)
    : m_flux(flux)
    , m_prefix(flux ? DeepgramFluxPrefix : DeepgramPrefix)
    , m_sampleRate(8000)
    , m_channels(1)
    , m_language("en-US")
    , m_shortInputMaxLen(0) // No short input heuristic by default
    , m_isFinalHeuristic(false)
  {
    PConfig env(PConfig::Environment);
    m_apiKey = env.GetString("DEEPGRAM_API_KEY");
    if (flux) {
      m_endpoint = env.GetString("DEEPGRAM_FLUX_ENDPOINT", "wss://api.deepgram.com/v2");
      m_options.SetAt(ModelHeader, DefaultModel);
      m_options.SetAt("eot_timeout_ms", "1000");
    }
    else {
      m_endpoint = env.GetString("DEEPGRAM_ENDPOINT", "wss://api.deepgram.com/v1");
      m_options.SetAt("utterance_end_ms", "2000");
      m_options.SetAt("interim_results", "true");
    }
    m_options.SetAt(EncodingHeader, "linear16");
    PTRACE(4, "Created " << this);
  }


public:
  ~PSpeechRecognition_Deepgram()
  {
    PTRACE(4, "Destroying " << this);
    Close();
  }


  virtual bool SetOptions(const PStringOptions & options) override
  {
    PWaitAndSignal lock(m_mutex);
    if (IsOpen())
      return false;

    m_endpoint = options.GetString(m_prefix + EndpointTag, m_endpoint);
    m_apiKey = options.GetString(m_prefix + ApiKeyTag, m_apiKey);
    m_shortInputMaxLen = options.GetVar(m_prefix + ShortInputMaxLenTag, m_shortInputMaxLen);
    m_shortInputTimeout = options.GetVar(m_prefix + ShortInputTimeoutTag, m_shortInputTimeout);
    m_isFinalHeuristic = options.GetBoolean(FinalHeuristicTag, m_isFinalHeuristic);
    m_httpProxies.FromOptions(options);

    static PStringSet SkipParams = MakeSkipParameters();
    for (PStringOptions::const_iterator opt = options.begin(); opt != options.end(); ++opt) {
      if (opt->first.starts_with(m_prefix) && !SkipParams.Contains(opt->first))
        m_options.SetAt(opt->first.Mid(m_prefix.length()), opt->second);
    }

    m_languageHint = options.GetBoolean(m_prefix + LanguageHintTag, !m_flux || m_options(ModelHeader) != DefaultModel);

    // Get buffer size in milliseconds before the sample rate changes
    unsigned bufferMilliseconds = options.GetVar(m_prefix + BufferDurationTag, m_bufferSize * 1000 / m_sampleRate);
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

    m_notifier = notifier;

    m_openingWebSocket = true;
    m_eventThread = new PThreadObj<PSpeechRecognition_Deepgram>(*this,
                                                                &PSpeechRecognition_Deepgram::ReadEvents,
                                                                false,
                                                                "AWS-SR");
    PTRACE(3, "Opened speech recognition.");
    return true;
  }


  virtual bool IsOpen() const override
  {
    return m_notifier != Notifier();
  }


  virtual bool Close() override
  {
    m_mutex.Wait();
    PTRACE(3, (IsOpen() ? "Closing " : "Already closed") << this);
    m_webSocket.CloseBaseReadChannel();
    m_notifier = Notifier();
    m_openingWebSocket = false; // In case we close while still opening
    return PThread::WaitAndDelete(m_eventThread, 5000, &m_mutex, false);
  }


  virtual bool Listen(const int16_t * samples, size_t sampleCount) override
  {
    PWaitAndSignal lock(m_mutex);
    if (!IsOpen())
      return false;

    m_buffer.insert(m_buffer.end(), samples, samples+sampleCount);
    if (m_buffer.size() < m_bufferSize || m_openingWebSocket)
      return true;

    if (!m_webSocket.Write(m_buffer.data(), m_bufferSize*sizeof(int16_t))) {
      PTRACE(2, "Error writing to WebSocket: " << m_webSocket.GetErrorText());
      return false;
    }

    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + m_bufferSize);
    return true;
  }
};

class PSpeechRecognition_DeepgramBasic : public PSpeechRecognition_Deepgram
{
public:
  PSpeechRecognition_DeepgramBasic()
    : PSpeechRecognition_Deepgram(false)
  { }
};

PFACTORY_CREATE(PFactory<PSpeechRecognition>, PSpeechRecognition_DeepgramBasic, P_SPEECH_RECOGNITION_DEEPGRAM, false);

class PSpeechRecognition_DeepgramFlux : public PSpeechRecognition_Deepgram
{
public:
  PSpeechRecognition_DeepgramFlux()
    : PSpeechRecognition_Deepgram(true)
  { }
};

PFACTORY_CREATE(PFactory<PSpeechRecognition>, PSpeechRecognition_DeepgramFlux, P_SPEECH_RECOGNITION_DEEPGRAM "Flux", false);
