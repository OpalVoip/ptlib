/*
 * speech.cxx
 *
 * Text To Speech and Speech Recognition classes
 *
 * Portable Windows Library
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 */

#ifdef P_USE_PRAGMA
#pragma implementation "speech.h"
#endif

#include <ptlib.h>

#if P_SAPI

#include <ptclib/speech.h>

#include <ptlib/msos/ptlib/pt_atl.h>


#define PTraceModule() "SAPI"


#ifdef _MSC_VER
  #pragma comment(lib, "sapi.lib")
  #pragma comment(lib, "propsys.lib")
#endif

#pragma warning(push)
#pragma warning(disable:4996)
#include <sphelper.h>
#include <propvarutil.h>
#pragma warning(pop)


class PSpStreamFormat
{
  protected:
    SPSTREAMFORMAT m_spStreamFormat;

    PSpStreamFormat()
      : m_spStreamFormat(SPSF_8kHz16BitMono)
    {
    }

    bool SetSampleRate(unsigned rate)
    {
      switch (rate) {
        case 8000 :
          m_spStreamFormat = GetChannels() > 1 ? SPSF_8kHz16BitStereo : SPSF_8kHz16BitMono;
          return true;
        case 11000 :
        case 11025 :
          m_spStreamFormat = GetChannels() > 1 ? SPSF_11kHz16BitStereo : SPSF_11kHz16BitMono;
          return true;
        case 12000 :
          m_spStreamFormat = GetChannels() > 1 ? SPSF_12kHz16BitStereo : SPSF_12kHz16BitMono;
          return true;
        case 16000 :
          m_spStreamFormat = GetChannels() > 1 ? SPSF_16kHz16BitStereo : SPSF_16kHz16BitMono;
          return true;
        case 22000 :
        case 22050 :
          m_spStreamFormat = GetChannels() > 1 ? SPSF_22kHz16BitStereo : SPSF_22kHz16BitMono;
          return true;
        case 24000 :
          m_spStreamFormat = GetChannels() > 1 ? SPSF_24kHz16BitStereo : SPSF_24kHz16BitMono;
          return true;
        case 32000 :
          m_spStreamFormat = GetChannels() > 1 ? SPSF_32kHz16BitStereo : SPSF_32kHz16BitMono;
          return true;
        case 44000 :
        case 44100 :
          m_spStreamFormat = GetChannels() > 1 ? SPSF_44kHz16BitStereo : SPSF_44kHz16BitMono;
          return true;
        case 48000 :
          m_spStreamFormat = GetChannels() > 1 ? SPSF_8kHz16BitStereo : SPSF_48kHz16BitMono;
          return true;
      }
      return false;
    }


    unsigned GetSampleRate() const
    {
      switch (m_spStreamFormat) {
        case SPSF_8kHz16BitMono :
        case SPSF_8kHz16BitStereo :
          return 8000;
        case SPSF_11kHz16BitMono :
        case SPSF_11kHz16BitStereo :
          return 11025;
        case SPSF_12kHz16BitMono :
        case SPSF_12kHz16BitStereo :
          return 12000;
        case SPSF_16kHz16BitMono :
        case SPSF_16kHz16BitStereo :
          return 16000;
        case SPSF_22kHz16BitMono :
        case SPSF_22kHz16BitStereo :
          return 22050;
        case SPSF_24kHz16BitMono :
        case SPSF_24kHz16BitStereo :
          return 24000;
        case SPSF_32kHz16BitMono :
        case SPSF_32kHz16BitStereo :
          return 32000;
        case SPSF_44kHz16BitMono :
        case SPSF_44kHz16BitStereo :
          return 44100;
        case SPSF_48kHz16BitMono :
        case SPSF_48kHz16BitStereo :
          return 48000;
      }
      return 0;
    }


