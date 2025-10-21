/*
 * speech_11labs.cxx
 *
 * 11Labs interface wrapper for speech
 *
 * Portable Windows Library
 *
 * Copyright (c) 2025 Imagicle
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
 * The Initial Developer of the Original Code is Imagicle.
 *
 * Contributor(s): ______________________________________.
 */

#include <ptlib.h>
#include <ptclib/speech.h>
#include <ptclib/http.h>
#include <ptclib/pjson.h>
#include <ptclib/pwavfile.h>
#include <ptclib/qchannel.h>

#define PTraceModule() "SR-11Labs"

static const PConstCaselessString ApiKeyTag("11Labs-API-Key");
static const PConstCaselessString ModelIdTag("11Labs-Model-Id");
static const PConstCaselessString DefaultVoiceIdTag("11Labs-Default-Voice-Id");
static const PConstCaselessString VoiceId("id");

class PTextToSpeech_11Labs : public PTextToSpeech
{
    PCLASSINFO(PTextToSpeech_11Labs, PTextToSpeech);

  public:
    PTextToSpeech_11Labs();
    ~PTextToSpeech_11Labs();

    // Pure virtual overrides from PTextToSpeech
    virtual PStringArray GetVoiceList() override;
    virtual bool SetSampleRate(unsigned rate) override;
    virtual unsigned GetSampleRate() const override;
    virtual bool SetChannels(unsigned channels) override;
    virtual unsigned GetChannels() const override;
    virtual bool SetVolume(unsigned volume) override;
    virtual unsigned GetVolume() const override;
    virtual bool OpenFile(const PFilePath& fn) override;
    virtual bool IsOpen() const override;
    virtual bool Close() override;
    virtual bool Speak(const PString& text, TextType hint = Default) override;

    // Optional virtual overrides
    virtual bool SetOptions(const PStringOptions& options) override;
    virtual bool OpenChannel(PChannel* channel) override;
    virtual PSharedPtr<PChannel> SpeakToChannel(const PString& text, TextType hint = Default) override;

  protected:
    // Protected virtual override
    virtual bool InternalSetVoice(PStringOptions& voice) override;
    bool UpdateVoiceCache();

  private:
    unsigned m_sampleRate;
    unsigned m_channels;
    unsigned m_volume;
    bool m_isOpen;
    PString m_modelId;
    PString m_defaultVoiceId;  // Instance-specific default voice ID
    PString m_baseUrl;
    PString m_apiKey;

    // Output handling
    PWAVFile m_file;
    PChannel* m_channel;
    PSharedPtr<PChannel> m_streamChannel;
    PThread * m_thread;

    bool InternalSpeak(const PString & text, bool streamed, PAutoPtr<PHTTPClient>& http, PMIMEInfo & replyHeaders);
    void StreamThread(PAutoPtr<PHTTPClient> http, PMIMEInfo replyHeaders);
};

PFACTORY_CREATE(PFactory<PTextToSpeech>, PTextToSpeech_11Labs, P_TEXT_TO_SPEECH_11LABS, false);

// Static member definitions
static PStringOptions const s_defaultVoice("name=Hope\nid=OYTbf65OHHFELVut7v2H"); // Default voice name
static std::map<PString, PStringOptions> s_validVoices{
  { "Hope", s_defaultVoice },
  { "OYTbf65OHHFELVut7v2H", s_defaultVoice }
};
PMutex s_voiceCacheMutex;


PTextToSpeech_11Labs::PTextToSpeech_11Labs()
  : m_sampleRate(8000)  // Set to 8kHz directly
  , m_channels(1)
  , m_volume(100)
  , m_isOpen(false)
  , m_modelId("eleven_turbo_v2_5")
  , m_defaultVoiceId("Hope")  // Default can be overridden via SetOptions
  , m_baseUrl("https://api.elevenlabs.io")
  , m_apiKey(PConfig(PConfig::Environment).GetString("11LABS_API_KEY"))
  , m_channel(nullptr)
  , m_thread(nullptr)
{
}


PTextToSpeech_11Labs::~PTextToSpeech_11Labs()
{
  Close();
}


