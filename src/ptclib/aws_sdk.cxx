/*
 * aws_sdk.cxx
 *
 * AWS SDK interface wrapper
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

#include <ptclib/aws_sdk.h>

#if P_AWS_SDK

#include <ptlib/pprocess.h>

P_PUSH_MSVC_WARNINGS(26495)
#define USE_IMPORT_EXPORT
#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/LogSystemInterface.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentials.h>
P_POP_MSVC_WARNINGS()


#define PTraceModule() "AWS-SDK"

#ifdef P_AWS_SDK_LIBDIR
  #pragma comment(lib, P_AWS_SDK_LIBDIR(core))
#endif


class PAwsSdkWrap : public PProcessStartup
{
private:
  Aws::SDKOptions m_options;
  std::atomic<bool> m_inititalised;

#if PTRACING

  class Logger : public Aws::Utils::Logging::LogSystemInterface
  {
    const PRegularExpression m_suppression;
  public:
    Logger()
      : m_suppression(getenv("PTLIB_AWS_LOG_SUPPRESSION"), PRegularExpression::IgnoreCase|PRegularExpression::Extended)
    { }

  private:
    virtual Aws::Utils::Logging::LogLevel GetLogLevel() const override
    {
      return static_cast<Aws::Utils::Logging::LogLevel>(PTrace::GetLevel()+1);
    }

    virtual void Log(Aws::Utils::Logging::LogLevel logLevel, const char */*tag*/, const char *formatStr, ...) override
    {
      auto level = static_cast<unsigned>(logLevel)-1;
      if (PTrace::CanTrace(level)) {
        va_list args;
        va_start(args, formatStr);
        PTRACE_BEGIN(level, NULL, "AWS-Log") << pvsprintf(formatStr, args) << PTrace::End;
        va_end(args);
      }
    }
#if AWS_SDK_VERSION_MAJOR > 1 || AWS_SDK_VERSION_MINOR > 11 || AWS_SDK_VERSION_PATCH > 300
    virtual void vaLog(Aws::Utils::Logging::LogLevel logLevel, const char* /*tag*/, const char * formatStr, va_list args) override
    {
      auto level = static_cast<unsigned>(logLevel)-1;
      if (PTrace::CanTrace(level)) {
        PTRACE_BEGIN(level, NULL, "AWS-Log") << pvsprintf(formatStr, args) << PTrace::End;
      }
    }
#endif // AWS_SDK_VERSION_*

    virtual void LogStream(Aws::Utils::Logging::LogLevel logLevel, const char */*tag*/, const Aws::OStringStream &messageStream) override
    {
      auto level = static_cast<unsigned>(logLevel)-1;
      if (PTrace::CanTrace(level)) {
        std::string msg = messageStream.str();
        PINDEX dummy;
        if (!m_suppression.Execute(msg.c_str(), dummy))
          PTRACE_BEGIN(level, NULL, "AWS-Log") << msg << PTrace::End;
      }
    }

    virtual void Flush() override
    { }
  };

  static std::shared_ptr<Aws::Utils::Logging::LogSystemInterface> CreateLogger()
  {
    return std::make_shared<Logger>();
  }

#endif // PTRACING

public:
  PFACTORY_GET_SINGLETON(PProcessStartupFactory, PAwsSdkWrap);

  void Initialise()
  {
    if (m_inititalised.exchange(true))
      return;

    PTRACE(3, "Initialising API");
#if PTRACING
    m_options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Debug; // Just needs to be something other than Off
    m_options.loggingOptions.logger_create_fn = PAwsSdkWrap::CreateLogger;
#endif // PTRACING
    Aws::InitAPI(m_options);

    PTRACE(4, "Initialised API");
  }

  virtual void OnShutdown()
  {
    if (m_inititalised) {
      PTRACE(3, "Shutting down API");
      Aws::ShutdownAPI(m_options);
    }
  }
};

PFACTORY_CREATE_SINGLETON(PProcessStartupFactory, PAwsSdkWrap);


PAwsClientBase::PAwsClientBase()
{
  PAwsSdkWrap & wrap = PAwsSdkWrap::GetInstance();
  wrap.Initialise();

  SetProfile(PString::Empty());
}


PAwsClientBase::~PAwsClientBase()
{
}