    bool SetChannels(unsigned channels)
    {
      switch (channels) {
        case 1:
          switch (m_spStreamFormat) {
            case SPSF_8kHz16BitMono :
            case SPSF_11kHz16BitMono :
            case SPSF_12kHz16BitMono :
            case SPSF_16kHz16BitMono :
            case SPSF_22kHz16BitMono :
            case SPSF_24kHz16BitMono :
            case SPSF_32kHz16BitMono :
            case SPSF_44kHz16BitMono :
            case SPSF_48kHz16BitMono :
              return true;
            case SPSF_8kHz16BitStereo :
              m_spStreamFormat = SPSF_8kHz16BitMono;
              return true;
            case SPSF_11kHz16BitStereo :
              m_spStreamFormat = SPSF_11kHz16BitMono;
              return true;
            case SPSF_12kHz16BitStereo :
              m_spStreamFormat = SPSF_12kHz16BitMono;
              return true;
            case SPSF_16kHz16BitStereo :
              m_spStreamFormat = SPSF_16kHz16BitMono;
              return true;
            case SPSF_22kHz16BitStereo :
              m_spStreamFormat = SPSF_22kHz16BitMono;
              return true;
            case SPSF_24kHz16BitStereo :
              m_spStreamFormat = SPSF_24kHz16BitMono;
              return true;
            case SPSF_32kHz16BitStereo :
              m_spStreamFormat = SPSF_16kHz16BitMono;
              return true;
            case SPSF_44kHz16BitStereo :
              m_spStreamFormat = SPSF_44kHz16BitMono;
              return true;
            case SPSF_48kHz16BitStereo :
              m_spStreamFormat = SPSF_48kHz16BitMono;
              return true;
          }
          break;

        case 2:
          switch (m_spStreamFormat) {
            case SPSF_8kHz16BitMono :
              m_spStreamFormat = SPSF_8kHz16BitStereo;
              return true;
            case SPSF_11kHz16BitMono :
              m_spStreamFormat = SPSF_11kHz16BitStereo;
              return true;
            case SPSF_12kHz16BitMono :
              m_spStreamFormat = SPSF_12kHz16BitStereo;
              return true;
            case SPSF_16kHz16BitMono :
              m_spStreamFormat = SPSF_16kHz16BitStereo;
              return true;
            case SPSF_22kHz16BitMono :
              m_spStreamFormat = SPSF_22kHz16BitStereo;
              return true;
            case SPSF_24kHz16BitMono :
              m_spStreamFormat = SPSF_24kHz16BitStereo;
              return true;
            case SPSF_32kHz16BitMono :
              m_spStreamFormat = SPSF_32kHz16BitStereo;
              return true;
            case SPSF_44kHz16BitMono :
              m_spStreamFormat = SPSF_44kHz16BitStereo;
              return true;
            case SPSF_48kHz16BitMono :
              m_spStreamFormat = SPSF_48kHz16BitStereo;
              return true;
            case SPSF_8kHz16BitStereo :
            case SPSF_11kHz16BitStereo :
            case SPSF_12kHz16BitStereo :
            case SPSF_16kHz16BitStereo :
            case SPSF_22kHz16BitStereo :
            case SPSF_24kHz16BitStereo :
            case SPSF_32kHz16BitStereo :
            case SPSF_44kHz16BitStereo :
            case SPSF_48kHz16BitStereo :
              return true;
          }
      }
      return false;
    }


    unsigned GetChannels() const
    {
      switch (m_spStreamFormat) {
        case SPSF_8kHz16BitMono :
        case SPSF_11kHz16BitMono :
        case SPSF_12kHz16BitMono :
        case SPSF_16kHz16BitMono :
        case SPSF_22kHz16BitMono :
        case SPSF_24kHz16BitMono :
        case SPSF_32kHz16BitMono :
        case SPSF_44kHz16BitMono :
        case SPSF_48kHz16BitMono :
          return 1;
        case SPSF_8kHz16BitStereo :
        case SPSF_11kHz16BitStereo :
        case SPSF_12kHz16BitStereo :
        case SPSF_16kHz16BitStereo :
        case SPSF_22kHz16BitStereo :
        case SPSF_24kHz16BitStereo :
        case SPSF_32kHz16BitStereo :
        case SPSF_44kHz16BitStereo :
        case SPSF_48kHz16BitStereo :
          return 2;
      }
      return 0;
    }
};


////////////////////////////////////////////////////////////
// Text to speech using Microsoft's Speech API (SAPI)

class PTextToSpeech_SAPI : public PTextToSpeech, protected PSpStreamFormat
{
    PCLASSINFO(PTextToSpeech_SAPI, PTextToSpeech);
  protected:
    bool               m_opened;
    CComPtr<ISpVoice>  m_cpVoice;
    CComPtr<ISpStream> m_cpFileStream;
    PString            m_currentVoice;
  public:
    PTextToSpeech_SAPI()
      : m_opened(false)
    {
      PTRACE(4, "Constructed " << this);
    }


    ~PTextToSpeech_SAPI()
    {
      Close();
      PTRACE(4, "Destroyed " << this);
    }


