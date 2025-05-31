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

#include <ptclib/speech.h>


#define PTraceModule() "SPEECH"


#if P_TEXT_TO_SPEECH

#include <ptlib/sound.h>
#include <ptlib/ipsock.h>
#include <ptclib/pwavfile.h>
#include <ptclib/url.h>


const PCaselessString & PTextToSpeech::VoiceName() { static PConstCaselessString s("name"); return s; }
const PCaselessString & PTextToSpeech::VoiceLanguage() { static PConstCaselessString s("language"); return s; }
const PCaselessString & PTextToSpeech::VoiceGender() { static PConstCaselessString s("gender"); return s; }


PTextToSpeech * PTextToSpeech::Create(const PString & name)
{
  if (!name.empty())
    return PFactory<PTextToSpeech>::CreateInstance(name);

  PTextToSpeech * tts = NULL;
#ifdef P_TEXT_TO_SPEECH_AWS
  if ((tts = PFactory<PTextToSpeech>::CreateInstance(P_TEXT_TO_SPEECH_AWS)) == NULL)
#endif
#ifdef P_TEXT_TO_SPEECH_FESTIVAL
    if ((tts = PFactory<PTextToSpeech>::CreateInstance(P_TEXT_TO_SPEECH_FESTIVAL)) == NULL)
#endif
#ifdef P_TEXT_TO_SPEECH_SAPI
      if ((tts = PFactory<PTextToSpeech>::CreateInstance(P_TEXT_TO_SPEECH_SAPI)) == NULL)
#endif
      {
        vector<string> keys = PFactory<PTextToSpeech>::GetKeyList();
        if (!keys.empty())
          tts = PFactory<PTextToSpeech>::CreateInstance(keys.front());
      }
  if (tts != NULL)
    PTRACE(4, tts, "Created " << *tts);
  else
    PTRACE(2, tts, "Could not create default Text To Speech engine");
  return tts;
}


const PString & PTextToSpeech::VoiceKey() { static const PConstString s("TTS-Voice"); return s; }
const PString & PTextToSpeech::SampleRateKey() { static const PConstString s("TTS-Sample-Rate"); return s; }
const PString & PTextToSpeech::ChannelsKey() { static const PConstString s("TTS-Channels"); return s; }
const PString & PTextToSpeech::VolumeKey() { static const PConstString s("TTS-Volume"); return s; }

bool PTextToSpeech::SetOptions(const PStringOptions & options)
{
  if (options.Has(VoiceKey())) {
    if (!SetVoice(options.GetString(VoiceKey())))
      return false;
  }

  if (options.Has(SampleRateKey())) {
    if (!SetSampleRate(options.GetInteger(SampleRateKey())))
      return false;
  }

  if (options.Has(ChannelsKey())) {
    if (!SetChannels(options.GetInteger(ChannelsKey())))
      return false;
  }

  if (options.Has(VolumeKey())) {
    if (!SetVolume(options.GetInteger(VolumeKey())))
      return false;
  }

  PTRACE(4, "SetOptions succeeded");
  return true;
}


bool PTextToSpeech::SetVoice(const PString & voiceStr)
{
  PStringOptions voice;
  if (voiceStr.empty() || voiceStr == ":" || voiceStr == "*") {
    if (m_voice.empty() || !voiceStr.empty()) {
      PStringArray voices = GetVoiceList();
      if (!voices.empty())
        PURL::SplitVars(voices[0], m_voice);
    }
    voice = m_voice;
  }
  else if (voiceStr.find('=') != string::npos)
    PURL::SplitVars(voiceStr, voice);
  else {
    PString name, language;
    voiceStr.Split(':', name, language, PString::SplitDefaultToBefore|PString::SplitTrim);
    if (!name.empty())
      voice.Set(VoiceName, name);
    if (!language.empty())
      voice.Set(VoiceLanguage, language);
  }

  if (!InternalSetVoice(voice))
    return false;

  m_voice = voice;
  return true;
}


PString PTextToSpeech::GetVoice() const
{
  if (m_voice.size() == 1)
    return m_voice.Get(VoiceName);
  return PSTRSTRM(m_voice);
}


bool PTextToSpeech::OpenChannel(PChannel * /*channel*/)
{
  Close();
  return false;
}


PChannel * PTextToSpeech::SpeakToChannel(const PString &, TextType)
{
  return NULL;
}


