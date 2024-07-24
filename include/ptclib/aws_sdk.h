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

#if P_AWS_SDK

#include <ptclib/http.h>


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

  typedef std::map<PString, PString> HeaderMap; // Note key order important!

  PURL SignURL(
    const PURL & urlToSign,
    const PString & service,
    const HeaderMap & headers,
    const char * command = "GET",
    const char * operation = "aws4_request",
    const char * algorithm = "AWS4-HMAC-SHA256"
  ) const;
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
#endif // PTLIB_AWS_SDK_H