    virtual PStringArray GetVoiceList()
    {
      //Enumerate the available voices 
      PStringArray voiceList;

      CComPtr<IEnumSpObjectTokens> cpEnum;
      ULONG ulCount = 0;
      PComResult hr;

      // Get the number of voices
      if (PCOM_SUCCEEDED_EX(hr,SpEnumTokens,(SPCAT_VOICES, NULL, NULL, &cpEnum))) {
        if (PCOM_SUCCEEDED_EX(hr,cpEnum->GetCount,(&ulCount))) {
          // Obtain a list of available voice tokens, set the voice to the token, and call Speak
          while (ulCount-- > 0) {
            CComPtr<ISpObjectToken> cpVoiceToken;
            if (hr.Succeeded(cpEnum->Next(1, &cpVoiceToken, NULL))) {
              PWSTR pDescription = NULL;
              SpGetDescription(cpVoiceToken, &pDescription);
              PString desc(pDescription);
              CoTaskMemFree(pDescription);
              if (desc.NumCompare("Microsoft ") == EqualTo) {
                PINDEX dash = desc.Find(" - ");
                if (dash != P_MAX_INDEX)
                  desc = PSTRSTRM(desc(10, dash-1) << ':' << desc.Mid(dash+3));
              }
              voiceList.AppendString(desc);
            }
          } 
        }
      }

      PTRACE(4, "Voices: " << setfill(',') << voiceList);
      return voiceList;
    }


    virtual bool InternalSetVoice(PStringOptions & voice)
    {
      if (!IsOpen())
        return true;

      PString name = voice.Get(VoiceName);
      PTRACE(4, "Trying to set voice \"" << name << '"');

      //Enumerate voice tokens with attribute "Name=<specified voice>"
      CComPtr<IEnumSpObjectTokens> cpEnum;
      if (PCOM_FAILED(SpEnumTokens, (SPCAT_VOICES, name.AsWide(), NULL, &cpEnum)))
        return false;

      //Get the closest token
      CComPtr<ISpObjectToken> cpVoiceToken;
      if (PCOM_FAILED(cpEnum->Next, (1, &cpVoiceToken, NULL)))
        return false;

      //set the voice
      if (PCOM_FAILED(m_cpVoice->SetVoice, (cpVoiceToken)))
        return false;

      PTRACE(4, "SetVoice(" << name << ") OK!");
      return true;
    }


    virtual bool SetSampleRate(unsigned rate)
    {
      return PSpStreamFormat::SetSampleRate(rate);
    }


    virtual unsigned GetSampleRate() const
    {
      return PSpStreamFormat::GetSampleRate();
    }


    virtual bool SetChannels(unsigned channels)
    {
      return PSpStreamFormat::SetChannels(channels);
    }


    virtual unsigned GetChannels() const
    {
      return PSpStreamFormat::GetChannels();
    }


    bool SetVolume(unsigned)
    {
      return false;
    }


    unsigned GetVolume() const
    {
      return 100;
    }


    bool OpenFile(const PFilePath & fn)
    {
      Close();

      if (PCOM_FAILED(m_cpVoice.CoCreateInstance,(CLSID_SpVoice)))
        return false;

      CSpStreamFormat sInputFormat;
      sInputFormat.AssignFormat(m_spStreamFormat);

      if (PCOM_FAILED(SPBindToFile,(fn.c_str(), SPFM_CREATE_ALWAYS, &m_cpFileStream, &sInputFormat.FormatId(), sInputFormat.WaveFormatExPtr())))
        return false;

      m_opened = PCOM_SUCCEEDED(m_cpVoice->SetOutput,(m_cpFileStream, true));
      return m_opened;
    }


    bool IsOpen() const
    {
      return m_opened;
    }


    bool Close()
    {
      if (!m_opened)
        return false;

      m_cpVoice->WaitUntilDone(INFINITE);
      m_cpFileStream.Release();
      m_cpVoice.Release();

      m_opened = false;
      return true;
    }


    bool Speak(const PString & text, TextType hint)
    {
      if (!IsOpen())
        return false;

      PWideString wtext = text;

      // do various things to the string, depending upon the hint
      switch (hint) {
        case Digits:
          break;
        default:
          break;
      };

      if (m_currentVoice != NULL && !m_currentVoice.IsEmpty()) {
        PTRACE(4, "Trying to set voice \"" << m_currentVoice << "\""
                  " of voices: " << setfill(',') << GetVoiceList());

        //Enumerate voice tokens with attribute "Name=<specified voice>"
        CComPtr<IEnumSpObjectTokens> cpEnum;
        if (PCOM_SUCCEEDED(SpEnumTokens,(SPCAT_VOICES, m_currentVoice.AsWide(), NULL, &cpEnum))) {
          //Get the closest token
          CComPtr<ISpObjectToken> cpVoiceToken;
          if (PCOM_SUCCEEDED(cpEnum->Next,(1, &cpVoiceToken, NULL))) {
            //set the voice
            if (PCOM_SUCCEEDED(m_cpVoice->SetVoice,(cpVoiceToken))) {
              PTRACE(4, "SetVoice(" << m_currentVoice << ") OK!");
            }
          }
        } 
      }

      PTRACE(4, "Speaking...");
      return PCOM_SUCCEEDED(m_cpVoice->Speak,(wtext, SPF_DEFAULT, NULL));
    }
};