bool PAwsClientBase::SetProfile(const PString & profile)
{
  PConfig env(PConfig::Environment);

  PString profileToUse = profile.empty() ? env.GetString("PTLIB_AWS_PROFILE", env.GetString("AWS_PROFILE")) : profile;
  if (profile.empty() || (profile == "default")) {
    PTRACE(4, "Default client config");
    m_clientConfig.reset(new Aws::Client::ClientConfiguration());
    if (m_lastRegion.empty())
      m_lastRegion = m_clientConfig->region;
    else
      m_clientConfig->region = m_lastRegion;
  }
  else {
    PTRACE(4, "Client config for \"" << profile << '"');
    m_clientConfig.reset(new Aws::Client::ClientConfiguration(profile.c_str()));
    if (!m_clientConfig->region.empty())
      m_lastRegion = m_clientConfig->region;
  }

  if (m_clientConfig->profileName.empty())
    return false;

  if (!m_credentials)
    SetAccessKey(env.GetString("PTLIB_AWS_ACCESS_KEY_ID", env.GetString("AWS_ACCESS_KEY_ID")),
                 env.GetString("PTLIB_AWS_SECRET_ACCESS_KEY", env.GetString("AWS_SECRET_ACCESS_KEY")));

  if (!m_credentials) {
    PConfig cfg(PProcess::Current().GetHomeDirectory() + ".aws/credentials", m_clientConfig->profileName.c_str());
    SetAccessKey(cfg.GetString("aws_access_key_id"), cfg.GetString("aws_secret_access_key"));
  }

  PTRACE(4, "Client config: "
         "profile=\"" << m_clientConfig->profileName << "\" "
         "region=\"" << m_clientConfig->region << "\" "
         "access-keys=\"" << (m_credentials ? "set" : "absent") << "\" "
         "proxy=" << m_clientConfig->proxyUserName << '@' << m_clientConfig->proxyHost << ':' << m_clientConfig->proxyPort);
  return true;
}


void PAwsClientBase::SetRegion(const PString & region)
{
  m_lastRegion = m_clientConfig->region = region.c_str();
}


PString PAwsClientBase::GetRegion() const
{
  return m_lastRegion;
}


void PAwsClientBase::SetAccessKey(const PString & accessKeyId, const PString & secretAccessKey)
{
  if (accessKeyId.empty() || secretAccessKey.empty())
    m_credentials.reset();
  else
    m_credentials.reset(new Aws::Auth::AWSCredentials(accessKeyId.c_str(), secretAccessKey.c_str()));
}

PString PAwsClientBase::GetAccessKeyId() const
{
  return m_credentials ? PString(m_credentials->GetAWSAccessKeyId().c_str()) : PString::Empty();
}


PString PAwsClientBase::GetSecretAccessKey() const
{
  return m_credentials ? PString(m_credentials->GetAWSSecretKey().c_str()) : PString::Empty();
}


PHTTP::SelectProxyResult PAwsClientBase::SetProxies(const PHTTP::Proxies & proxies)
{
  PURL proxy;
  PHTTP::SelectProxyResult result = proxies.Select(proxy, PSTRSTRM("https://" << GetRegion() << ".amazonaws.com"));
  if (result == PHTTP::e_HasProxy) {
    m_clientConfig->proxyScheme = proxy.GetScheme() == "https" ? Aws::Http::Scheme::HTTPS : Aws::Http::Scheme::HTTP;
    m_clientConfig->proxyUserName = proxy.GetUserName().c_str();
    m_clientConfig->proxyPassword = proxy.GetPassword().c_str();
    m_clientConfig->proxyHost = proxy.GetHostName().c_str();
    m_clientConfig->proxyPort = proxy.GetPort();
  }
  return result;
}


bool PAwsClientBase::SetOptions(const PStringOptions & options)
{
  if (!SetProfile(options.GetString("AWS-Profile")))
    return false;

  if (options.Has("AWS-Region"))
    SetRegion(options.GetString("AWS-Region"));

  if (options.Has("AWS-Access-Key-Id") && options.Has("AWS-Secret-Access-Key"))
    SetAccessKey(options.GetString("AWS-Access-Key-Id"), options.GetString("AWS-Secret-Access-Key"));

  SetProxies(options);
  return true;
}


#endif // P_AWS_SDK


