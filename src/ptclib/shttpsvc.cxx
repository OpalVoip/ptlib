/*
 * shttpsvc.cxx
 *
 * Class for secure service applications using HTTPS as the user interface.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2001-2002 Equivalence Pty. Ltd.
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
#pragma implementation "shttpsvc.h"
#endif

#include <ptlib.h>

#ifdef P_HTTPSVC

#include <ptclib/shttpsvc.h>

#ifdef P_SSL

#define new PNEW


PSecureHTTPServiceProcess::PSecureHTTPServiceProcess(const Info & inf)
  : PHTTPServiceProcess(inf)
{
}


PSecureHTTPServiceProcess::~PSecureHTTPServiceProcess()
{
}


PSecureHTTPServiceProcess::Params::Params(const char * configPageName, const char * section)
  : PHTTPServiceProcess::Params(configPageName, section)
  , m_certificateFileKey("HTTP Certificate")
  , m_createCertificateKey("Create HTTP Certificate")
{
}


bool PSecureHTTPServiceProcess::InitialiseBase(PHTTPServiceProcess::Params & params)
{
  if (!PHTTPServiceProcess::InitialiseBase(params))
    return false;

  if (params.m_configPage == NULL)
    return true;

  Params * secure = dynamic_cast<Params *>(&params);
  if (secure == NULL || secure->m_certificateFileKey == NULL)
    return true;

  // SSL certificate file.
  PString certificateFile = params.m_configPage->AddStringField(secure->m_certificateFileKey, 250, PString::Empty(),
                                                                "Certificate for HTTPS user interface, if empty HTTP is used.", 1, 50);
  if (certificateFile.IsEmpty()) {
    DisableSSL();
    return true;
  }

  bool create = true;
  if (secure->m_createCertificateKey != NULL)
    create = params.m_configPage->AddBooleanField(secure->m_createCertificateKey, true, "Automatically create certificate file if does not exist");

  if (SetServerCertificate(certificateFile, create))
    return true;

  PSYSTEMLOG(Fatal, "Could not load certificate \"" << certificateFile << '"');
  return false;
}


void PSecureHTTPServiceProcess::OnHTTPStarted(PHTTPServer & server)
{
  if (m_sslContext != NULL)
    server.GetConnectionInfo().SetPersistenceMaximumTransations(0);
}


bool PSecureHTTPServiceProcess::SetServerCertificate(const PFilePath & certificateFile,
                                                     bool create,
                                                     const char * dn)
{
  SetSSLCredentials(GetSSLCertificateAuthority(), certificateFile, certificateFile, create, dn);
  return HasSSLCertificates();
}


bool PSecureHTTPServiceProcess::SetServerCredentials(const PString & cert, const PString & key, const PString & ca)
{
  SetSSLCredentials(ca, cert, key, false);
  return HasSSLCertificates();
}


#endif //P_SSL

#endif // P_HTTPSVC

// End Of File ///////////////////////////////////////////////////////////////