PFACTORY_CREATE(PFactory<PTextToSpeech>, PTextToSpeech_SAPI, P_TEXT_TO_SPEECH_SAPI, false);


////////////////////////////////////////////////////////////
// Speech recognition using Microsoft's Speech API (SAPI)

class PSpeechRecognition_SAPI : public PSpeechRecognition, protected PSpStreamFormat
{
    PCLASSINFO(PSpeechRecognition_SAPI, PSpeechRecognition);
  protected:
    bool                    m_opened;
    CComPtr<ISpeechMemoryStream> m_cpMemoryStream;
    CComPtr<ISpStream>      m_cpStream;
    CComPtr<ISpRecognizer>  m_cpRecognizer;
    CComPtr<ISpRecoContext> m_cpRecoContext;
    CComPtr<ISpRecoGrammar> m_cpRecoGrammar;
    PString                 m_language;
    PString                 m_vocabulary;

  public:
    PSpeechRecognition_SAPI()
      : m_opened(false)
    {
      PTRACE(4, "Constructed " << this);
    }


    ~PSpeechRecognition_SAPI()
    {
      Close();
    }

    virtual bool SetSampleRate(unsigned rate)
    {
      return PSpStreamFormat::SetSampleRate(rate);
    }


    virtual unsigned GetSampleRate() const
    {
      return PSpStreamFormat::GetSampleRate();
    }


    virtual bool SetChannels(unsigned channels)
    {
      return PSpStreamFormat::SetChannels(channels);
    }


    virtual unsigned GetChannels() const
    {
      return PSpStreamFormat::GetChannels();
    }


    virtual bool SetLanguage(const PString & language)
    {
      // TODO: Check if valid language string
      m_language = language;
      return true;
    }


    virtual PString GetLanguage() const
    {
      return m_language;
    }

    virtual bool CreateVocabulary(const PString & /*name*/, const PStringArray & /*words*/)
    {
        /*
        WORD langId = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
        m_cpRecoGrammar->ResetGrammar(langId);

        // Create rules
        hr = recoGrammar->GetRule(ruleName1, 0, SPRAF_TopLevel | SPRAF_Active, true, &sate);
        check_result(hr);

        // Add a word
        const std::wstring commandWstr = std::wstring(command.begin(), command.end());
        hr = recoGrammar->AddWordTransition(sate, NULL, commandWstr.c_str(), L" ", SPWT_LEXICAL, 1, nullptr);
        check_result(hr);

        // Commit changes
        hr = recoGrammar->Commit(0);
        check_result(hr);

        ULONGLONG interest;
        interest = SPFEI(SPEI_RECOGNITION);
        hr = recoContext->SetInterest(interest, interest);
        check_result(hr);

        // Activate Grammar
        hr = recoGrammar->SetRuleState(ruleName1, 0, SPRS_ACTIVE);
        check_result(hr);
        */
    }

    virtual bool Open(const Notifier & notifier)
    {
      m_notifier = notifier;

      PCOM_RETURN_ON_FAILED(m_cpRecognizer.CoCreateInstance,(CLSID_SpInprocRecognizer));

      PCOM_RETURN_ON_FAILED(m_cpRecognizer->CreateRecoContext, (&m_cpRecoContext));
      PCOM_RETURN_ON_FAILED(m_cpRecoContext->SetNotifyWin32Event, ());
      PCOM_RETURN_ON_FAILED(m_cpRecoContext->CreateGrammar, (0, &m_cpRecoGrammar));

      if (m_vocabulary.empty())
        PCOM_RETURN_ON_FAILED(m_cpRecoGrammar->LoadDictation, (m_vocabulary.AsWide(), SPLO_STATIC));

      PCOM_RETURN_ON_FAILED(m_cpRecoContext->SetInterest,(SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
                                                          SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM)));
      PCOM_RETURN_ON_FAILED(m_cpRecoGrammar->SetDictationState,(SPRS_ACTIVE));

