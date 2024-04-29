/*
 * vxml.h
 *
 * VXML engine for pwlib library
 *
 * Copyright (C) 2002 Equivalence Pty. Ltd.
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

#ifndef PTLIB_VXML_H
#define PTLIB_VXML_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptclib/pxml.h>

#if P_VXML

#include <ptlib/pfactory.h>
#include <ptlib/pipechan.h>
#include <ptlib/sound.h>
#include <ptlib/videoio.h>
#include <ptclib/delaychan.h>
#include <ptclib/pwavfile.h>
#include <ptclib/speech.h>
#include <ptclib/url.h>
#include <ptclib/script.h>

#include <queue>


class PVXMLSession;
class PVXMLDialog;


// these are the same strings as the Opal equivalents, but as this is PWLib, we can't use Opal contants
#define VXML_PCM16         PSOUND_PCM16
#define VXML_G7231         "G.723.1"
#define VXML_G729          "G.729"


/*! \page pageVXML Voice XML support

\section secOverview Overview 
The VXML implementation here is highly piecemeal. It is not even close to
complete and while roughly following the VXML 2.0 standard there are many
incompatibilities. Below is a list of what is available.
\section secSupported Elements & Attributes Supported
<TABLE>
<TR><TH>Supported tag   <TH>Supported attributes<TH>Notes
<TR><TD>&lt;menu&gt;    <TD>dtmf
                        <TD>property.timeout is supported
<TR><TD>&lt;choice&gt;  <TD>dtmf, next, expr, event
<TR><TD>&lt;form&gt;    <TD>id
                        <TD>property.timeout is supported
<TR><TD>&lt;field&gt;   <TD>name, cond
<TR><TD>&lt;prompt&gt;  <TD>bargein, cond
                        <TD>property.bargein is supported
<TR><TD>&lt;grammar&gt; <TD>mode, type
                        <TD>Only "dtmf" is supported for "mode"<BR>
                            Only "X-OPAL/digits" is supported for "type"<BR>
                            The "X-OPAL/digits" grammar consists of three parameters
                            of the form: minDigits=1; maxDigits=5; terminators=#
<TR><TD>&lt;filled&gt;  <TD>
<TR><TD>&lt;noinput&gt; <TD>count, cond
<TR><TD>&lt;nomatch&gt; <TD>count, cond
<TR><TD>&lt;error&gt;   <TD>count, cond
<TR><TD>&lt;catch&gt;   <TD>count, event
                        <TD>Note there is a limited set of events that are thrown
<TR><TD>&lt;goto&gt;    <TD>nextitem, expritem, expr
<TR><TD>&lt;exit&gt;    <TD>No attributes supported
<TR><TD>&lt;submit&gt;  <TD>next, expr, enctype, method, fetchtimeout, namelist
<TR><TD>&lt;disconnect&gt;<TD>No attributes needed
<TR><TD>&lt;block&gt;   <TD>cond
<TR><TD>&lt;audio&gt;   <TD>src, expr
<TR><TD>&lt;break&gt;   <TD>msecs, time, size
<TR><TD>&lt;value&gt;   <TD>expr, class, voice
                        <TD>class & voice are VXMKL 1.0 attributes and will be
                            removed when &lt;say-as;&gt; is implemented.
<TR><TD>&lt;script&gt;  <TD>src
<TR><TD>&lt;var&gt;     <TD>name, expr
<TR><TD>&lt;property&gt;<TD>name", value
<TR><TD>&lt;if&gt;      <TD>cond
<TR><TD>&lt;elseif&gt;  <TD>cond
<TR><TD>&lt;else&gt;    <TD>No attributes needed
<TR><TD>&lt;transfer&gt;<TD>name, dest, destexpr, bridge, cond
<TR><TD>&lt;record&gt;  <TD>name, type, beep, dtmfterm, maxtime, finalsilence, cond
</TABLE>
\section secScripting Scripting support
Depending on how the system was built there are three possibilities:
<TABLE>
<TR><TD>Ultra-primitive<TD>Here, all variables are string types, the only expressions
    possible are string concatenation using '+' operator. Literals using single
    quotes are allowed. For &lt;if&gt; only the == and != operator may be used.
<TR><TD>Lua<TD>While a full scripting language, this is not ECMA
    compatible, so is not compliant to standards. However it can be on option if
    compiling V* proves too dificult.
<TR><TD>Google V8<TD>THis is the most compliant to ECMA, and the default scripting
    language, if available.
</TABLE>

Default variables:
<TABLE>
<TR><TD colspan=2>On VXML load
<TR><TD>document.uri                    <TD>URI for currently loaded VXML document
<TR><TD>document.path                   <TD>Root of URI
<TR><TD colspan=2>Recoding element
<TR><TD>{name}$.type                    <TD>MIME type for recording output
<TR><TD>{name}$.uri                     <TD>URI for recording output
<TR><TD>{name}$.maxtime                 <TD>Boolean flag for recording hit maximum time
<TR><TD>{name}$.termchar                <TD>Value used if recording ended by termination character
<TR><TD>{name}$.duration                <TD>Duration of the recording
<TR><TD>{name}$.size                    <TD>Size in bytes of the recording
<TR><TD colspan=2>Transfer element
<TR><TD>{name}$.duration                <TD>Duration of transfer operation
<TR><TD colspan=2>When executed from OPAL call
<TR><TD>session.time                    <TD>Wall clock time of call start
<TR><TD>session.connection.local.uri    <TD>Local party URI
<TR><TD>session.connection.local.dnis   <TD>Called party number
<TR><TD>session.connection.remote.ani   <TD>Calling party number
<TR><TD>session.connection.remote.uri   <TD>Remote party URI
<TR><TD>session.connection.remote.ip    <TD>Remote party media IP address
<TR><TD>session.connection.remote.port  <TD>REmote party media port numner
<TR><TD>session.connection.calltoken    <TD>Call token (OPAL internal)
</TABLE>
*/

