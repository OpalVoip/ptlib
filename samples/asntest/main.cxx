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

#include <ptclib/asner.h>

using namespace std;

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

void DISPLAY_RESULT()
{
  if (g_errorCount == 0)
    cout << "FINAL PASS: " << g_totalTests << " passed" << endl;
  else
    cout << "FINAL FAIL: " << g_errorCount << " failed of " << g_totalTests << endl;
}

////////////////////////////////////////////////

void Test1()
{
  const char literal[] = "hello world";
  PASN_BMPString pstring(literal);

  REQUIRE(pstring.GetValue() == literal);

  std::stringstream str;
  pstring.PrintOn(str);

  const char * output = " 11 characters {\n\
        0068 0065 006c 006c 006f 0020 0077 006f   hello wo\n\
        0072 006c 0064                            rld\n\
      }";

  REQUIRE(str.str() == output);
}

////////////////////////////////////////////////

void Test2()
{
  const wchar_t literal[] = L"Hell� world";
  PWCharArray arr(literal, sizeof(literal)/sizeof(wchar_t)-1);

  PASN_BMPString pstring(arr);

  REQUIRE(pstring.GetValue() == arr);

  std::stringstream str;
  pstring.PrintOn(str);

  const char * output = " 11 characters {\n\
        0048 0065 006c 006c fffd 0020 0077 006f   Hell  wo\n\
        0072 006c 0064                            rld\n\
      }";

  REQUIRE(str.str() == output);
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

  DISPLAY_RESULT();
}
