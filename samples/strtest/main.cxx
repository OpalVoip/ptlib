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

unsigned g_totalTests = 0;
unsigned g_errorCount = 0;

bool TestResult(bool condResult, const char * condText, const char * file, int line, bool noisy)
{
  std::string filename(file);
  size_t pos = filename.find_last_of("\\/");
  if (pos != std::string::npos)
    filename.erase(0, pos+1);
  g_totalTests++;
  if (condResult) {
    if (noisy)
      cout << "pass: " << filename << "(" << line << "): " << condText << endl;
  }
  else {
    cerr << "FAIL: " << filename << "(" << line << "): " << condText << endl;
    g_errorCount++;
  }
  return condResult;
}

#define REQUIRE_QUIET(cond) TestResult(cond, #cond, __FILE__, __LINE__, false)
#define REQUIRE(cond) TestResult(cond, #cond, __FILE__, __LINE__, true)


////////////////////////////////////////////////
//
// test #1 - PString test
//

void Test1()
{
  cout.setf(ios::boolalpha);
  {
    PString pstring1("hello world");
    PString pstring2(pstring1);

    P_DISABLE_MSVC_WARNINGS(4996, strcpy((char *)(const char *)pstring2, "overwrite"));

    REQUIRE(pstring1 == "hello world");
    REQUIRE(pstring2 == "overwrite");
  }
  {
    PString pstring1("hello world");
    PString pstring2(pstring1);

    P_DISABLE_MSVC_WARNINGS(4996, strcpy(pstring2.GetPointerAndSetLength(9), "overwrite"));

    REQUIRE(pstring1 == "hello world");
    REQUIRE(pstring2 == "overwrite");
  }
#ifdef P_HAS_WCHAR
  {
    wchar_t widestr[] = L"Hell\x1D2 world";
    PString pstring(widestr, sizeof(widestr)/sizeof(wchar_t)-1);
    REQUIRE(pstring == widestr);
    REQUIRE(pstring == "Hell\xC7\x92 world");
    PWCharArray wide = pstring.AsWide();
    REQUIRE(wide.GetSize() == PARRAYSIZE(widestr));
    REQUIRE(memcmp(wide, widestr, sizeof(widestr)) == 0);
  }
#endif

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

  {
    PString print;
    print.sprintf("Hello %s", "world");
    REQUIRE(print == "Hello world");
    print.sprintf(" %d times", 2);
    REQUIRE(print == "Hello world 2 times");

    print = psprintf("%999s.", "");
    REQUIRE(print.length() == 1000);
    REQUIRE(print[print.length()-1] == '.');
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
// test #3 - PBYTEArray test
//

void Test3()
{
  {
    PBYTEArray buffer1(1024);
    PBYTEArray buffer2(buffer1);

    REQUIRE(buffer1 == buffer2);
  }

  {
    PString str1("hello");
    PString str2(str1);

    str2 = "world";

    REQUIRE((void *)(str1.GetPointer() != (void *)str2.GetPointer()));
  }
}

////////////////////////////////////////////////
//
// test #4 - regex test
//

void Test4()
{
  PRegularExpression regex;

  if (!regex.Compile("b(cd)e(fg)", PRegularExpression::Extended))
    cout << "Error " << regex.GetErrorText() << endl;

  PStringArray substrings(3);
  if (REQUIRE(regex.Execute("abcdefgh", substrings))) {
    REQUIRE(substrings.GetSize() == 3);
    REQUIRE(substrings[0] == "bcdefg");
    REQUIRE(substrings[1] == "cd");
    REQUIRE(substrings[2] == "fg");
  }
}

////////////////////////////////////////////////
//
// test #5 - string concurrency test
//

static const char SPECIALNAME[] = "openH323";

template <class S>
struct StringConv
{
  static const char * ToConstCharStar(const S &) { return NULL; }
};

template <class S, class C>
class StringHolder
{
public:
  StringHolder(const S & _str)
    : str(_str)
  { }
  S GetString() const { return str; }
  S str;

  void TestString()
  {
    S s = GetString();
    const char * ptr = C::ToConstCharStar(s);
    REQUIRE_QUIET(ptr != NULL);
    REQUIRE_QUIET(strcmp(ptr, SPECIALNAME) == 0);
  }

  class TestThread : public PThread
  {
    PCLASSINFO(TestThread, PThread);
  public:
    TestThread(StringHolder & holder, PTimer & timer)
      : PThread(NoAutoDeleteThread)
      , m_holder(holder)
      , m_timer(timer)
    {
      Start();
    }

    void Main() override
    {
      while (m_timer.IsRunning())
        m_holder.TestString();
    }

    StringHolder & m_holder;
    PTimer & m_timer;
  };

  PThread * StartThread(PTimer & timer)
  {
    return new TestThread(*this, timer);
  }

};

struct PStringConv : public StringConv<PString>
{
  static const char * ToConstCharStar(const PString & s) { return (const char *)s; }
};

struct StdStringConv : public StringConv<std::string>
{
  static const char * ToConstCharStar(const std::string & s) { return s.c_str(); }
};

void Test5()
{
  // uncomment this to test std::string
  //StringHolder<std::string, StdStringConv> holder(SPECIALNAME);

  // uncomment this to test PString
  StringHolder<PString, PStringConv> holder(SPECIALNAME);

  PTimer timer(0, 30);
  cerr << "testing string concurrency for " << timer.GetResetTime() << " seconds" << endl;

  PThread * thread = holder.StartThread(timer);
  while (timer.IsRunning())
    holder.TestString();
  thread->WaitForTermination();
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
  Test5(); cout << "End of test #4\n" << endl;

  if (g_errorCount == 0)
    cout << "FINAL PASS: " << g_totalTests << " passed" << endl;
  else
    cout << "FINAL FAIL: " << g_errorCount << " failed of " << g_totalTests << endl;
}
