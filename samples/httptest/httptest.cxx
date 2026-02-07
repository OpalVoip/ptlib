//
// httptest.cxx
//
// Copyright 2011 Vox Lucida Pty. Ltd.
//

#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <ptlib/sockets.h>
#include <ptclib/pssl.h>
#include <ptclib/http.h>
#include <ptclib/threadpool.h>


class HTTPConnection
{
  public:
    HTTPConnection(
      PHTTPSpace & httpNameSpace
#if P_SSL
      , PSSLContext * context
#endif
    ) : m_httpNameSpace(httpNameSpace)
#if P_SSL
      , m_context(context)
#endif
    {
    }

    void Work()
    {
      PTRACE(3, "HTTPTest\tStarted work on " << m_socket.GetPeerAddress());

      PHTTPServer httpServer(m_httpNameSpace);

#if P_SSL
      if (m_context != NULL) {
        PSSLChannel * ssl = new PSSLChannel(m_context);
        if (!ssl->Open(m_socket))
          return;
        if (!ssl->Accept())
          return;
        if (!httpServer.Open(ssl))
          return;
      }
      else
#endif
        if (!httpServer.Open(m_socket))
          return;

      unsigned count = 0;
      while (httpServer.ProcessCommand())
        ++count;

      PTRACE(3, "HTTPTest\tEnded work on " << m_socket.GetPeerAddress() << ", " << count << " transactions.");
    }

    PHTTPSpace  & m_httpNameSpace;
#if P_SSL
    PSSLContext * m_context;
#endif
    PTCPSocket    m_socket;
};


class HTTPTest : public PProcess
{
  PCLASSINFO(HTTPTest, PProcess)
  PSyncPoint m_done;
public:
  PDECLARE_HttpPoolNotifier(HTTPTest, OnPoolDone)
  {
    m_done.Signal();
  }

  void ClientPool(PHTTP::Commands cmd, const PArgList & args)
  {
    PHTTPClientPool pool;
#if P_SSL
    pool.SetSSLCredentials(args.GetOptionString("ca"),
                           args.GetOptionString("certificate"),
                           args.GetOptionString("private-key"));
#endif

    pool.QueueRequest(PHTTPClientPool::Request(cmd, args[0], PCREATE_NOTIFIER(OnPoolDone)));
    m_done.Wait();
  }


  void Client(PHTTP::Commands cmd, const PArgList & args)
  {
    PHTTPClient client;
#if P_SSL
    client.SetSSLCredentials(args.GetOptionString("ca"),
                             args.GetOptionString("certificate"),
                             args.GetOptionString("private-key"));
#endif

    bool ok;
    switch (cmd) {
      case PHTTP::GET :
      {
        PString str;
        ok = client.GetTextDocument(args[0], str);
        if (ok)
          cout << "Response body: \"" << str << '"' << endl;
        break;
      }

      case PHTTP::DELETE :
        ok = client.DeleteDocument(args[0]);
        break;

      case PHTTP::PUT :
        ok = client.PutDocument(args[0], PFilePath(args[1]));
        break;

      case PHTTP::POST :
      {
        PMIMEInfo outMIME;
        ok = client.PostData(args[0], outMIME, args[1]);
        break;
      }

      default :
        cerr << args.Usage("[ url [ file ] ]");
        return;
    }
    if (ok)
      cout << client.GetNameFromCommand(cmd) << " sucessful.";
    else
      cout << "Error in " << client.GetNameFromCommand(cmd)
      << " code=" << client.GetLastResponseCode()
      << " info=\"" << client.GetLastResponseInfo() << '"';
    cout << endl;
  }