//////////////////////////////////////////////////////////////////

struct PVXMLGrammarInit
{
  PString        m_type;
  PVXMLSession & m_session;
  PXMLElement  & m_field;
  PXMLElement  * m_grammarElement;
  PStringOptions m_parameters;

  PVXMLGrammarInit(const PString & builtinType, PVXMLSession & session, PXMLElement & field, PXMLElement  * grammar = NULL)
    : m_session(session)
    , m_field(field)
    , m_grammarElement(grammar)
  {
    PINDEX pos = builtinType.Find('?');
    if (pos == P_MAX_INDEX)
      m_type = builtinType;
    else {
      m_type = builtinType.Left(pos);
      PURL::SplitVars(builtinType.Mid(pos+1), m_parameters);
    }
  }
};


class PVXMLGrammar : public PObject, protected PVXMLGrammarInit
{
    PCLASSINFO(PVXMLGrammar, PObject);
  public:
    PVXMLGrammar(const PVXMLGrammarInit & init);

    virtual void OnUserInput(const PString & input);
    virtual void OnAudioInput(const short * samples, size_t count);

    P_DECLARE_TRACED_ENUM(GrammarState,
      Idle,         ///< Not yet started
      Started,      ///< Grammar awaiting input
      PartFill,     ///< If times out goes to Filled rather than NoInput
      Filled,       ///< got something that matched the grammar
      NoInput,      ///< timeout or still waiting to match
      NoMatch,      ///< recognized something but didn't match the grammar
      Help,         ///< help keyword
      Illegal,      ///< Illegal, could not be initialised
      BadFetch      ///< Could not get dependent resource
    );

    GrammarState GetState() const { return m_state; }

    virtual bool Start();
    virtual void Stop();
    virtual bool Process();

    void SetTimeout(const PTimeInterval & timeout);

  protected:
    PDECLARE_NOTIFIER(PTimer, PVXMLGrammar, OnTimeout);
    PDECLARE_SpeechRecognitionNotifier(PVXMLGrammar, OnRecognition);
    virtual void GetWordsToRecognise(PStringArray & words) const;
    virtual void OnInput(const PString & input) = 0;
    virtual void SetPartFilled(const PString & input, bool overwrite = false);
    virtual void SetFilled(const PString & value, bool overwrite = false);
    virtual void Finished();

    PSpeechRecognition * m_recogniser;
    bool                 m_allowDTMF;
    PString              m_fieldName;
    PString              m_terminators;
    PString              m_value;
    bool                 m_usingDTMF;
    float                m_confidence;
    atomic<GrammarState> m_state;
    PTimeInterval        m_noInputTimeout;
    PTimer               m_timer;
};

typedef PParamFactory<PVXMLGrammar, PVXMLGrammarInit, PCaselessString> PVXMLGrammarFactory;


//////////////////////////////////////////////////////////////////

class PVXMLMenuGrammar : public PVXMLGrammar
{
    PCLASSINFO(PVXMLMenuGrammar, PVXMLGrammar);
  public:
    PVXMLMenuGrammar(const PVXMLGrammarInit & init);
    virtual void OnInput(const PString & input) override;
    virtual bool Process() override;

  protected:
    virtual void GetWordsToRecognise(PStringArray & words) const override;
};


//////////////////////////////////////////////////////////////////

class PVXMLDigitsGrammar : public PVXMLGrammar
{
    PCLASSINFO(PVXMLDigitsGrammar, PVXMLGrammar);
  public:
    PVXMLDigitsGrammar(const PVXMLGrammarInit & init);

    virtual void OnInput(const PString & input) override;

  protected:
    virtual void GetWordsToRecognise(PStringArray & words) const override;

    unsigned m_minDigits;
    unsigned m_maxDigits;
};


//////////////////////////////////////////////////////////////////

class PVXMLBooleanGrammar : public PVXMLGrammar
{
    PCLASSINFO(PVXMLBooleanGrammar, PVXMLGrammar);
  public:
    PVXMLBooleanGrammar(const PVXMLGrammarInit & init);

    virtual void OnInput(const PString & input) override;

  protected:
    virtual void GetWordsToRecognise(PStringArray & words) const override;

    PCaselessString m_yes;
    PCaselessString m_no;
};


//////////////////////////////////////////////////////////////////

class PVXMLTextGrammar : public PVXMLGrammar
{
    PCLASSINFO(PVXMLTextGrammar, PVXMLGrammar);
  public:
    PVXMLTextGrammar(const PVXMLGrammarInit & init);

