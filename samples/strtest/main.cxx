//
// main.cxx
//
// String Tests
//
// Copyright 2011 Vox Lucida Pty. Ltd.
//


#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <string>

unsigned g_totalTests= 0;
unsigned g_errorCount = 0;

#define REQUIRE_QUIET(cond) \
do { \
  g_totalTests++; \
  if (!(cond)) { \
    cerr << "FAIL: " __FILE__ << "(" << __LINE__ << "): " << #cond << endl; \
    g_errorCount++; \
  } \
} while (0)

#define REQUIRE(cond) \
do { \
  g_totalTests++; \
  if (!(cond)) { \
    cerr << "FAIL: " __FILE__ << "(" << __LINE__ << "): " << #cond << endl; \
    g_errorCount++; \
  } \
  else { \
    cout << "PASS: " __FILE__ << "(" << __LINE__ << "): " << #cond << endl; \
  } \
} while (0)


////////////////////////////////////////////////
//
// test #1 - PString test
//

void Test1()
{
  {
    wchar_t widestr[] = L"Hellò world";
    PString pstring(widestr, sizeof(widestr)/2-1);
    cout << pstring << endl;
    PWCharArray wide = pstring.AsWide();
    REQUIRE(wide.GetSize() == PARRAYSIZE(widestr));
    REQUIRE(memcmp(wide, widestr, sizeof(widestr)) == 0);
  }

  {
    PCaselessString caseless("hello world");
    PString cased("Hello World");

    REQUIRE(cased != "Hello");
    REQUIRE(cased != "hello");
    REQUIRE(cased == "Hello World");
    REQUIRE(cased != caseless);
    REQUIRE(cased.NumCompare("Hello") == PObject::EqualTo);
    REQUIRE(cased.NumCompare("hello") != PObject::EqualTo);
    REQUIRE(caseless == cased);
    REQUIRE(caseless != "Hello");
    REQUIRE(caseless.NumCompare("Hello") == PObject::EqualTo);
  }

  {
    // Use UTF-8 with special character so eillipsis counts are based on the
    // number of characters, not the number of bytes.
    PString pstring("Hell\xC7\x92 world");
    REQUIRE(pstring.Ellipsis(6)    == "Hell\xC7\x92\xE2\x80\xA6");
    REQUIRE(pstring.Ellipsis(9, 4) == "Hell\xE2\x80\xA6orld");
  }
}

////////////////////////////////////////////////
//
// test #2 - String stream test
//

void Test2()
{
  for (PINDEX i = 0; i < 2000; ++i) {
    PStringStream str;
    str   << "Test #" << 2;
    REQUIRE_QUIET(str == "Test #2");
  }

  PStringStream s;
  s     << "This is a test of the string stream, integer: " << 5 << ", real: " << 5.6;
  REQUIRE(s == "This is a test of the string stream, integer: 5, real: 5.6");
}


////////////////////////////////////////////////
//
// test #3 - regex test
//

void Test3()
{
  PRegularExpression regex;

  if (!regex.Compile("ab(cd)e(fg)", PRegularExpression::extended))
    cout << "Error " << regex.GetErrorText() << endl;

  PStringArray substrings;
  if (regex.Execute("abcdefgh", substrings))
    cout << "Matched " << substrings[0].ToLiteral() << ' ' << substrings[1].ToLiteral() << ' ' << substrings[2].ToLiteral() << endl;
  else
    cout << "No match" << endl;
}

////////////////////////////////////////////////
//
// test #4 - string concurrency test
//

#define SPECIALNAME     "openH323"
#define COUNT_MAX       2000000

bool finishFlag;

template <class S>
struct StringConv {
  static const char * ToConstCharStar(const S &) { return NULL; }
};

template <class S, class C>
class StringHolder
{
  public:
    StringHolder(const S & _str)
      : str(_str) { }
    S GetString() const { return str; }
    S str;

    void TestString()
    {
      if (finishFlag)
        return;

      S s = GetString();
      const char * ptr = C::ToConstCharStar(s);
      //const char * ptr = s.c_str();
      char buffer[20];
      strncpy_s(buffer, sizeof(buffer), ptr, 20);

      REQUIRE_QUIET(strcmp((const char *)buffer, SPECIALNAME) == 0);
    }

    class TestThread : public PThread
    {
      PCLASSINFO(TestThread, PThread);
      public:
        TestThread(StringHolder & _holder) 
        : PThread(NoAutoDeleteThread), holder(_holder)
        { Start(); }

        void Main() override
        {
          int count = 0;
          while (!finishFlag && count < COUNT_MAX)
            holder.TestString();
        }

        StringHolder & holder;
    };

    PThread * StartThread()
    {
      return new TestThread(*this);
    }

};

struct PStringConv : public StringConv<PString> {
  static const char * ToConstCharStar(const PString & s) { return (const char *)s; }
};

struct StdStringConv : public StringConv<std::string> {
  static const char * ToConstCharStar(const std::string & s) { return s.c_str(); }
};

void Test4()
{
  // uncomment this to test std::string
  //StringHolder<std::string, StdStringConv> holder(SPECIALNAME);
  
  // uncomment this to test PString
  StringHolder<PString, PStringConv> holder(SPECIALNAME);

  cerr << "testing string concurrency for " << COUNT_MAX << " iterations" << endl;

  PThread * thread = holder.StartThread();
  finishFlag = false;
  int count = 0;
  while (!finishFlag && count < COUNT_MAX) 
    holder.TestString();
  finishFlag = true;
  thread->WaitForTermination(9000);
  cerr << "finish" << endl;
  delete thread;
}

////////////////////////////////////////////////
//
// main
//

class StringTest : public PProcess
{
  PCLASSINFO(StringTest, PProcess)
  public:
    virtual void Main() override;
};

PCREATE_PROCESS(StringTest);

void StringTest::Main()
{
  //PMEMORY_ALLOCATION_BREAKPOINT(16314);
  Test1(); cout << "End of test #1\n" << endl;
  Test2(); cout << "End of test #2\n" << endl;
  Test3(); cout << "End of test #3\n" << endl;
  Test4(); cout << "End of test #4\n" << endl;

  if (g_errorCount == 0)
    cout << "FINAL PASS: " << g_totalTests << " passed" << endl;
  else
    cout << "FINAL FAIL: " << g_errorCount << " failed of " << g_totalTests << endl;
}
