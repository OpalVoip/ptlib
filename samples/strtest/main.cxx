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
    cout << boolalpha << (wide.GetSize() == PARRAYSIZE(widestr) && memcmp(wide, widestr, sizeof(widestr)) == 0) << endl;
  }

  {
    PCaselessString caseless("hello world");
    PString cased("Hello World");
    cout << cased.ToLiteral() << " == \"Hello\" " << boolalpha << (cased == "Hello") << endl;
    cout << cased.ToLiteral() << " == \"hello\" " << boolalpha << (cased == "hello") << endl;
    cout << cased.ToLiteral() << " == \"Hello World\" " << boolalpha << (cased == "Hello World") << endl;
    cout << cased.ToLiteral() << " == " << caseless.ToLiteral() << ' ' << boolalpha << (cased == caseless) << endl;
    cout << cased.ToLiteral() << " == \"Hello\" " << boolalpha << (cased.NumCompare("Hello") == PObject::EqualTo) << endl;
    cout << cased.ToLiteral() << " == \"hello\" " << boolalpha << (cased.NumCompare("hello") == PObject::EqualTo) << endl;
    cout << caseless.ToLiteral() << " == " << cased.ToLiteral() << ' ' << boolalpha << (caseless == cased) << endl;
    cout << caseless.ToLiteral() << " == \"Hello\" " << boolalpha << (caseless == "Hello") << endl;
    cout << caseless.ToLiteral() << " == \"Hello\" " << boolalpha << (caseless.NumCompare("Hello") == PObject::EqualTo) << endl;
  }

  {
    wchar_t widestr[] = L"Hell� world";
    PString pstring(widestr, sizeof(widestr)/2-1);
    cout << pstring.Ellipsis(6) << endl;
    cout << pstring.Ellipsis(9, 4) << endl;
  }
}


////////////////////////////////////////////////
//
// test #2 - String stream test
//

void Test2()
{
  for (PINDEX i = 0; i < 2000; ++i) 
  {
    PStringStream str;
    str << "Test #" << 2;
  }

  PStringStream s;
  s << "This is a test of the string stream, integer: " << 5 << ", real: " << 5.6;
  cout << '"' << s << '"' << endl;
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

    void TestString(int count, const char * label)
    {
      if (finishFlag)
        return;

      S s = GetString();
      const char * ptr = C::ToConstCharStar(s);
      //const char * ptr = s.c_str();
      char buffer[20];
      strncpy_s(buffer, sizeof(buffer), ptr, 20);

      if (strcmp((const char *)buffer, SPECIALNAME)) {
        finishFlag = true;
        cerr << "String compare failed at " << count << " in " << label << " thread" << endl;
        return;
      }
      if (count % 10000 == 0)
        cout << "tested " << count << " in " << label << " thread" << endl;
    }

    class TestThread : public PThread
    {
      PCLASSINFO(TestThread, PThread);
      public:
        TestThread(StringHolder & _holder) 
        : PThread(NoAutoDeleteThread), holder(_holder)
        { Start(); }

        void Main() 
        { int count = 0; while (!finishFlag && count < COUNT_MAX) holder.TestString(count++, "sub"); }

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

  PThread * thread = holder.StartThread();
  finishFlag = false;
  int count = 0;
  while (!finishFlag && count < COUNT_MAX) 
    holder.TestString(count++, "main");
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
}