    virtual void OnRecognition(PSpeechRecognition &, PSpeechRecognition::Transcript transcript) override;
    virtual void OnInput(const PString & input) override;
};


//////////////////////////////////////////////////////////////////

class PVXMLGrammarSRGS : public PVXMLGrammar
{
    PCLASSINFO(PVXMLGrammarSRGS, PVXMLGrammar);
  public:
    PVXMLGrammarSRGS(const PVXMLGrammarInit & init);

    virtual void OnInput(const PString & input) override;

  protected:
    virtual void GetWordsToRecognise(PStringArray & words) const override;

    struct Item : PObject
    {
      unsigned          m_minRepeat;
      unsigned          m_maxRepeat;
      PString           m_token;
      PString           m_value;
      unsigned          m_repeatCount;
      typedef std::vector< std::vector<Item> > Items;
      Items m_items; // A sequence of alternatives

      Item() : m_minRepeat(1), m_maxRepeat(1), m_repeatCount(0) { }
      bool Parse(PXMLElement & grammar, PXMLElement * element);
      GrammarState OnInput(const PString & input, bool usingDTMF);
      void AddWordsToRecognise(PStringArray & words) const;
    };

    Item m_rule;
};


//////////////////////////////////////////////////////////////////

class PVXMLCache : public PSafeObject
{
  public:
    PVXMLCache();

    struct Params
    {
      Params() : m_size(0), m_expires(0) { }

      PString       m_prefix;
      PString       m_key;
      PString       m_suffix;
      PTimeInterval m_maxAge;
      PTimeInterval m_maxStale;

      PFile         m_file;    // Cached file
      size_t        m_size;    // If zero then there is a cache miss, m_file must be written
      PTime         m_expires; // Expiry time of loaded resource for max stale
      PTime         m_date;    // Date of the loaded resource for max age
    };
    // Start cache operation, if return true, Finish must be called.
    virtual bool StartCache(Params & params);
    virtual bool FinishCache(Params & params, bool success);

    void SetDirectory(const PDirectory & directory);
    const PDirectory & GetDirectory() const { return m_directory; }

  protected:
    virtual PFilePath CreateFilename(const Params & params);

    PDirectory m_directory;

  P_REMOVE_VIRTUAL(bool, Get(const PString &, const PString &, const PString &, PFilePath &), false);
  P_REMOVE_VIRTUAL(bool, PutWithLock(const PString &, const PString &, const PString &, PFile &), false);
  P_REMOVE_VIRTUAL(PFilePath, CreateFilename(const PString &, const PString &, const PString &), "");
};

//////////////////////////////////////////////////////////////////

class PVXMLChannel;

class PVXMLSession : public PIndirectChannel, public PSSLCertificateInfo
{
  PCLASSINFO(PVXMLSession, PIndirectChannel);
  public:
    PVXMLSession();
    virtual ~PVXMLSession();

    // new functions
    PTextToSpeech * SetTextToSpeech(PTextToSpeech * tts, PBoolean autoDelete = false, const PStringOptions & options = PStringOptions());
    PTextToSpeech * SetTextToSpeech(const PString & ttsName, const PStringOptions & options = PStringOptions());
    PTextToSpeech * GetTextToSpeech() const { return m_textToSpeech; }
    bool SetSpeechRecognition(const PString & srName, const PStringOptions & options = PStringOptions());
    PString GetSpeechRecognition() const { PWaitAndSignal lock(m_grammersMutex); return m_speechRecognitionName.c_str(); }
    virtual PSpeechRecognition * CreateSpeechRecognition();

    typedef PSafePtr<PVXMLCache, PSafePtrMultiThreaded> CachePtr;
    void SetCache(const CachePtr & cache) { m_resourceCache = cache; }
    PVXMLCache & GetCache() { return *m_resourceCache; }
    const PVXMLCache & GetCache() const { return *m_resourceCache; }

    void SetRecordDirectory(const PDirectory & dir) { m_recordDirectory = dir; }
    const PDirectory & GetRecordDirectory() const { return m_recordDirectory; }

    void SetProxies(const PHTTP::Proxies & proxies);
    virtual PHTTPClient * CreateHTTPClient();

    virtual PBoolean Load(const PString & source);
    virtual PBoolean LoadFile(const PFilePath & file, const PString & firstForm = PString::Empty());
    virtual PBoolean LoadURL(const PURL & url);
    virtual PBoolean LoadVXML(const PString & xml, const PString & firstForm = PString::Empty());
    virtual PBoolean IsLoaded() const { return m_currentXML.get() != NULL; }

    virtual bool Open(const PString & mediaFormat, unsigned sampleRate = 8000, unsigned channels = 1);
    virtual PBoolean Close() override;

    PVXMLChannel * GetAndLockVXMLChannel();
    void UnLockVXMLChannel() { m_sessionMutex.Signal(); }
    PMutex & GetSessionMutex() { return m_sessionMutex; }

    virtual PBoolean PlayText(const PString & text, PTextToSpeech::TextType type = PTextToSpeech::Default, PINDEX repeat = 1, PINDEX delay = 0);