PConstCaselessString PAwsSigner::AmzDate() { static PConstCaselessString s("X-Amz-Date"); return s; }
PConstCaselessString PAwsSigner::AmzAlgorithm() { static PConstCaselessString s("X-Amz-Algorithm"); return s; }
PConstCaselessString PAwsSigner::AmzCredential() { static PConstCaselessString s("X-Amz-Credential"); return s; }
PConstCaselessString PAwsSigner::AmzExpires() { static PConstCaselessString s("X-Amz-Expires"); return s; }
PConstCaselessString PAwsSigner::AmzSignedHeaders() { static PConstCaselessString s("X-Amz-SignedHeaders"); return s; }
PConstCaselessString PAwsSigner::AmzContentSHA256() { static PConstCaselessString s("X-Amz-Content-SHA256"); return s; }
PConstCaselessString PAwsSigner::AmzSignature() { static PConstCaselessString s("X-Amz-Signature"); return s; }
PConstCaselessString PAwsSigner::AmzQueryParametersToSign() { static PConstCaselessString s("X-Amz-Algorithm;X-Amz-Credential;X-Amz-Date;X-Amz-Expires;X-Amz-SignedHeaders"); return s; }

static void UriEncode(std::ostream& strm, const PString& str)
{
  // This is subtly different to PURL::TranslateString, well done Amazon!
  PConstString safeChars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~");
  for (size_t i = 0; i < str.length(); ++i) {
    if (safeChars.Find(str[i]) != P_MAX_INDEX)
      strm << str[i];
    else
      strm << '%' << hex << setw(2) << setfill('0') << (unsigned)(BYTE)(str[i]) << dec;
  }
}

