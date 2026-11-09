/*
 * main.cxx
 *
 * PWLib application source file for LDAP Test
 *
 * Main program entry point.
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 *
 */

#include "precompile.h"
#include "main.h"

#include <ptclib/pldap.h>

#if !P_LDAP
#error Must have LDAP enabled for this application.
#endif


/* Test command lines:

search -h public.ldap.ucalgary.ca -x "cn=Cindy*"

add    -h ils.seconix.com -x "c=AU, cn=robertj@equival.com.au, objectClass=RTPerson" "cn=robertj@equival.com.au" surname=Jongbloed givenName=Robert c=AU
delete -h ils.seconix.com -x "c=AU, cn=robertj@equival.com.au, objectClass=RTPerson"
search -h ils.seconix.com -x -b "objectClass=RTPerson" "cn=*au"
search -h ils.seconix.com -x -b "objectClass=RTPerson" "cn=*" cn surname givenName -P
*/


PCREATE_PROCESS(LDAPTest);



LDAPTest::LDAPTest()
  : PProcess("Equivalence", "LDAP Test")
{
}


void LDAPTest::Main()
{
  PArgList & args = GetArguments();
  args.Parse("h:p:xb:s:P", false);

  if (args.GetCount() == 0) {
    Usage();
    return;
  }

  PLDAPSession ldap;
  if (!ldap.Open(args.GetOptionString('h'), (WORD)args.GetOptionString('p').AsUnsigned())) {
    cerr << "Could not open LDAP server at " << args[1];
    return;
  }

  if (args[0] *= "add") {
    Add(args, ldap);
    return;
  }

  if (args[0] *= "delete") {
    Delete(args, ldap);
    return;
  }

  if (args[0] *= "search") {
    Search(args, ldap);
    return;
  }

  cerr << "Invalid command: " << args[0];
}


void LDAPTest::Usage()
{
  cerr << "usage: " << GetFile().GetTitle() << " { add | delete | search } [ args ]\n"
          "   General arguments:\n"
          "      -h host    LDAP server\n"
          "      -p port    port on LDAP server\n"
          "      -s scope   one of base, one, or sub (search scope)\n"
          "      -x         Simple authentication\n"
          "\n"
          "   add arguments:\n"
          "      dn attribute=value [ attribute=value ... ]\n"
          "\n"
          "   delete arguments:\n"
          "      dn [ dn ... ]\n"
          "\n"
          "   search arguments:\n"
          "      filter [ attribute ... ]\n"
          "      -b basedn  base dn for search\n"
          "      -P         Pause between entries\n"
          "\n";
}


void LDAPTest::Add(PArgList & args, PLDAPSession & ldap)
{
  if (args.GetCount() < 3) {
    Usage();
    return;
  }

  if (ldap.Add(args[1], args.GetParameters(2)))
    cout << "Added " << args[1] << endl;
  else
    cout << "Could not add " << args[1] << ": " << ldap.GetErrorText() << endl;
}


void LDAPTest::Delete(PArgList & args, PLDAPSession & ldap)
{
  if (args.GetCount() < 2) {
    Usage();
    return;
  }

  for (PINDEX i = 1; i < args.GetCount(); i++) {
    if (ldap.Delete(args[i]))
      cout << args[i] << " removed." << endl;
    else
      cout << "Could not remove " << args[i] << ": " << ldap.GetErrorText() << endl;
  }
}


void LDAPTest::Search(PArgList & args, PLDAPSession & ldap)
{
  if (args.GetCount() < 2) {
    Usage();
    return;
  }

  ldap.SetBaseDN(args.GetOptionString('b'));

  if (args.HasOption('P')) {
    PLDAPSession::SearchContext context;
    if (ldap.Search(context, args[1], args.GetParameters(2))) {
      do {
        PStringToString entry;
        if (ldap.GetSearchResult(context, entry)) {
          cout << entry << "\n\nPress ENTER ..." << flush;
          cin.get();
          cout << '\n';
        }
      } while (ldap.GetNextSearchResult(context));
    }
    else
      cout << "Could not find entries for filter: " << args[1]
           << ": " << ldap.GetErrorText() << endl;
  }
  else {
    PList<PStringToString> data = ldap.Search(args[1], args.GetParameters(2));
    if (data.IsEmpty())
      cout << "Could not find entries for filter: " << args[1]
           << ": " << ldap.GetErrorText() << endl;
    else
      cout << setfill('\n') << data;
  }
}


// End of File ///////////////////////////////////////////////////////////////