    virtual PBoolean PlayFile(const PString & fn, PINDEX repeat = 1, PINDEX delay = 0, PBoolean autoDelete = false);
    virtual PBoolean PlayData(const PBYTEArray & data, PINDEX repeat = 1, PINDEX delay = 0);
    virtual PBoolean PlayCommand(const PString & data, PINDEX repeat = 1, PINDEX delay = 0);
    virtual PBoolean PlayResource(const PString & src, PINDEX repeat = 1, PINDEX delay = 0, PXMLElement * element = NULL);
    virtual PBoolean PlayTone(const PString & toneSpec, PINDEX repeat = 1, PINDEX delay = 0);
    virtual PBoolean PlayElement(PXMLElement & element);

    //virtual PBoolean PlayMedia(const PURL & url, PINDEX repeat = 1, PINDEX delay = 0);
    virtual PBoolean PlaySilence(PINDEX msecs = 0);
    virtual PBoolean PlaySilence(const PTimeInterval & timeout);

    virtual PBoolean PlayStop();

    virtual void SetPause(PBoolean pause);
    virtual void GetBeepData(PBYTEArray & data, unsigned ms);

    virtual void OnUserInput(const PString & str);

    PString GetXMLError() const { return m_lastXMLError; }

    virtual void OnEndDialog();
    virtual void OnEndSession();

    P_DECLARE_TRACED_ENUM(TransferType,
      BridgedTransfer,
      BlindTransfer,
      ConsultationTransfer
    );
    P_DECLARE_TRACED_ENUM(TransferStatus,
      NotTransfering,
      TransferInProgress,
      TransferCompleted,
      // States after this are termination states
      TransferSuccessful,
      TransferBusy,
      TransferCongestion,
      TransferNoAnswer,
      TransferUnknown,
      TransferUnsupported,
      TransferFailed
    );

    /* Do the <transfer> operation.
       Returns: TransferInProgress if proceeding with transfer,
                TransferUnsupported if type not supported,
                TransferFailed for most other error
    */
    virtual TransferStatus OnTransfer(const PString & destination, TransferType type);
    void SetTransferStatus(TransferStatus status, const PString & protocolError = PString::Empty());

    PStringToString GetVariables() const;
    virtual PCaselessString GetVar(const PString & varName) const;
    virtual bool SetVar(const PString & varName, const PString & val);

    PCaselessString GetProperty(
      const PString & propName,
      const PXMLElement * overrideElement = NULL,
      const PString & attrName = PString::Empty()
    ) const;
    PTimeInterval GetTimeProperty(
      const PString & propName,
      PXMLElement * overrideElement = NULL,
      const PString & attrName = PString::Empty()
    ) const
    {
      return PTimeInterval(GetProperty(propName, overrideElement, attrName));
    }
    void SetProperty(
      const PString & name,
      const PString & value,
      bool session = false
    );

    void SetDialogVar(const PString & varName, const PString & value);

    void SetConnectionVars(
      const PString & localURI,
      const PString & remoteURI,
      const PString & protocolName,
      const PString & protocolVersion,
      const PArray<PStringToString> & redirects,
      const PStringToString & aai,
      bool originator
    );

    bool SetCurrentForm(const PString & id, bool fullURI);
    enum CatchHierarchy { e_CatchStarted, e_CatchNextDot, e_CatchNextScope };
    bool GoToEventHandler(PXMLElement & element, const PString & eventName, bool exitIfNotFound, CatchHierarchy catchHierarchy = e_CatchStarted);

    // overrides from VXMLChannelInterface
    virtual void OnEndRecording(PINDEX bytesRecorded, bool timedOut);
    virtual void Trigger();


    virtual PBoolean TraverseRootNode(PXMLElement & element);
    virtual PBoolean TraverseBlock(PXMLElement & element);
    virtual PBoolean TraversedBlock(PXMLElement & element);
    virtual PBoolean TraverseReprompt(PXMLElement & element);
    virtual PBoolean TraverseAudio(PXMLElement & element);
    virtual PBoolean TraverseBreak(PXMLElement & element);
    virtual PBoolean TraverseValue(PXMLElement & element);
    virtual PBoolean TraverseSayAs(PXMLElement & element);
    virtual PBoolean TraversedSayAs(PXMLElement & element);
    virtual PBoolean TraverseVoice(PXMLElement & element);
    virtual PBoolean TraverseGoto(PXMLElement & element);
    virtual PBoolean TraverseGrammar(PXMLElement & element);
    virtual PBoolean TraversedGrammar(PXMLElement & element);
    virtual PBoolean TraverseRecord(PXMLElement & element);
    virtual PBoolean TraversedRecord(PXMLElement & element);
    virtual PBoolean TraverseIf(PXMLElement & element);
    virtual PBoolean TraversedIf(PXMLElement & element);
    virtual PBoolean TraverseElseIf(PXMLElement & element);
    virtual PBoolean TraverseElse(PXMLElement & element);
    virtual PBoolean TraverseExit(PXMLElement & element);
    virtual PBoolean TraverseVar(PXMLElement & element);
    virtual PBoolean TraverseAssign(PXMLElement & element);
    virtual PBoolean TraverseSubmit(PXMLElement & element);
    virtual PBoolean TraverseMenu(PXMLElement & element);
    virtual PBoolean TraversedMenu(PXMLElement & element);
    virtual PBoolean TraverseChoice(PXMLElement & element);
    virtual PBoolean TraverseProperty(PXMLElement & element);
    virtual PBoolean TraverseDisconnect(PXMLElement & element);
    virtual PBoolean TraverseForm(PXMLElement & element);
    virtual PBoolean TraversedForm(PXMLElement & element);
    virtual PBoolean TraversePrompt(PXMLElement & element);
    virtual PBoolean TraversedPrompt(PXMLElement & element);
    virtual PBoolean TraverseField(PXMLElement & element);
    virtual PBoolean TraversedField(PXMLElement & element);
    virtual PBoolean TraverseTransfer(PXMLElement & element);
    virtual PBoolean TraversedTransfer(PXMLElement & element);
    virtual PBoolean TraverseScript(PXMLElement & element);

