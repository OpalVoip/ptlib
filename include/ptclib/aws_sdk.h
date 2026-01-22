/*
 * aws_sdk.h
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

#include <ptlib/object.h>

#ifndef PTLIB_AWS_SDK_H
#define PTLIB_AWS_SDK_H

#include <ptclib/http.h>


#if P_AWS_SDK

/* Note, all the AWS code requires C++11 or better. So, we can use things like
   std::shared_ptr<> in this header too. Some compilers (MSVC) allow use of
   C++11 classes even if not explicitly stated C++11 is in use. Others (GCC) do
   not allow this, so you must configure them for C++11 or later to use AWS.
*/
#include <memory>


namespace Aws {
  namespace Client {
    struct ClientConfiguration;
  }
  namespace Auth {
    class AWSCredentials;
  }
};


class PAwsClientBase
{
protected:
  std::unique_ptr<Aws::Client::ClientConfiguration> m_clientConfig;
  std::unique_ptr<Aws::Auth::AWSCredentials> m_credentials;
  std::string m_lastRegion;
public:
  PAwsClientBase();
  ~PAwsClientBase();

  bool SetProfile(const PString & profile);

  void SetRegion(const PString & region);
  PString GetRegion() const;

  void SetAccessKey(const PString & accessKeyId, const PString & secretAccessKey);
  PString GetAccessKeyId() const;
  PString GetSecretAccessKey() const;

  PHTTP::SelectProxyResult SetProxies(const PHTTP::Proxies & proxies);

  bool SetOptions(const PStringOptions & options);
};


template <class Client> class PAwsClient : public PAwsClientBase
{
private:
  std::shared_ptr<Client> m_client;
public:
  std::shared_ptr<Client> GetClient()
  {
    if (!m_client) {
      if (m_credentials)
        m_client = std::make_shared<Client>(*m_credentials, *m_clientConfig);
      else
        m_client = std::make_shared<Client>(*m_clientConfig);
    }
    return m_client;
  }

  bool HasClient() const { return !!m_client; }

  void ClearClient()
  {
    m_client.reset();
  }
};


#endif //P_AWS_SDK


/** Class to wrap the signing of AWS requests.
 */
class PAwsSigner : public PObject
{
  PCLASSINFO(PAwsSigner, PObject);
public:
  PString   m_awsAccessKeyId;
  PString   m_awsSecretAccessKey;
  PString   m_region;
  PString   m_service;

  PURL      m_uri;
  PMIMEInfo m_headers;
  PString   m_body;

  PString   m_bodyHash;
  PString   m_headersToSign = PHTTP::HostTag();

  PString   m_command = "GET";
  PString   m_operation = "aws4_request";
  PString   m_algorithm = "AWS4-HMAC-SHA256";
  unsigned  m_expires = 300; // 5 minutes

  static PConstCaselessString AmzDate();
  static PConstCaselessString AmzAlgorithm();
  static PConstCaselessString AmzCredential();
  static PConstCaselessString AmzExpires();
  static PConstCaselessString AmzSignedHeaders();
  static PConstCaselessString AmzContentSHA256();
  static PConstCaselessString AmzSignature();
  static PConstCaselessString AmzQueryParametersToSign();

  bool Calculate(bool useHeaders);
};


class PHTTPAws4Authentication : public PHTTPClientAuthentication
{
  PCLASSINFO(PHTTPAws4Authentication, PHTTPClientAuthentication);
public:
  static PConstString SchemeName();

  virtual PBoolean Parse(
    const PString& auth,
    PBoolean proxy
  ) override;

  virtual PBoolean Authorise(
    AuthObject& authObject
  ) const override;
};

#endif // PTLIB_AWS_SDK_H