//////////////////////////////////////////////////////////////////////////////////

class PTextToSpeech_WAV : public PTextToSpeech
{
protected:
  PDECLARE_MUTEX(mutex);
  bool      m_opened;
  bool      m_usingFile;
  PString   m_text;
  PFilePath m_path;
  unsigned  m_sampleRate;
  unsigned  m_channels;
  unsigned  m_volume;

  std::vector<PFilePath> m_filenames;

public:
  PTextToSpeech_WAV()
    : m_opened(false)
    , m_usingFile(false)
    , m_sampleRate(8000)
    , m_channels(1)
    , m_volume(100)
  { }
  PStringArray GetVoiceList() { return PStringArray(); }
  PBoolean SetSampleRate(unsigned rate) { m_sampleRate = rate; return true; }
  unsigned GetSampleRate() const { return m_sampleRate; }
  PBoolean SetChannels(unsigned channels) { m_channels = channels; return true; }
  unsigned GetChannels() const { return m_channels; }
  PBoolean SetVolume(unsigned volume) { m_volume = volume; return true; }
  unsigned GetVolume() const { return m_volume; }
  PBoolean OpenFile(const PFilePath & fn)
  {
    PWaitAndSignal m(mutex);

    Close();
    m_usingFile = true;
    m_path = fn;
    m_opened = true;

    PTRACE(3, "Writing speech to " << fn);

    return true;
  }
  PBoolean IsOpen() const { return m_opened; }
  PBoolean Close()
  {
    PWaitAndSignal m(mutex);

    if (!m_opened)
      return true;

    PBoolean stat = true;

    #if P_WAVFILE
    if (m_usingFile) {
      PWAVFile outputFile(PSOUND_PCM16, m_path, PFile::WriteOnly);
      if (!outputFile.IsOpen()) {
        PTRACE(1, "Cannot create output file " << m_path);
        stat = false;
      }
      else {
        std::vector<PFilePath>::const_iterator r;
        for (r = m_filenames.begin(); r != m_filenames.end(); ++r) {
          PFilePath f = *r;
          PWAVFile file;
          file.SetAutoconvert();
          if (!file.Open(f, PFile::ReadOnly)) {
            PTRACE(1, "Cannot open input file " << f);
            stat = false;
          }
          else {
            PTRACE(1, "Reading from " << f);
            BYTE buffer[1024];
            for (;;) {
              if (!file.Read(buffer, 1024))
                break;
              outputFile.Write(buffer, file.GetLastReadCount());
            }
          }
        }
      }
      m_filenames.erase(m_filenames.begin(), m_filenames.end());
    }
    #endif // P_WAVFILE

    m_opened = false;
    return stat;
  }
  PBoolean SpeakNumber(unsigned number)
  {
    return Speak(PString(PString::Signed, number), Number);
  }
  PBoolean SpeakFile(const PString & text)
  {
    PFilePath f = PDirectory(m_voice.Get(VoiceName)) + (text.ToLower() + ".wav");
    if (!PFile::Exists(f)) {
      PTRACE(2, "Unable to find explicit file for " << text);
      return false;
    }
    m_filenames.push_back(f);
    return true;
  }
  PBoolean Speak(const PString & text, TextType hint = Default)
  {
    // break into lines
    PStringArray lines = text.Lines();
    PINDEX i;
    for (i = 0; i < lines.GetSize(); ++i) {

      PString line = lines[i].Trim();
      if (line.IsEmpty())
        continue;

      PTRACE(3, "Asked to speak " << text << " with type " << hint);

      if (hint == DateAndTime) {
        PTRACE(3, "Speaking date and time");
        Speak(text, Date);
        Speak(text, Time);
        continue;
      }

      if (hint == Date) {
        PTime time(line);
        if (time.IsValid()) {
          PTRACE(4, "Speaking date " << time);
          SpeakFile(time.GetDayName(time.GetDayOfWeek(), PTime::FullName));
          SpeakNumber(time.GetDay());
          SpeakFile(time.GetMonthName(time.GetMonth(), PTime::FullName));
          SpeakNumber(time.GetYear());
        }
        continue;
      }

      if (hint == Time) {
        PTime time(line);
        if (time.IsValid()) {
          PTRACE(4, "Speaking time " << time);
          int hour = time.GetHour();
          if (hour < 13) {
            SpeakNumber(hour);
            SpeakNumber(time.GetMinute());
            SpeakFile(PTime::GetTimeAM());
          }
          else {
            SpeakNumber(hour-12);
            SpeakNumber(time.GetMinute());
            SpeakFile(PTime::GetTimePM());
          }
        }
        continue;
      }

      if (hint == Default) {
        PBoolean isTime = false;
        PBoolean isDate = false;

        for (i = 0; !isDate && i < 7; ++i) {
          isDate = isDate || (line.Find(PTime::GetDayName((PTime::Weekdays)i, PTime::FullName)) != P_MAX_INDEX);
          isDate = isDate || (line.Find(PTime::GetDayName((PTime::Weekdays)i, PTime::Abbreviated)) != P_MAX_INDEX);
          PTRACE(4, " " << isDate << " - " << PTime::GetDayName((PTime::Weekdays)i, PTime::FullName) << "," << PTime::GetDayName((PTime::Weekdays)i, PTime::Abbreviated));
        }
        for (i = 1; !isDate && i <= 12; ++i) {
          isDate = isDate || (line.Find(PTime::GetMonthName((PTime::Months)i, PTime::FullName)) != P_MAX_INDEX);
          isDate = isDate || (line.Find(PTime::GetMonthName((PTime::Months)i, PTime::Abbreviated)) != P_MAX_INDEX);
          PTRACE(4, " " << isDate << " - " << PTime::GetMonthName((PTime::Months)i, PTime::FullName) << "," << PTime::GetMonthName((PTime::Months)i, PTime::Abbreviated));
        }

        if (!isTime)
          isTime = line.Find(PTime::GetTimeSeparator()) != P_MAX_INDEX;
        if (!isDate)
          isDate = line.Find(PTime::GetDateSeparator()) != P_MAX_INDEX;

        if (isDate && isTime) {
          PTRACE(4, "Default changed to DateAndTime");
          Speak(line, DateAndTime);
          continue;
        }
        if (isDate) {
          PTRACE(4, "Default changed to Date");
          Speak(line, Date);
          continue;
        }
        else if (isTime) {
          PTRACE(4, "Default changed to Time");
          Speak(line, Time);
          continue;
        }
      }

      PStringArray tokens = line.Tokenise("\t ", false);
      for (PINDEX j = 0; j < tokens.GetSize(); ++j) {
        PString word = tokens[j].Trim();
        if (word.IsEmpty())
          continue;
        PTRACE(4, "Speaking word " << word << " as " << hint);
        switch (hint) {

          case Time:
          case Date:
          case DateAndTime:
            PAssertAlways("Logic error");
            break;

          default:
          case Default:
          case Literal:
          {
            PBoolean isDigits = true;
            PBoolean isIpAddress = true;

            PINDEX k;
            for (k = 0; k < word.GetLength(); ++k) {
              if (word[k] == '.')
                isDigits = false;
              else if (!isdigit(word[k]))
                isDigits = isIpAddress = false;
            }

            if (isIpAddress) {
              PTRACE(4, "Default changed to IPAddress");
              Speak(word, IPAddress);
            }
            else if (isDigits) {
              PTRACE(4, "Default changed to Number");
              Speak(word, Number);
            }
            else {
              PTRACE(4, "Default changed to Spell");
              Speak(word, Spell);
            }
          }
          break;

          case Spell:
            PTRACE(4, "Spelling " << text);
            for (PINDEX k = 0; k < text.GetLength(); ++k)
              SpeakFile(text[k]);
            break;

          case Phone:
          case Digits:
            PTRACE(4, "Speaking digits " << text);
            for (PINDEX k = 0; k < text.GetLength(); ++k) {
              if (isdigit(text[k]))
                SpeakFile(text[k]);
            }
            break;

          case Duration:
          case Currency:
          case Number:
          {
            int number = atoi(line);
            PTRACE(4, "Speaking number " << number);
            if (number < 0) {
              SpeakFile("negative");
              number = -number;
            }
            else if (number == 0) {
              SpeakFile("0");
            }
            else {
              if (number >= 1000000) {
                int millions = number / 1000000;
                number = number % 1000000;
                SpeakNumber(millions);
                SpeakFile("million");
              }
              if (number >= 1000) {
                int thousands = number / 1000;
                number = number % 1000;
                SpeakNumber(thousands);
                SpeakFile("thousand");
              }
              if (number >= 100) {
                int hundreds = number / 100;
                number = number % 100;
                SpeakNumber(hundreds);
                SpeakFile("hundred");
              }
              if (!SpeakFile(PString(PString::Signed, number))) {
                int tens = number / 10;
                number = number % 10;
                if (tens > 0)
                  SpeakFile(PString(PString::Signed, tens*10));
                if (number > 0)
                  SpeakFile(PString(PString::Signed, number));
              }
            }
          }
          break;

          case IPAddress:
          {
            PIPSocket::Address addr(line);
            PTRACE(4, "Speaking IP address " << addr);
            for (PINDEX k = 0; k < 4; ++k) {
              int octet = addr[k];
              if (octet < 100)
                SpeakNumber(octet);
              else
                Speak(octet, Digits);
              if (k != 3)
                SpeakFile("dot");
            }
          }
          break;
        }
      }
    }

    return true;
  }
};