    __inline PVXMLChannel * GetVXMLChannel() const { return (PVXMLChannel *)readChannel; }
#if P_VXML_VIDEO
    const PVideoOutputDevice & GetVideoReceiver() const { return m_videoReceiver; }
          PVideoOutputDevice & GetVideoReceiver()       { return m_videoReceiver; }
    const PVideoInputDevice & GetVideoSender() const { return m_videoSender; }
          PVideoInputDevice & GetVideoSender()       { return m_videoSender; }
    static bool SetSignLanguageAnalyser(const PString & dllName);
#endif // P_VXML_VIDEO

    PHTTPCookies GetCookies() const { PWaitAndSignal lock(m_cookieMutex);  return m_cookies; }

    const PString & GetLanguage() const { return m_xmlLanguage; }

  protected:
    virtual bool InternalLoadVXML(const PURL & url, const PString & xml, const PString & firstForm);
    virtual void InternalStartThread();
    virtual void InternalThreadMain();
    virtual void InternalStartVXML();
    virtual PString InternalGetName(PXMLElement & element, bool allowScope);
    virtual PCaselessString InternalGetVar(const PString & scope, const PString & varName) const;
    virtual void InternalSetVar(const PString & scope, const PString & varName, const PString & value);
    virtual bool RunScript(PXMLElement & element, const PString & script);
    virtual bool EvaluateExpr(PXMLElement & element, const PString & attrib, PString & result, bool allowEmpty = false);

    virtual bool SetCurrentNode(PXMLObject * newNode);
    virtual bool ProcessNode();
    virtual bool ProcessEvents();
    virtual bool NextNode(bool processChildren);
    void NextElseIfNode(PXMLElement & ifElement, PINDEX pos);
    bool SelectMenuChoice(PXMLElement & choice);
    bool ExecuteCondition(PXMLElement & element);
    void ClearBargeIn();
    void FlushInput();
    bool IsFinalProcessing() const { return m_closing || m_promptMode == e_FinalProcessing; }

    PURL NormaliseResourceName(const PString & src);
    PHTTPClient * InternalCreateHTTPClient();

    PDECLARE_MUTEX(m_sessionMutex);

    PString          m_xmlLanguage;
    PHTTPCookies     m_cookies;
    PDECLARE_MUTEX(  m_cookieMutex);

    PTextToSpeech  * m_textToSpeech;
    bool             m_autoDeleteTextToSpeech;
    PString          m_speechRecognitionName;
    PStringOptions   m_speechRecognitionOptions;

    CachePtr m_resourceCache;

    PDECLARE_MUTEX(m_httpMutex);
    PHTTP::Proxies m_httpProxies;

#if P_VXML_VIDEO
    void SetRealVideoSender(PVideoInputDevice * device);
    friend class PVXMLSignLanguageAnalyser;
    PDECLARE_ScriptFunctionNotifier(PVXMLSession, SignLanguagePreviewFunction);
    PDECLARE_ScriptFunctionNotifier(PVXMLSession, SignLanguageControlFunction);

    class VideoReceiverDevice : public PVideoOutputDevice
    {
      PCLASSINFO(VideoReceiverDevice, PVideoOutputDevice)
    public:
      VideoReceiverDevice(PVXMLSession & vxmlSession);
      ~VideoReceiverDevice() { Close(); }
      virtual PStringArray GetDeviceNames() const override;
      virtual PBoolean Open(const PString & deviceName, PBoolean startImmediate = true) override;
      virtual PBoolean IsOpen() override;
      virtual PBoolean Close() override;
      virtual PBoolean SetColourFormat(const PString & colourFormat) override;
      virtual PBoolean SetFrameData(const FrameData & frameData) override;
      int GetAnalayserInstance() const { return m_analayserInstance; }
    protected:
      PVXMLSession & m_vxmlSession;
      int            m_analayserInstance;
    };
    VideoReceiverDevice m_videoReceiver;