  void Server(const PArgList & args)
  {
    PHTTPListener listener;
    auto & httpSpace = listener.GetSpace();
    httpSpace.AddResource(new PHTTPString("index.html", "Hello", "text/plain"));
    httpSpace.AddResource(new PHTTPString("sub/index.html", "Hello Sub", "text/plain"));
    httpSpace.AddResource(new PHTTPString("sub/fred.html", "Fred", "text/plain"));
    httpSpace.AddResource(new PHTTPString("sub/fred/nurk/esquire.html", "Fred Nurk Esquire", "text/plain"));
    httpSpace.AddResource(new PHTTPString("sub/wildcard/*", "Wildcard", "text/plain"));
    httpSpace.AddResource(new PHTTPDirectory("files", PDirectory()));

    if (!PAssert(!httpSpace.AddResource(new PHTTPString("index.html", "Hello", "text/plain")), PLogicError))
      return;
    if (!PAssert(httpSpace.AddResource(new PHTTPString("index.html", "Hello", "text/plain"), PHTTPSpace::Overwrite), PLogicError))
      return;

#if P_SSL
    if (args.HasOption('s')) {
      listener.SetSSLCredentials(args.GetOptionString("ca", "*"),
                                 args.GetOptionString("certificate", "certificate.pem"),
                                 args.GetOptionString("private-key", "privatekey.pem"),
                                 true);
    }
    if (!listener.ListenForHTTP(args.GetOptionString('i'),
                                args.GetOptionAs('p', (WORD)(listener.HasSSLCertificates() ? 443 : 80)),
                                PSocket::CanReuseAddress,
                                args.GetOptionString('Q', "100").AsUnsigned())) {
      cerr << "Could not listen on port " << listener.GetPort() << endl;
      return;
    }
    cout << "Listening for " << (listener.HasSSLCertificates() ? "https" : "http")
         << " on port " << listener.GetPort() << endl;
#else
    if (!listener.ListenForHTTP(args.GetOptionString('i'),
                                args.GetOptionAs('p', 80),
                                PSocket::CanReuseAddress,
                                args.GetOptionString('Q', "100").AsUnsigned())) {
      cerr << "Could not listen on port " << listener.GetPort() << endl;
      return;
    }
    cout << "Listening for http on port " << listener.GetPort() << endl;
#endif

    cout << "\nPress Enter to exit HTTP server test ..." << flush;
    cin.get();
  }


  virtual void Main() override
  {
    PArgList & args = GetArguments();
    args.Parse("h-help.       print this help message.\n"
               "O-operation:  do a GET/POST/PUT/DELETE/auth, if absent then acts as server\n"
               "P-pool.       do above operation using client pool"
               "p-port:       port number to listen on (default 80 or 443).\n"
               "i-interface:  interface to listen on (default is all).\n"
#if P_SSL
               "s-secure.     SSL/TLS mode for server.\n"
               "-ca:          SSL/TLS client certificate authority file/directory.\n"
               "-certificate: SSL/TLS server certificate.\n"
               "-private-key: SSL/TLS server private key.\n"
#endif
               "T-theads:  max number of threads in pool(default 10)\n"
               "Q-queue:   max queue size for listening sockets(default 100).\n"
               PTRACE_ARGLIST
    );

    PTRACE_INITIALISE(args);

    if (!args.IsParsed() || args.HasOption('h')) {
      cerr << args.Usage("[ options] [ url [ file ] ]");
      return;
    }

    if (args.HasOption('O')) {
      PCaselessString op = args.GetOptionString('O');
      if (op == "auth") {
        if (args.GetCount() < 3) {
          cerr << args.Usage("<cmd> <uri> <header-string>");
          // -O auth REGISTER http://example.org "Digest realm=http-auth@example.org,qop=auth,auth-int,algorithm=MD5,nonce=7ypfxwf94BcCAzFZH4GiTo0v"
          return;
        }

        struct PHTTPClientAuthenticator : PHTTPClientAuthentication::AuthObject
        {
          PString m_cmd;
          PString m_uri;
          PMIMEInfo m_mime;
          virtual PMIMEInfo & GetMIME() { return m_mime; }
          virtual PString GetURI() { return m_uri; }
          virtual PHTTPContentProcessor * GetContentProcessor() { return NULL; }
          virtual PString GetMethod() { return m_cmd; }
        } authObject;
        authObject.m_cmd = args[0];
        authObject.m_uri = args[1];
        PHTTPClientDigestAuthentication authDigest;
        authDigest.SetUsername("username");
        authDigest.SetPassword("password");
        authDigest.SetAuthRealm("realm");
        authDigest.SetClientNonce("cnonce");
        if (!authDigest.Parse(args[2], false))
          cerr << "Could not parse digest authentication header." << endl;
        else if (authDigest.Authorise(authObject))
          cout << "Authorisation successful: " <<   authObject.GetMIME().GetString(PHTTP::AuthorizationTag);
        else
          cout << "Authorisation failed." << endl;
        return;
      }

      PINDEX cmd = PHTTPClient().GetCommandFromName(op);
      if (cmd == P_MAX_INDEX) {
        cerr << args.Usage("[ options] [ url [ file ] ]");
        return;
      }

      if (args.GetCount() < (cmd == PHTTP::PUT || cmd == PHTTP::POST ? 2 : 1)) {
        cerr << args.Usage("[ options] [ url [ file ] ]");
        return;
      }

      if (args.HasOption('P'))
        ClientPool((PHTTP::Commands)cmd, args);
      else
        Client((PHTTP::Commands)cmd, args);
    }
    else
      Server(args);
  }
};


PCREATE_PROCESS(HTTPTest)

// End of hello.cxx