bool PTextToSpeech_11Labs::SetOptions(const PStringOptions & options)
{
    // Configure API key
  m_apiKey = options.GetString(ApiKeyTag, m_apiKey);
  //m_apiKey = "sk_b7765728d1d1c596e9b3aef83132d40bae221f796cced826";

  // Configure custom model
  PString customModel = options.GetString(ModelIdTag, m_modelId);
  if (!customModel.IsEmpty())
    m_modelId = customModel;

// Configure custom default voice ID
  PString customDefaultVoice = options.GetString(DefaultVoiceIdTag, m_defaultVoiceId);
  if (!customDefaultVoice.IsEmpty()) {
    m_defaultVoiceId = customDefaultVoice;
    PTRACE(3, "Custom default voice set to: " << m_defaultVoiceId);
  }

  return PTextToSpeech::SetOptions(options);
}


bool PTextToSpeech_11Labs::SetSampleRate(unsigned rate)
{
  PTRACE(4, "Invoking ElevenLabs SetSampleRate: " << rate);
  m_sampleRate = rate;
  return true;
}


unsigned PTextToSpeech_11Labs::GetSampleRate() const
{
  return m_sampleRate;
}


bool PTextToSpeech_11Labs::SetChannels(unsigned channels)
{
  if (channels != 1)
    return false;

  m_channels = channels;
  return true;
}


unsigned PTextToSpeech_11Labs::GetChannels() const
{
  return m_channels;
}


bool PTextToSpeech_11Labs::SetVolume(unsigned volume)
{
  if (volume > 100)
    return false;

  m_volume = volume;
  return true;
}


unsigned PTextToSpeech_11Labs::GetVolume() const
{
  return m_volume;
}


bool PTextToSpeech_11Labs::OpenFile(const PFilePath & fn)
{
  PTRACE(4, "OpenFile: " << fn);

  Close();

  m_file.SetChannels(m_channels);
  m_file.SetSampleRate(m_sampleRate);
  if (m_file.Open(fn, PFile::WriteOnly))
    return true;

  PTRACE(2, "Could not open \"" << fn << "\" - " << m_file.GetErrorText());
  return false;
}


bool PTextToSpeech_11Labs::IsOpen() const
{
  if (m_channel || m_file.IsOpen())
    return true;

// Only consider "open" if we have an active file OR a valid streaming channel
// This prevents the VXML system from trying to use a TTS engine that's being shut down
  auto channel = m_streamChannel;
  return channel && channel->IsOpen();
}


bool PTextToSpeech_11Labs::OpenChannel(PChannel * channel)
{
  Close();
  m_channel = channel;
  return channel != nullptr;
}


bool PTextToSpeech_11Labs::Close()
{
  PTRACE(4, "Closing");

  m_isOpen = false;
  m_file.Close();
  m_channel = nullptr;

  auto channel = std::exchange(m_streamChannel, nullptr);
  if (channel) {
    PTRACE(4, "Closing streaming channel gracefully");
    channel->Close();
  }

  PThread::WaitAndDelete(m_thread);

  return true;
}


bool PTextToSpeech_11Labs::Speak(const PString & text, TextType /*hint*/)
{
  if (!IsOpen())
    return false;

  PAutoPtr<PHTTPClient> http;
  PMIMEInfo replyHeaders;
  if (!InternalSpeak(text, false, http, replyHeaders))
    return false;

  PHTTPChannelProcessor processor(m_channel ? *m_channel : m_file, true);
  if (!http->ReadContentBody(replyHeaders, processor)) {
    PTRACE(2, "Write failure to channel: " << http->GetLastResponseInfo());
    return false;
  }

  PTRACE(3, "Played text");
  return true;
}


PSharedPtr<PChannel> PTextToSpeech_11Labs::SpeakToChannel(const PString & text, TextType /*hint*/)
{
  PAutoPtr<PHTTPClient> http;
  PMIMEInfo replyHeaders;
  if (!InternalSpeak(text, true, http, replyHeaders))
    return nullptr;

  m_streamChannel.reset(new PQueueChannel(m_sampleRate*10)); // One second of PCM
  m_thread = new PThreadObj2Arg<PTextToSpeech_11Labs, PAutoPtr<PHTTPClient>, PMIMEInfo>(
    *this, http, replyHeaders, &PTextToSpeech_11Labs::StreamThread, false, "11LabsStrm");
  return m_streamChannel;
}