    class VideoSenderDevice : public PVideoInputDeviceIndirect
    {
      PCLASSINFO(VideoSenderDevice, PVideoInputDeviceIndirect)
    public:
      VideoSenderDevice();
      ~VideoSenderDevice() { Close(); }
      virtual void SetActualDevice(PVideoInputDevice * actualDevice, bool autoDelete = true) override;
      virtual PBoolean Start() override;
      bool IsRunning() const { return m_running; }
    protected:
      virtual bool InternalGetFrameData(BYTE * buffer, PINDEX & bytesReturned, bool & keyFrame, bool wait) override;
      bool m_running;
    };
    VideoSenderDevice m_videoSender;
#endif // P_VXML_VIDEO

    PThread     *    m_vxmlThread;
    bool             m_closing;
    bool             m_abortVXML;
    PSyncPoint       m_waitForEvent;
    PURL             m_newURL;
    PAutoPtr<PXML>   m_newXML;
    PString          m_lastXMLError;
    PString          m_newFormName;
    PAutoPtr<PXML>   m_currentXML;
    PXMLObject  *    m_currentNode;
    bool             m_speakNodeData;
    bool             m_bargeIn;
    bool             m_bargingIn;
    PStringSet       m_dialogFieldNames;
    unsigned         m_promptCount;
    enum { e_NormalPrompt, e_Reprompt, e_EventPrompt, e_FinalProcessing } m_promptMode;
    PTextToSpeech::TextType m_promptType;
    std::map<std::string, unsigned> m_eventCount;

    struct Properties : PStringToString
    {
#if PTRACING
      PString m_node;
      Properties(const char * node) : m_node(node) { }
      Properties(const PXMLElement & node) { m_node = PSTRSTRM(node.PrintTrace()); }
#else
      Properties() { }
      Properties(const PXMLElement &) { }
#endif
    };
    std::list<Properties> m_properties;

    bool LoadCachedResource(
      const PURL & url,
      PXMLElement * element,
      const PString & maxagePropName,
      const PString & maxstalePropName,
      PBYTEArray & data
    );
    bool LoadActualResource(
      const PURL & url,
      const PTimeInterval & timeout,
      PBYTEArray & data,
      PVXMLCache::Params & cacheParams
    );
    virtual void LoadGrammar(const PVXMLGrammarInit & init);
    void ClearGrammars();
    void StartGrammars();
    void OnUserInputToGrammars();
    void OnAudioInputToGrammars(const short * samples, size_t count);
    bool IsGrammarRunning() const;
    typedef PList<PVXMLGrammar> Grammars;
    Grammars       m_grammars;
    PDECLARE_MUTEX(m_grammersMutex);
    char           m_defaultMenuDTMF;

    PAutoPtr<PScriptLanguage> m_scriptContext;

    PSyncQueue<PString> m_userInputQueue;

    enum {
      NotRecording,
      RecordingInProgress,
      RecordingComplete
    }          m_recordingStatus;
    bool       m_recordStopOnDTMF;
    PString    m_recordingName;
    PTime      m_recordingStartTime;
    PDirectory m_recordDirectory;

    PXMLElement * m_transferElement;
    TransferType m_transferType;
    TransferStatus m_transferStatus;
    PString m_transferError;
    PTime m_transferStartTime;
    PTimer m_transferTimeout;
    PDECLARE_NOTIFIER(PTimer, PVXMLSession, OnTransferTimeout);
    void CompletedTransfer();

    friend class PVXMLChannel;
    friend class PVXMLMenuGrammar;
    friend class PVXMLGrammar;
    friend class PVXMLGrammarSRGS;
    friend class VideoReceiverDevice;
    friend class PVXMLNodeHandler;
    friend class PVXMLTraverseEvent;
    friend class PVXMLTraverseFilled;
    friend class PVXMLTraverseLog;
    friend class PVXMLPlayableHTTP;
};


//////////////////////////////////////////////////////////////////

class PVXMLRecordable : public PObject
{
  PCLASSINFO(PVXMLRecordable, PObject);
  public:
    PVXMLRecordable();

    virtual PBoolean Open(const PString & arg) = 0;

    virtual bool OnStart(PVXMLChannel & incomingChannel) = 0;
    virtual void OnStop() { }

    virtual PBoolean OnFrame(PBoolean /*isSilence*/) { return false; }

    const PTimeInterval & GetFinalSilence() const { return m_finalSilence; }
    void SetFinalSilence(const PTimeInterval & v) { m_finalSilence = v > 0 ? v : 5000; }

    const PTimeInterval & GetMaxDuration() const { return m_maxDuration; }
    void SetMaxDuration(const PTimeInterval & v) { m_maxDuration = v > 0 ? v : 86400000; }

  protected:
    PSimpleTimer  m_silenceTimer;
    PSimpleTimer  m_recordTimer;
    PTimeInterval m_finalSilence;
    PTimeInterval m_maxDuration;
};

//////////////////////////////////////////////////////////////////

class PVXMLRecordableFilename : public PVXMLRecordable
{
  PCLASSINFO(PVXMLRecordableFilename, PVXMLRecordable);
  public:
    PBoolean Open(const PString & arg) override;
    bool OnStart(PVXMLChannel & incomingChannel) override;
    PBoolean OnFrame(PBoolean isSilence) override;

  protected:
    PFilePath m_fileName;
};

//////////////////////////////////////////////////////////////////