PFACTORY_CREATE(PFactory<PTextToSpeech>, PTextToSpeech_WAV, "WAV", false);

#endif // P_TEXT_TO_SPEECH


#if P_SPEECH_RECOGNITION

PSpeechRecognition * PSpeechRecognition::Create(const PString & name)
{
  if (!name.empty())
    return PFactory<PSpeechRecognition>::CreateInstance(name);

  PSpeechRecognition * sr = NULL;
#ifdef P_SPEECH_RECOGNITION_AWS
  if ((sr = PFactory<PSpeechRecognition>::CreateInstance(P_SPEECH_RECOGNITION_AWS)) == NULL)
#endif
#ifdef P_SPEECH_RECOGNITION_SAPI
    if ((sr = PFactory<PSpeechRecognition>::CreateInstance(P_SPEECH_RECOGNITION_SAPI)) == NULL)
#endif
    {
      vector<string> keys = PFactory<PSpeechRecognition>::GetKeyList();
      if (!keys.empty())
        sr = PFactory<PSpeechRecognition>::CreateInstance(keys.front());
    }
  if (sr != NULL)
    PTRACE(4, sr, "Created " << *sr);
  else
    PTRACE(2, sr, "Could not create default Speech Recognition engine");
  return sr;
}


const PString & PSpeechRecognition::SampleRateKey() { static const PConstString s("SR-Sample-Rate"); return s; }
const PString & PSpeechRecognition::ChannelsKey  () { static const PConstString s("SR-Channels"); return s; }
const PString & PSpeechRecognition::LanguageKey  () { static const PConstString s("SR-Language"); return s; }

