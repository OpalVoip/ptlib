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

#if P_AWS_SDK

#include <ptlib/pprocess.h>
#include <ptclib/aws_sdk.h>
#include <ptclib/http.h>

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


static void AddSignedHeader(PURL & url, PStringStream & canonical, const char * key, const PString & value, bool first = false)
{
  url.SetQueryVar(key, value);
  if (!first)
    canonical << '&';
  canonical << key << '=' << PURL::TranslateString(value, PURL::QueryTranslation);
}


PURL PAwsClientBase::SignURL(const PURL & urlToSign,
                             const PString & service,
                             const HeaderMap & headers,
                             const char * command,
                             const char * operation,
                             const char * algorithm) const
{
  PURL url = urlToSign;

  PString hostname = url.GetHostName();
  if (hostname.find('.') == std::string::npos)
    url.SetHostName(PSTRSTRM(hostname << '.' << GetRegion() << ".amazonaws.com"));

  PString timestamp = PTime().AsString(PTime::ShortISO8601, PTime::UTC);
  PString credentialDatestamp = timestamp.Left(8);
  PString credentialScope = PSTRSTRM(credentialDatestamp << '/' << GetRegion() << '/' << service << '/' << operation);

  PStringStream canonical;
  canonical << command << '\n' << url.GetPathStr() << '\n';

  // The following must be in ANSI order
  AddSignedHeader(url, canonical, "X-Amz-Algorithm", algorithm, true);
  AddSignedHeader(url, canonical, "X-Amz-Credential", PSTRSTRM(GetAccessKeyId() << '/' << credentialScope));
  AddSignedHeader(url, canonical, "X-Amz-Date", timestamp);
  AddSignedHeader(url, canonical, "X-Amz-Expires", "300");
  AddSignedHeader(url, canonical, "X-Amz-SignedHeaders", "host");
  for (HeaderMap::const_iterator it = headers.begin(); it != headers.end(); ++it)
    AddSignedHeader(url, canonical, it->first, it->second);

  canonical << "\nhost:" << url.GetHostPort() << "\n\nhost\n";

  PMessageDigest::Result result;
  PMessageDigestSHA256::Encode("", result);
  canonical << result.AsHex();
  PMessageDigestSHA256::Encode(canonical, result);

  PString stringToSign = PSTRSTRM(algorithm << '\n' << timestamp << '\n' << credentialScope << '\n' << result.AsHex());

  PHMAC_SHA256 signer;
  signer.SetKey("AWS4" + GetSecretAccessKey()); signer.Process(credentialDatestamp, result);
  signer.SetKey(result); signer.Process(GetRegion().c_str(), result);
  signer.SetKey(result); signer.Process(service, result);
  signer.SetKey(result); signer.Process(operation, result);
  signer.SetKey(result); signer.Process(stringToSign, result);
  url.SetQueryVar("X-Amz-Signature", result.AsHex());

  PTRACE(4, "Signed URL : " << url << "\nCanonical string : \n'" << canonical << "'\nStringToSign:\n'" << stringToSign << '\'');
  return url;
}


#endif // P_AWS_SDK