bool PAwsSigner::Calculate(bool useHeaders)
{
  if (m_awsAccessKeyId.empty() || m_awsSecretAccessKey.empty()) {
    PTRACE(1, "AWS4 authentication requires access keys");
    return false;
  }

  PString hostname = m_uri.GetHostName();
  if (hostname.empty())
    hostname = m_headers.GetString(PHTTP::HostTag);

  PStringArray hostParts = hostname.Tokenise('.');
  switch (hostParts.size()) {
    case 0 :
      hostname += PSTRSTRM(m_service << '.' << m_region << ".amazonaws.com");
      break;
    case 1 :
      hostname += PSTRSTRM('.' << m_region << ".amazonaws.com");
    case 2 :
    case 3 :
      PTRACE(1, "AWS4 authentication requires valid host: " << hostname);
      return false;
    default :
      if (m_region.empty())
        m_region = hostParts[hostParts.size() - 3];
      if (m_service.empty())
        m_service = hostParts[hostParts.size() - 4];
  }

  if (m_region.empty()) {
    PTRACE(1, "AWS4 authentication requires a region");
    return false;
  }

  if (m_service.empty()) {
    PTRACE(1, "AWS4 authentication requires a service");
    return false;
  }

  if (m_bodyHash.empty())
    m_bodyHash = PMessageDigestSHA256::Encode(m_body, false);

  PString timestamp = PTime().AsString(PTime::ShortISO8601, PTime::UTC);
  PString credentialDatestamp = timestamp.Left(8);
  PString credentialScope = PSTRSTRM(credentialDatestamp << '/' << m_region << '/' << m_service << '/' << m_operation);
  PString credential = PSTRSTRM(m_awsAccessKeyId << '/' << credentialScope);

  PStringStream canonical;
  canonical << m_command << '\n';
  if (m_uri.GetRelativePath())
    canonical << '/';
  canonical << m_uri.GetPathStr() << '\n';

  auto& queryVars = m_uri.GetQueryVars();
  PSortedStringList sortedKeys;
  for (const auto& it : queryVars)
    sortedKeys.AppendString(it.first);

  if (sortedKeys.find(AmzAlgorithm()) != sortedKeys.end())
    m_uri.SetQueryVar(AmzAlgorithm(), m_algorithm);
  if (sortedKeys.find(AmzCredential()) != sortedKeys.end())
    m_uri.SetQueryVar(AmzCredential(), credential);
  if (sortedKeys.find(AmzDate()) != sortedKeys.end())
    m_uri.SetQueryVar(AmzDate(), timestamp);
  if (sortedKeys.find(AmzExpires()) != sortedKeys.end())
    m_uri.SetQueryVar(AmzExpires(), m_expires);
  if (sortedKeys.find(AmzSignedHeaders()) != sortedKeys.end())
    m_uri.SetQueryVar(AmzSignedHeaders(), m_headersToSign.ToLower());

  bool outputAmpersand = false;
  for (const auto& key : sortedKeys) {
    if (std::exchange(outputAmpersand, true))
      canonical << '&';
    UriEncode(canonical, key);
    canonical << '=';
    UriEncode(canonical, queryVars.Get(key));
  }
  canonical << '\n';

  if (useHeaders)
    m_headers.Set(AmzDate(), timestamp);

  sortedKeys.clear();
  for (const auto& it : m_headers) {
    PString key = it.first.ToLower();
    if (key.starts_with("x-amz-"))
      sortedKeys.AppendString(key);
  }
  for (const auto& key : m_headersToSign.Tokenise(';'))
    sortedKeys.AppendString(key.ToLower());
  for (const auto& key : sortedKeys)
    canonical << key << ':' << m_headers.Get(key).Trim() << '\n';
  canonical << '\n' << setfill(';') << sortedKeys << '\n' << m_bodyHash;

  PString stringToSign = PSTRSTRM(
      m_algorithm << '\n' <<
      timestamp << '\n' <<
      credentialScope << '\n' <<
      PMessageDigestSHA256::Encode(canonical, false)
  );

  PHMAC_SHA256 signer;
  PHMAC_SHA256::Result result;
  signer.SetKey("AWS4" + m_awsSecretAccessKey); signer.Process(credentialDatestamp, result);
  signer.SetKey(result); signer.Process(m_region.c_str(), result);
  signer.SetKey(result); signer.Process(m_service, result);
  signer.SetKey(result); signer.Process(m_operation, result);
  signer.SetKey(result); signer.Process(stringToSign, result);

  if (useHeaders) {
    PString auth = PSTRSTRM(
      "AWS4-HMAC-SHA256"
      " Credential=" << credential << ","
      " SignedHeaders=" << setfill(';') << sortedKeys << ","
      " Signature=" << result.AsHex()
    );
    m_headers.Set(AmzContentSHA256(), m_bodyHash);
    m_headers.Set(PHTTP::AuthorizationTag, auth);
    PTRACE(4, "AWS Signature:\n"
           "  Canonical    : " << canonical.ToLiteral() << "\n"
           "  StringToSign : " << stringToSign.ToLiteral() << "\n"
           "  Authorization: " << auth.ToLiteral());
  }
  else {
    m_uri.SetQueryVar(AmzSignature(), result.AsHex());
    PTRACE(4, "AWS Signature:\n"
           "  Canonical   : " << canonical.ToLiteral() << "\n"
           "  StringToSign: " << stringToSign.ToLiteral() << "\n"
           "  Signed URL  : " << m_uri);
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////////

PBoolean PHTTPAws4Authentication::Parse(const PString&, PBoolean proxy)
{
  return !proxy;
}


PBoolean PHTTPAws4Authentication::Authorise(AuthObject& authObject) const
{
  auto & headers = authObject.GetMIME();
  if (!headers.Has(PHTTP::ContentLengthTag)) {
    PTRACE(1, "AWS4 authentication requires Content-Length header");
    return false;
  }

  PAwsSigner signer;
  signer.m_uri = authObject.GetURI();
  signer.m_command = authObject.GetMethod();
  signer.m_headers = authObject.GetMIME();
  signer.m_headersToSign += ';' + PHTTP::ContentLengthTag();
  if (headers.Has(PHTTP::ContentTypeTag))
    signer.m_headersToSign += ';' + PHTTP::ContentTypeTag();
  if (headers.Has(PHTTP::AcceptTag))
    signer.m_headersToSign += ';' + PHTTP::AcceptTag();
  signer.m_awsAccessKeyId = m_username;
  signer.m_awsSecretAccessKey = m_password;

  PHTTPContentProcessor* processor = authObject.GetContentProcessor();
  if (processor == NULL)
    signer.m_body = authObject.GetEntityBody();
  else {
    PMessageDigestSHA256 digestor;
    digestor.Start();

    void* data;
    PINDEX len;
    processor->Reset();
    while ((data = processor->GetBuffer(len)) != NULL)
      digestor.Process(data, len);

    PMessageDigestSHA256::Result bodyHash;
    digestor.Complete(bodyHash);
    signer.m_bodyHash = bodyHash.AsHex();
  }

  if (!signer.Calculate(true))
    return false;

  headers = signer.m_headers;
  return true;
}

PConstString PHTTPAws4Authentication::SchemeName() { static PConstString s = "aws4"; return s; }

PFACTORY_CREATE(PHTTPClientAuthenticationFactory, PHTTPAws4Authentication, PHTTPAws4Authentication::SchemeName());

PURL_LEGACY_SCHEME(s3, false, false, true, false, false, false, false, false, true, false, 0)