class PVXMLPlayable : public PObject
{
  PCLASSINFO(PVXMLPlayable, PObject);
  public:
    PVXMLPlayable();
    ~PVXMLPlayable();

    virtual PBoolean Open(PVXMLChannel & chan, const PString & arg, PINDEX delay, PINDEX repeat, PBoolean autoDelete);

    virtual bool OnStart() = 0;
    virtual bool OnRepeat();
    virtual bool OnDelay();
    virtual void OnStop();

    virtual void SetRepeat(PINDEX v) 
    { m_repeat = v; }

    virtual PINDEX GetRepeat() const
    { return m_repeat; }

    virtual PINDEX GetDelay() const
    { return m_delay; }

    void SetFormat(const PString & fmt)
    { m_format = fmt; }

    friend class PVXMLChannel;

  protected:
    PVXMLChannel * m_vxmlChannel;
    PChannel * m_subChannel;
    PINDEX   m_repeat;
    PINDEX   m_delay;
    PString  m_format;
    bool     m_autoDelete;
    bool     m_delayDone; // very tacky flag used to indicate when the post-play delay has been done
};

//////////////////////////////////////////////////////////////////

class PVXMLPlayableStop : public PVXMLPlayable
{
  PCLASSINFO(PVXMLPlayableStop, PVXMLPlayable);
  public:
    virtual bool OnStart() override;
};

//////////////////////////////////////////////////////////////////

class PVXMLPlayableHTTP : public PVXMLPlayable
{
  PCLASSINFO(PVXMLPlayableHTTP, PVXMLPlayable);
  public:
    virtual PBoolean Open(PVXMLChannel & chan, const PString & arg, PINDEX delay, PINDEX repeat, PBoolean autoDelete) override;
    virtual bool OnStart() override;
  protected:
    PURL m_url;
};

//////////////////////////////////////////////////////////////////

class PVXMLPlayableData : public PVXMLPlayable
{
  PCLASSINFO(PVXMLPlayableData, PVXMLPlayable);
  public:
    virtual PBoolean Open(PVXMLChannel & chan, const PString & arg, PINDEX delay, PINDEX repeat, PBoolean autoDelete) override;
    void SetData(const PBYTEArray & data);
    virtual bool OnStart() override;
    virtual bool OnRepeat() override;
  protected:
    PBYTEArray m_data;
};

//////////////////////////////////////////////////////////////////

#if P_DTMF

#include <ptclib/dtmf.h>

class PVXMLPlayableTone : public PVXMLPlayableData
{
  PCLASSINFO(PVXMLPlayableTone, PVXMLPlayableData);
  public:
    virtual PBoolean Open(PVXMLChannel & chan, const PString & arg, PINDEX delay, PINDEX repeat, PBoolean autoDelete) override;
  protected:
    PTones m_tones;
};

#endif // P_DTMF


//////////////////////////////////////////////////////////////////

class PVXMLPlayableCommand : public PVXMLPlayable
{
  PCLASSINFO(PVXMLPlayableCommand, PVXMLPlayable);
  public:
    virtual PBoolean Open(PVXMLChannel & chan, const PString & arg, PINDEX delay, PINDEX repeat, PBoolean autoDelete) override;
    virtual bool OnStart() override;
    virtual void OnStop() override;

  protected:
    PString m_command;
};

//////////////////////////////////////////////////////////////////

class PVXMLPlayableFile : public PVXMLPlayable
{
  PCLASSINFO(PVXMLPlayableFile, PVXMLPlayable);
  public:
    virtual PBoolean Open(PVXMLChannel & chan, const PString & arg, PINDEX delay, PINDEX repeat, PBoolean autoDelete) override;
    virtual bool OnStart() override;
    virtual bool OnRepeat() override;
    virtual void OnStop() override;
  protected:
    PFilePath m_filePath;
};

//////////////////////////////////////////////////////////////////

class PVXMLPlayableFileList : public PVXMLPlayableFile
{
  PCLASSINFO(PVXMLPlayableFileList, PVXMLPlayableFile);
  public:
    PVXMLPlayableFileList();
    virtual PBoolean Open(PVXMLChannel & chan, const PString & arg, PINDEX delay, PINDEX repeat, PBoolean autoDelete) override;
    virtual bool OnStart() override;
    virtual bool OnRepeat() override;
    virtual void OnStop() override;
  protected:
    PStringArray m_fileNames;
    PINDEX       m_currentIndex;
};

//////////////////////////////////////////////////////////////////

PQUEUE(PVXMLQueue, PVXMLPlayable);

//////////////////////////////////////////////////////////////////

class PVXMLChannel : public PDelayChannel
{
    PCLASSINFO(PVXMLChannel, PDelayChannel);
  protected:
    PVXMLChannel(unsigned frameDelay, PINDEX frameSize);
  public:
    ~PVXMLChannel();

    virtual bool Open(PVXMLSession * session, unsigned sampleRate, unsigned channels);

    // overrides from PIndirectChannel
    virtual PBoolean IsOpen() const override;
    virtual PBoolean Close() override;
    virtual PBoolean Read(void * buffer, PINDEX amount) override;
    virtual PBoolean Write(const void * buf, PINDEX len) override;