bool PSpeechRecognition::SetOptions(const PStringOptions & options)
{
  if (options.Has(SampleRateKey())) {
    if (!SetSampleRate(options.GetInteger(SampleRateKey())))
      return false;
  }

  if (options.Has(ChannelsKey())) {
    if (!SetChannels(options.GetInteger(ChannelsKey())))
      return false;
  }

  if (options.Has(LanguageKey())) {
    if (!SetLanguage(options.GetString(LanguageKey())))
      return false;
  }

  PTRACE(4, "SetOptions succeeded");
  return true;
}


bool PSpeechRecognition::CreateVocabulary(const PString &, const PStringArray &)
{
  return false;
}


bool PSpeechRecognition::DeleteVocabulary(const PString &, const PStringArray &)
{
  return false;
}


bool PSpeechRecognition::ActivateVocabulary(const PStringSet &, const PStringSet &)
{
  return false;
}


PSpeechRecognition::Transcript::Transcript(bool final, const PTimeInterval & when, const PString & content, float confidence)
  : m_final(final)
  , m_when(when)
  , m_content(content)
  , m_confidence(confidence > 0 ? confidence : 1.0f)
{
}


PObject::Comparison PSpeechRecognition::Transcript::Compare(const PObject & obj) const
{
  const Transcript & other = dynamic_cast<const Transcript &>(obj);
  Comparison c = Compare2(m_final, other.m_final);
  if (c == EqualTo)
    c = m_when.Compare(other.m_when);
  if (c == EqualTo)
    c = m_content.Compare(other.m_content);
  return c;
}


void PSpeechRecognition::Transcript::PrintOn(ostream & strm) const
{
  strm << setprecision(2) << m_when << ' ';
  if (m_final)
    strm << "(final) ";
  strm << m_content.ToLiteral();
}

#endif // P_SPEECH_RECOGNITION

