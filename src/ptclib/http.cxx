/*
 * http.cxx
 *
 * HTTP ancestor class and common classes.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2002 Equivalence Pty. Ltd.
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
#pragma implementation "http.h"
#endif

#include <ptlib.h>

#if P_HTTP

#include <ptlib/sockets.h>
#include <ptclib/http.h>
#include <ptclib/url.h>


//////////////////////////////////////////////////////////////////////////////
// PHTTP

static char const * const HTTPCommands[PHTTP::NumCommands] = {
  // HTTP 1.0 commands
  "GET", "HEAD", "POST",

  // HTTP 1.1 commands
  "PUT",  "DELETE", "TRACE", "OPTIONS", "PATCH",

  // HTTPS command
  "CONNECT"
};


const PCaselessString & PHTTP::HostTag             () { static const PConstCaselessString s("Host"); return s; }
const PCaselessString & PHTTP::AllowTag            () { static const PConstCaselessString s("Allow"); return s; }
const PCaselessString & PHTTP::AuthorizationTag    () { static const PConstCaselessString s("Authorization"); return s; }
const PCaselessString & PHTTP::ContentEncodingTag  () { static const PConstCaselessString s("Content-Encoding"); return s; }
const PCaselessString & PHTTP::ContentLengthTag    () { static const PConstCaselessString s("Content-Length"); return s; }
const PCaselessString & PHTTP::DateTag             () { static const PConstCaselessString s("Date"); return s; }
const PCaselessString & PHTTP::ExpiresTag          () { static const PConstCaselessString s("Expires"); return s; }
const PCaselessString & PHTTP::FromTag             () { static const PConstCaselessString s("From"); return s; }
const PCaselessString & PHTTP::IfModifiedSinceTag  () { static const PConstCaselessString s("If-Modified-Since"); return s; }
const PCaselessString & PHTTP::LastModifiedTag     () { static const PConstCaselessString s("Last-Modified"); return s; }
const PCaselessString & PHTTP::LocationTag         () { static const PConstCaselessString s("Location"); return s; }
const PCaselessString & PHTTP::PragmaTag           () { static const PConstCaselessString s("Pragma"); return s; }
const PCaselessString & PHTTP::PragmaNoCacheTag    () { static const PConstCaselessString s("no-cache"); return s; }
const PCaselessString & PHTTP::RefererTag          () { static const PConstCaselessString s("Referer"); return s; }
const PCaselessString & PHTTP::ServerTag           () { static const PConstCaselessString s("Server"); return s; }
const PCaselessString & PHTTP::UserAgentTag        () { static const PConstCaselessString s("User-Agent"); return s; }
const PCaselessString & PHTTP::WWWAuthenticateTag  () { static const PConstCaselessString s("WWW-Authenticate"); return s; }
const PCaselessString & PHTTP::MIMEVersionTag      () { static const PConstCaselessString s("MIME-Version"); return s; }
const PCaselessString & PHTTP::ConnectionTag       () { static const PConstCaselessString s("Connection"); return s; }
const PCaselessString & PHTTP::KeepAliveTag        () { static const PConstCaselessString s("Keep-Alive"); return s; }
const PCaselessString & PHTTP::UpgradeTag          () { static const PConstCaselessString s("Upgrade"); return s; }
const PCaselessString & PHTTP::WebSocketTag        () { static const PConstCaselessString s("websocket"); return s; }
const PCaselessString & PHTTP::WebSocketKeyTag     () { static const PConstCaselessString s("Sec-WebSocket-Key"); return s; }
const PCaselessString & PHTTP::WebSocketAcceptTag  () { static const PConstCaselessString s("Sec-WebSocket-Accept"); return s; }
const PCaselessString & PHTTP::WebSocketProtocolTag() { static const PConstCaselessString s("Sec-WebSocket-Protocol"); return s; }
const PCaselessString & PHTTP::WebSocketVersionTag () { static const PConstCaselessString s("Sec-WebSocket-Version"); return s; }
const PCaselessString & PHTTP::TransferEncodingTag () { static const PConstCaselessString s("Transfer-Encoding"); return s; }
const PCaselessString & PHTTP::ChunkedTag          () { static const PConstCaselessString s("chunked"); return s; }
const PCaselessString & PHTTP::ProxyConnectionTag  () { static const PConstCaselessString s("Proxy-Connection"); return s; }
const PCaselessString & PHTTP::ProxyAuthorizationTag(){ static const PConstCaselessString s("Proxy-Authorization"); return s; }
const PCaselessString & PHTTP::ProxyAuthenticateTag() { static const PConstCaselessString s("Proxy-Authenticate"); return s; }
const PCaselessString & PHTTP::ForwardedTag        () { static const PConstCaselessString s("Forwarded"); return s; }
const PCaselessString & PHTTP::SetCookieTag        () { static const PConstCaselessString s("Set-Cookie"); return s; }
const PCaselessString & PHTTP::CookieTag           () { static const PConstCaselessString s("Cookie"); return s; }
const PCaselessString & PHTTP::FormUrlEncoded      () { static const PConstCaselessString s("application/x-www-form-urlencoded"); return s; }
const PCaselessString & PHTTP::RequestHeadersTag   () { static const PConstCaselessString s("Access-Control-Request-Headers"); return s; }
const PCaselessString & PHTTP::RequestMethodsTag   () { static const PConstCaselessString s("Access-Control-Request-Methods"); return s; }
const PCaselessString & PHTTP::AllowOriginTag      () { static const PConstCaselessString s("Access-Control-Allow-Origin"); return s; }
const PCaselessString & PHTTP::AllowHeadersTag     () { static const PConstCaselessString s("Access-Control-Allow-Headers"); return s; }
const PCaselessString & PHTTP::ExposeHeadersTag    () { static const PConstCaselessString s("Access-Control-Expose-Headers"); return s; }
const PCaselessString & PHTTP::AllowMethodsTag     () { static const PConstCaselessString s("Access-Control-Allow-Methods"); return s; }
const PCaselessString & PHTTP::AllowCredentialsTag () { static const PConstCaselessString s("Access-Control-Allow-Credentials"); return s; }
const PCaselessString & PHTTP::MaxAgeTAG           () { static const PConstCaselessString s("Access-Control-Max-Age"); return s; }
const PString         & PHTTP::HttpProxyKey        () { static const PConstString         s("Http-Proxy"); return s; }
const PString         & PHTTP::HttpsProxyKey       () { static const PConstString         s("Https-Proxy"); return s; }
const PString         & PHTTP::NoProxyKey          () { static const PConstString         s("No-Proxy"); return s; }



PHTTP::PHTTP()
  : PInternetProtocol("www 80", NumCommands, HTTPCommands)
{
}


PHTTP::PHTTP(const char * defaultServiceName)
  : PInternetProtocol(defaultServiceName, NumCommands, HTTPCommands)
{
}


PINDEX PHTTP::ParseResponse(const PString & line)
{
  PINDEX endVer = line.Find(' ');
  if (endVer == P_MAX_INDEX)
    return SetLastResponse(PHTTP::BadResponse, "Bad response");

  m_lastResponseInfo = line.Left(endVer);
  PINDEX endCode = line.Find(' ', endVer+1);
  m_lastResponseCode = line(endVer+1,endCode-1).AsInteger();
  if (m_lastResponseCode == 0)
    m_lastResponseCode = PHTTP::BadResponse;
  m_lastResponseInfo &= line.Mid(endCode);
  return 0;
}


///////////////////////////////////////////////////////////////////////

PHTTP::Proxies::Proxies(const PConfig & cfg)
  : m_httpProxy(cfg.GetString("HTTP_PROXY", cfg.GetString("http_proxy")).Trim())
  , m_httpsProxy(cfg.GetString("HTTPS_PROXY", cfg.GetString("https_proxy", m_httpProxy)).Trim())
  , m_noProxy(cfg.GetString("NO_PROXY", cfg.GetString("no_proxy")))
{ }


PHTTP::Proxies::Proxies(const PStringOptions & options)
  : m_httpProxy(options.GetString(HttpProxyKey(), options.GetString("HTTP_PROXY", options.GetString("http_proxy"))).Trim())
  , m_httpsProxy(options.GetString(HttpsProxyKey(), options.GetString("HTTPS_PROXY", options.GetString("https_proxy", m_httpProxy))).Trim())
  , m_noProxy(options.GetString(NoProxyKey(), options.GetString("NO_PROXY", options.GetString("no_proxy"))))
{ }


void PHTTP::Proxies::ToOptions(PStringToString & options) const
{
  if (m_httpProxy.empty())
    options.RemoveAt(HttpProxyKey());
  else
    options.SetAt(HttpProxyKey(), m_httpProxy);

  if (m_httpsProxy.empty())
    options.RemoveAt(HttpsProxyKey());
  else
    options.SetAt(HttpsProxyKey(), m_httpProxy);

  if (m_noProxy.empty())
    options.RemoveAt(NoProxyKey());
  else
    options.SetAt(NoProxyKey(), m_noProxy);
}


PHTTP::SelectProxyResult PHTTP::Proxies::Select(PURL & proxyURL, const PURL & destURL) const
{
  PString proxyStr = m_httpProxy;
  if (destURL.GetScheme() == "https" || destURL.GetScheme() == "wss")
    proxyStr = m_httpsProxy;
  if (proxyStr.empty())
    return e_NoProxy;

  PStringSet no_proxy = m_noProxy.Tokenise(" ,;");
  if (no_proxy.Contains(destURL.GetHostName()))
    return e_NoProxy;
  if (no_proxy.Contains(destURL.GetHostPort(true)))
    return e_NoProxy;
  for (PStringSet::iterator it = no_proxy.begin(); it != no_proxy.end(); ++it) {
    if (*it == "*")
      return e_NoProxy;
    if (it->GetAt(0) == '.') {
      if (destURL.GetHostName() == it->Mid(1))
        return e_NoProxy;
      if (destURL.GetHostName().Right(it->length()) == *it)
        return e_NoProxy;
    }
    else if (it->GetAt(0) == '*') {
      PINDEX offset = it->GetAt(1) == '.' ? 2 : 1;
      if (destURL.GetHostName().Right(it->length()-offset) == it->Mid(offset))
        return e_NoProxy;
    }
  }

  if (proxyURL.Parse(proxyStr) || proxyURL.Parse("http://" + proxyStr))
    return e_HasProxy;

  PTRACE(2, &proxyURL, "Invalid URL in proxy: \"" << proxyStr << '"');
  return e_BadProxy;
}


///////////////////////////////////////////////////////////////////////

PHTTPCookies::Info::Info()
  : m_expiryTime(0)
  , m_peristent(false)
  , m_hostOnly(false)
  , m_secureOnly(false)
  , m_httpOnly(false)
{
}


bool PHTTPCookies::Info::operator<(const Info & other) const
{
  // If same name, which should be unique, then is never less than or greater thans
  if (m_name == other.m_name)
    return false;

  // Yes, this is a greater than, for a less than function returning true
  // This is so std::set<> orders longer paths before shorter paths as per RFC6265
  if (m_path.length() > other.m_path.length())
    return true;
  if (m_path.length() < other.m_path.length())
    return false;

  // If same length we use creation time as per RFC6265
  if (m_creationTime < other.m_creationTime)
    return true;
  if (m_creationTime > other.m_creationTime)
    return false;

  // If same creation time, use the name, which should be unique
  return m_name < other.m_name;
}


bool PHTTPCookies::Parse(const PString & strCookies,const PURL & url, const PTime & now)
{
  PStringArray cookies = strCookies.Lines();
  for (PINDEX i = 0; i < cookies.GetSize(); ++i) {
    PString cookie = cookies[i];
    PString nameValue, attributes;
    if (!cookie.Split(';', nameValue, attributes, PString::SplitBeforeNonEmpty)) {
      PTRACE(3, "Invalid cookie: \"" << cookie << '"');
      continue;
    }

    Info info;
    if (!nameValue.Split('=', info.m_name, info.m_value, PString::SplitTrim|PString::SplitBeforeNonEmpty)) {
      PTRACE(3, "Invalid cookie name: \"" << cookie << '"');
      continue;
    }

    int maxAge = INT_MAX;
    PStringArray fields = attributes.Tokenise(';');
    PINDEX fldIdx;
    for (fldIdx = 0; fldIdx < fields.GetSize(); ++fldIdx) {
      PCaselessString attrName, attrValue;
      fields[fldIdx].Split('=', attrName, attrValue, PString::SplitTrim|PString::SplitDefaultToBefore);

      if (attrName == "Domain")
        info.m_domain = attrValue.ToLower();
      else if (attrName == "Path")
        info.m_path = attrValue;
      else if (attrName == "HttpOnly")
        info.m_httpOnly = true;
      else if (attrName == "Secure")
        info.m_secureOnly = true;
      else if (attrName == "Expires") {
        if (!info.m_expiryTime.Parse(attrValue)) {
          maxAge = -1; // Illegal time
          break;
        }
        info.m_peristent = true;
      }
      else if (attrName == "Max-Age") {
        maxAge = attrValue.AsInteger();
        info.m_peristent = true;
      }
    }

    if (info.m_httpOnly && url.GetScheme().NumCompare("http") != PString::EqualTo) {
      PTRACE(3, "Invalid cookie scheme: \"" << cookie << '"');
      continue;
    }

    if (maxAge <= 0) {
      PTRACE(3, "Invalid cookie Expires/Max-Age: \"" << cookie << '"');
      continue;
    }

    if (maxAge != INT_MAX)
      info.m_expiryTime = now + PTimeInterval(0, maxAge);

    if (info.m_domain.empty()) {
      info.m_domain = url.GetHostName().ToLower();
      info.m_hostOnly = true;
    }
    if (info.m_domain[info.m_domain.length()-1] == '.')
      info.m_domain.erase(info.m_domain.length()-1, 1);

    if (info.m_path.empty())
      info.m_path = url.GetPathStr();
    if (info.m_path[0] != '/')
      info.m_path.Splice("/", 0);

    // Overwrite existing cookie
    m_cookies.erase(info);
    m_cookies.insert(info);
  }
  return !m_cookies.empty();
}


bool PHTTPCookies::Parse(const PMIMEInfo & mime, const PURL & url, const PTime & now)
{
  return Parse(mime.GetString(PHTTP::SetCookieTag), url, now);
}


PString PHTTPCookies::GetCookie(const PURL & url, const PTime & now) const
{
  PStringStream cookie;
  for (std::set<Info>::const_iterator it = m_cookies.begin(); it != m_cookies.end(); ++it) {
    if (!it->m_domain.empty() && url.GetHostName() != it->m_domain) {
      if (it->m_hostOnly)
        continue;
      if (url.GetHostName().Right(it->m_domain.length()) != it->m_domain)
        continue;
    }

    if (!it->m_path.empty()) {
      PString urlPath = url.GetPathStr();
      if (it->m_path != urlPath && urlPath.NumCompare(it->m_path+'/') != PString::EqualTo)
        continue;
    }

    if (it->m_secureOnly && url.GetScheme().Right(1) != "s")
      continue;

    if (it->m_httpOnly && url.GetScheme().NumCompare("http") != PString::EqualTo)
      continue;

    if (it->m_peristent && now > it->m_expiryTime)
      continue;

    if (it != m_cookies.begin())
      cookie << "; ";
    cookie << it->m_name << '=' << it->m_value;
  }
  return cookie;
}


void PHTTPCookies::AddCookie(PMIMEInfo & mime, const PURL & url, const PTime & now) const
{
  PString cookie = GetCookie(url, now);
  if (cookie.empty())
    mime.RemoveAt(PHTTP::CookieTag());
  else
    mime.SetAt(PHTTP::CookieTag(), cookie);
}


PString PHTTPCookies::AsString() const
{
  PStringStream strm;
  for (std::set<Info>::const_iterator it = m_cookies.begin(); it != m_cookies.end(); ++it) {
    if (it != m_cookies.begin())
      strm << "; ";
    strm << it->m_name << '=' << it->m_value
         << ";path=" << it->m_path
         << ";domain=" << it->m_domain;
    if (it->m_peristent)
      strm << ";expires=" << it->m_expiryTime.AsString(PTime::RFC1123);
    if (it->m_secureOnly)
      strm << ";secure";
  }
  return strm;
}


#endif // P_HTTP


// End Of File ///////////////////////////////////////////////////////////////