    // new functions
    virtual PString GetAudioFormat() const = 0;
    bool IsMediaPCM() const { return GetAudioFormat() == VXML_PCM16; }

    virtual unsigned GetSampleRate() const = 0;
    virtual bool SetSampleRate(unsigned rate) = 0;
    virtual unsigned GetChannels() const = 0;
    virtual bool SetChannels(unsigned channels) = 0;

    virtual PString GetMediaFileSuffix() const;
    virtual bool AdjustMediaFilename(const PFilePath & ifn, PFilePath & ofn);
    virtual PChannel * OpenMediaFile(const PFilePath & fn, bool recording);

    // Incoming channel functions
    virtual PBoolean WriteFrame(const void * buf, PINDEX len) = 0;
    virtual PBoolean IsSilenceFrame(const void * buf, PINDEX len) const = 0;

    virtual bool QueueRecordable(PVXMLRecordable * newItem);
    bool EndRecording(bool timedOut);
    bool IsRecording() const { return m_recordable != NULL; }

    // Outgoing channel functions
    virtual PBoolean ReadFrame(void * buffer, PINDEX amount) = 0;
    virtual PINDEX CreateSilenceFrame(void * buffer, PINDEX amount) = 0;
    virtual void GetBeepData(PBYTEArray &, unsigned) { }

    virtual PBoolean QueuePlayable(const PString & type, const PString & str, PINDEX repeat = 1, PINDEX delay = 0, PBoolean autoDelete = false);
    virtual PBoolean QueuePlayable(PVXMLPlayable * newItem);
    virtual PBoolean QueueData(const PBYTEArray & data, PINDEX repeat = 1, PINDEX delay = 0);

    virtual void FlushQueue();
    virtual PBoolean IsPlaying() const { return m_currentPlayItem != NULL || m_playQueue.GetSize() > 0; }

    void SetPause(PBoolean pause) { m_paused = pause; }
    void SetSilence(unsigned msecs);

    PVXMLSession & GetSession() const { return *PAssertNULL(m_vxmlSession); }

  protected:
    PVXMLSession * m_vxmlSession;

    PDECLARE_MUTEX(m_recordingMutex);
    PDECLARE_MUTEX(m_playQueueMutex);
    bool     m_closed;
    bool     m_paused;
    PINDEX   m_totalData;

    // Incoming audio variables
    PVXMLRecordable * m_recordable;

    // Outgoing audio variables
    PVXMLQueue      m_playQueue;
    PVXMLPlayable * m_currentPlayItem;
    PSimpleTimer    m_silenceTimer;

#if P_WAVFILE
    P_REMOVE_VIRTUAL(PWAVFile*,CreateWAVFile(const PFilePath&,PBoolean),NULL);
    P_REMOVE_VIRTUAL(PString,AdjustWavFilename(const PString&),NULL);
#endif
};


//////////////////////////////////////////////////////////////////

class PVXMLNodeHandler : public PObject
{
    PCLASSINFO(PVXMLNodeHandler, PObject);
  public:
    // Return true for process node, false to skip and move to next sibling
    virtual bool StartTraversal(PVXMLSession & /*session*/, PXMLElement & /*node*/) const;

    // Return true to move to next sibling, false to stay at this node.
    virtual bool FinishTraversal(PVXMLSession & /*session*/, PXMLElement & /*node*/) const;

#if PTRACING
    virtual const char * GetDescription() const = 0;
#endif

    static bool IsTraversing(PXMLElement & node);
};


template <PBoolean(PVXMLSession::*traversing)(PXMLElement &)>
class PVXMLSinglePhaseNodeHandler : public PVXMLNodeHandler
{
  PCLASSINFO(PVXMLSinglePhaseNodeHandler, PVXMLNodeHandler);
public:
  virtual bool StartTraversal(PVXMLSession & session, PXMLElement & node) const override
  {
    return PVXMLNodeHandler::StartTraversal(session, node) && (session.*traversing)(node);
  }
#if PTRACING
  virtual const char * GetDescription() const override { return "single phase"; }
#endif
};


template <
  PBoolean(PVXMLSession::*traversing)(PXMLElement &),
  PBoolean(PVXMLSession::*traversed)(PXMLElement &)
> class PVXMLDualPhaseNodeHandler : public PVXMLNodeHandler
{
  PCLASSINFO(PVXMLDualPhaseNodeHandler, PVXMLNodeHandler);
public:
  virtual bool StartTraversal(PVXMLSession & session, PXMLElement & node) const override
  {
    return PVXMLNodeHandler::StartTraversal(session, node) && (session.*traversing)(node);
  }
  virtual bool FinishTraversal(PVXMLSession & session, PXMLElement & node) const override
  {
    bool nextNode = true;
    if (IsTraversing(node))
      nextNode = (session.*traversed)(node);
    PVXMLNodeHandler::FinishTraversal(session, node);
    return nextNode;
  }
#if PTRACING
  virtual const char * GetDescription() const override { return "dual phase"; }
#endif
};


typedef PFactory<PVXMLNodeHandler, PCaselessString> PVXMLNodeFactory;


#endif // P_VXML

#endif // PTLIB_VXML_H


// End of file ////////////////////////////////////////////////////////////////