      PTRACE(3, "Opened speech recognition.");
      m_opened = true;
      return true;
    }


    virtual bool IsOpen() const
    {
      return m_opened;
    }


    virtual bool Close()
    {
      if (!m_opened)
        return false;

      if (m_cpRecoGrammar) {
        PCOM_SUCCEEDED(m_cpRecoGrammar->SetDictationState,(SPRS_INACTIVE));
        PCOM_SUCCEEDED(m_cpRecoGrammar->UnloadDictation,());
        m_cpRecoGrammar.Release();
      }

      if (m_cpMemoryStream) {
        //PCOM_SUCCEEDED(m_cpMemoryStream->Close,());
        m_cpMemoryStream.Release();
      }

      PCOM_SUCCEEDED(m_cpStream->Close,());
      m_cpStream.Release();

      PCOM_SUCCEEDED(m_cpRecoContext->SetContextState,(SPCS_DISABLED));
      m_cpRecoContext.Release();
      m_cpRecognizer.Release();

      m_opened = false;
      return true;
    }


    enum {
      ProcessEventFailed,
      ProcessEventTimeout,
      ProcessEventFinished,
      ProcessEventContinue
    } ProcessEvents(DWORD timeout)
    {
      PComResult hr;
      if (PCOM_FAILED_EX(hr, m_cpRecoContext->WaitForNotifyEvent,(timeout)))
          return ProcessEventFailed;

      if (hr.GetErrorNumber() == S_FALSE)
          return ProcessEventTimeout;

      CSpEvent spEvent;
      while (spEvent.GetFrom(m_cpRecoContext) == S_OK) {
        switch (spEvent.eEventId) {
          case SPEI_END_SR_STREAM :
            return ProcessEventFinished;

          case SPEI_RECOGNITION :
            ISpRecoResult * pResult = spEvent.RecoResult();
            SPPHRASE * pPhrase;
            if (PCOM_SUCCEEDED(pResult->GetPhrase, (&pPhrase))) {
              WCHAR * pwszText;
              pResult->GetText((ULONG)SP_GETWHOLEPHRASE, (ULONG)SP_GETWHOLEPHRASE, FALSE, &pwszText, NULL);
              if (!m_notifier.IsNULL())
                m_notifier(*this, Transcript(true, PTimer::Tick(), pwszText, 1));
              CoTaskMemFree(pwszText);
            }
        }
        spEvent.Clear();
      }

      return ProcessEventContinue;
    }


    virtual bool Listen(const PFilePath & fn)
    {
      if (m_cpMemoryStream) {
        PAssertAlways("Listening to stream!");
        return false;
      }

      CSpStreamFormat sInputFormat;
      sInputFormat.AssignFormat(m_spStreamFormat);
      PCOM_RETURN_ON_FAILED(SPBindToFile,(fn.c_str(), SPFM_OPEN_READONLY, &m_cpStream, &sInputFormat.FormatId(), sInputFormat.WaveFormatExPtr()));
      PCOM_RETURN_ON_FAILED(m_cpRecognizer->SetInput,(m_cpStream, TRUE));

      for (;;) {
        switch (ProcessEvents(10000)) {
          case ProcessEventFailed :
          case ProcessEventTimeout :
            return false;
          case ProcessEventFinished :
            return true;
          case ProcessEventContinue :
            break;
        }
      }
    }


    virtual bool Listen(const short * samples, size_t sampleCount)
    {
      if (m_cpStream) {
        PAssertAlways("Listening to file!");
        return false;
      }

      if (!m_cpMemoryStream) {
        PCOM_RETURN_ON_FAILED(m_cpMemoryStream.CoCreateInstance, (CLSID_SpMemoryStream));
        CSpStreamFormat sInputFormat;
        sInputFormat.AssignFormat(m_spStreamFormat);
        PCOM_RETURN_ON_FAILED(m_cpRecognizer->SetInput, (m_cpMemoryStream, TRUE));
      }

      long written = 0;
      VARIANT data;
      PCOM_RETURN_ON_FAILED(InitVariantFromInt16Array,(samples, sampleCount, &data));
      PCOM_RETURN_ON_FAILED(m_cpMemoryStream->Write, (data, &written));

      return ProcessEvents(0) != ProcessEventFailed;
    }
};

#ifdef P_SPEECH_RECOGNITION_SAPI
  PFACTORY_CREATE(PFactory<PSpeechRecognition>, PSpeechRecognition_SAPI, P_SPEECH_RECOGNITION_SAPI, false);
#endif


#endif // P_SAPI