void PTextToSpeech_11Labs::StreamThread(PAutoPtr<PHTTPClient> http, PMIMEInfo replyHeaders)
{
  PTRACE(3, "Started streaming thread");

  PHTTPChannelProcessor processor(*m_streamChannel, true);
  if (http->ReadContentBody(replyHeaders, processor)) {
    PQueueChannel * channel = dynamic_cast<PQueueChannel *>(m_streamChannel.get());
    auto prevLength = channel->GetLength();
    PTRACE_IF(5, prevLength > 0, "Draining queue of " << prevLength << " bytes");

    unsigned noDrainCount = 0;
    while (prevLength > 0) {
      PThread::Sleep(100);

      auto nextLength = channel->GetLength();
      if (nextLength != prevLength)
        noDrainCount = 0;
      else if (noDrainCount++ > 4) {
        PTRACE(3, "Nothing draining queue.");
        break;
      }

      prevLength = nextLength;
    }
  }
  else {
    PTRACE(2, "Write failure to channel: " << http->GetLastResponseInfo());
  }

  m_streamChannel->Close();
  m_streamChannel.reset();

  PTRACE(3, "Finished streaming thread");
}


bool PTextToSpeech_11Labs::InternalSpeak(const PString & text,
                                         bool streamed,
                                         PAutoPtr<PHTTPClient> & http,
                                         PMIMEInfo & replyHeaders)
{
  if (text.IsEmpty()) {
    PTRACE(2, "Empty text");
    return false;
  }

  if (m_apiKey.IsEmpty()) {
    PTRACE(2, "Missing API key");
    return false;
  }

  http.reset(new PHTTPClient("PTLib 11Labs TTS Client"));
  http->SetReadTimeout(30000);

  // Use the streaming endpoint but get complete response
  PURL url(m_baseUrl);
  url.AppendPath("v1");
  url.AppendPath("text-to-speech");
  url.AppendPath(m_voice.Get(VoiceId));
  if (streamed)
    url.AppendPath("stream");
  url.SetQueryVar("output_format", PSTRSTRM("pcm_" << m_sampleRate));

  // Prepare the JSON request body
  PJSON json(PJSON::e_Object);
  PJSON::Object & settings = json.GetObject();
  settings.SetString("text", text);
  settings.SetString("model_id", m_modelId);

  PJSON::Object & voiceSettings = settings.SetObject("voice_settings");
  voiceSettings.SetNumber("stability", 0.5);
  voiceSettings.SetNumber("similarity_boost", 0.5);

  // Set up request headers
  PMIMEInfo requestHeaders;
  requestHeaders.Set(PHTTP::ContentTypeTag, PMIMEInfo::ApplicationJSON());
  requestHeaders.Set("Accept", "audio/pcm");
  requestHeaders.Set("xi-api-key", m_apiKey);

  PTRACE(4, "Calling 11Labs streaming API: " << url);

  // Make the POST request and get complete response
  if (!http->PostData(url, requestHeaders, json.AsString(), replyHeaders)) {
    PTRACE(2, "Streaming API failed with code: "
           << http->GetLastResponseCode() << ' ' << http->GetLastResponseInfo());
    return false;
  }

  PTRACE(3, "Receiving " << replyHeaders.GetString(PHTTP::ContentLengthTag)
         << " bytes of " << replyHeaders.GetString(PHTTP::ContentTypeTag));
  return true;
}


static PStringOptions AddVoice(const PJSON::Object & voiceObj)
{
  PStringOptions voice;

  PString name = voiceObj.GetString("name");
  PString id = voiceObj.GetString("voice_id");
  if (id.empty())
    return voice;

  if (name.empty())
    name = id;

  voice.Set(PTextToSpeech::VoiceName, name);
  voice.Set(VoiceId, id);

  PWaitAndSignal lock(s_voiceCacheMutex);
  s_validVoices.emplace(name, voice);
  s_validVoices.emplace(id, voice);
  return voice;
}


bool PTextToSpeech_11Labs::UpdateVoiceCache()
{
  PWaitAndSignal lock(s_voiceCacheMutex);

  if (s_validVoices.size() > 2)
    return true;

  // Create HTTP client for validation
  PHTTPClient http("PTLib 11Labs Voice Validator");
  http.SetReadTimeout(10000); // 10 second timeout

  // Use GET /v1/voices/{voice_id} endpoint
  PURL url(m_baseUrl);
  url.AppendPath("v1");
  url.AppendPath("voices");
  url.SetQueryVar("page_size", "100");

  PMIMEInfo requestHeaders;
  requestHeaders.Set("xi-api-key", m_apiKey);
  requestHeaders.Set("Accept", PMIMEInfo::ApplicationJSON());

  // Make GET request
  PString reply;
  if (!http.GetTextDocument(url, requestHeaders, reply)) {
    PTRACE(2, "Voice list API failed with code: "
            << http.GetLastResponseCode() << ' ' << http.GetLastResponseInfo());
    return false;
  }

  PJSON json(reply);
  if (json.IsType(PJSON::e_Object)) {
    const PJSON::Object & root = json.GetObject();
    if (root.IsType("voices", PJSON::e_Array)) {
      const PJSON::Array & arr = root.GetArray("voices");
      for (size_t i = 0; i < arr.size(); ++i) {
        if (arr.IsType(i, PJSON::e_Object))
          AddVoice(arr.GetObject(i));
      }
    }
  }

  return true;
}


bool PTextToSpeech_11Labs::InternalSetVoice(PStringOptions & voice)
{
  PString voiceId = voice.Get(VoiceName, m_defaultVoiceId);

  // Check cache first
  if (UpdateVoiceCache()) {
    PWaitAndSignal lock(s_voiceCacheMutex);
    auto it = s_validVoices.find(voiceId);
    if (it != s_validVoices.end()) {
      PTRACE(4, "Voice " << it->second.Get(VoiceName) << " found in cache: " << it->second.Get(VoiceId));
      voice = it->second;
      return true;
    }
  }

  if (m_apiKey.IsEmpty()) {
    PTRACE(2, "Cannot set voice: API key not configured");
    return false;
  }

  // Not cached - validate via API
  PTRACE(4, "Validating voice ID " << voiceId << " via API");

  // Create HTTP client for validation
  PHTTPClient http("PTLib 11Labs Voice Validator");
  http.SetReadTimeout(10000); // 10 second timeout

  // Use GET /v1/voices/{voice_id} endpoint
  PURL url(m_baseUrl);
  url.AppendPath("v1");
  url.AppendPath("voices");
  url.AppendPath(voiceId);

  PMIMEInfo requestHeaders;
  requestHeaders.Set("xi-api-key", m_apiKey);
  requestHeaders.Set("Accept", PMIMEInfo::ApplicationJSON());

  PTRACE(5, "Validating voice via: " << url);

  // Make GET request
  PString reply;
  if (!http.GetTextDocument(url, requestHeaders, reply)) {
    PTRACE(3, "Voice ID " << voiceId << " invalid (HTTP " << http.GetLastResponseCode() << ')');
    return false;
  }

  PJSON json(reply);
  if (!json.IsType(PJSON::e_Object))
    return false;

  voice = AddVoice(json.GetObject());
  if (voice.empty()) {
    PTRACE(3, "Voice name " << voiceId << " has no id");
    return false;
  }

  PTRACE(3, "Voice ID " << voiceId << " cached");
  return true;
}


PStringArray PTextToSpeech_11Labs::GetVoiceList()
{
  PStringArray voices;

  if (UpdateVoiceCache()) {
    PWaitAndSignal lock(s_voiceCacheMutex);
    for (const auto & v : s_validVoices)
      voices.AppendString(PSTRSTRM("name=" << v.second.Get(VoiceName) << ";"
                                   "id=" << v.second.Get(VoiceId)));
  }

  return voices;
}
