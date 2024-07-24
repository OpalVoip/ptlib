#include <ptlib.h>
#include <ptlib/pprocess.h>

#include <ptclib/pjson.h>

class JSONTest : public PProcess
{
  PCLASSINFO(JSONTest, PProcess);
 public:
  JSONTest();
  virtual void Main() override;
};

PCREATE_PROCESS(JSONTest);


struct MyJSON : PJSONRecord
{
  P_JSON_MEMBER(PString, m_a_string);
  P_JSON_MEMBER(PTime, m_a_time);
  P_JSON_MEMBER(bool, m_a_boolean);
  P_JSON_MEMBER(int, m_an_integer);
  P_JSON_MEMBER(unsigned, m_an_unsigned);
  P_JSON_MEMBER(long, m_a_long);
  P_JSON_MEMBER(int16_t, m_an_int16);
  P_JSON_MEMBER(uint16_t, m_a_uint16);
  P_JSON_MEMBER(int32_t, m_an_int32);
  P_JSON_MEMBER(uint32_t, m_a_uint32);
  P_JSON_MEMBER(int64_t, m_an_int64);
  P_JSON_MEMBER(uint64_t, m_a_uint64);
  P_JSON_MEMBER(float, m_a_float);
  P_JSON_MEMBER(double, m_a_double);
  P_JSON_MEMBER(long double, m_a_big_double);

  struct MyNested : PJSONRecord
  {
    P_JSON_MEMBER(PString, m_a_string);
    P_JSON_MEMBER(int, m_an_integer);
  };
  P_JSON_MEMBER(MyNested, m_an_object, PJSONMemberRecord);

  P_JSON_MEMBER(PString, m_a_string_array, PJSONMemberArray);
  P_JSON_MEMBER(bool, m_a_bool_array, PJSONMemberArray);

  P_JSON_MEMBER(MyNested, m_an_object_array, PJSONMemberArrayRecord);
};


JSONTest::JSONTest()
  : PProcess("JSON Test Program", "JSONTest", 1, 0, AlphaCode, 0)
{
}

void JSONTest::Main()
{
  PArgList & args = GetArguments();
  if (args.GetCount() > 0) {
    PJSON json;
    if (args[0] == "-")
      cin >> json;
    else
      json.FromString(args.GetParameters().ToString());
    if (json.IsValid())
      cout << "\n\n\nParsed JSON:\n" << setw(2) << json << endl;
    else
      cout << "Could not parse JSON\n";
    return;
  }

  PJSON json1(PJSON::e_Object);
  PJSON::Object & obj1 = json1.GetObject();
  obj1.SetString("one", "first");
  obj1.SetNumber("two", 2);
  obj1.SetBoolean("three", true);
  obj1.Set("four", PJSON::e_Array);
  PJSON::Array & arr1 = *obj1.Get<PJSON::Array>("four");
  arr1.AppendString("was");
  arr1.AppendString("here");
  arr1.AppendString("in");
  arr1.AppendNumber(42);
  arr1.AppendBoolean(true);
  obj1.Set("five", PJSON::e_Null);

  cout << "Test 1\n" << json1 << endl;

  PJSON json2("{\n\"test\" : \"hello world\",\n\"field2\" : null }");
  cout << "Test 2\n" << json2 << endl;

  PJSON json3("\n[ \"test\", \"hello world\"]");
  cout << "Test 3\n" << json3 << endl;

  PJSON json4(json1.AsString());
  cout << "Test 4\n" << json4 << endl;

  PJSON json5(json4);
  cout << "Test 5a\n" << json5 << endl;

  json5 = json3;
  cout << "Test 5b\n" << json5 << endl;

  cout << "Test 1 pretty A\n" << setw(4) << json1 << "\n"
          "Test 1 pretty B\n" << setprecision(4) << json1 << "\n"
          "Test 1 pretty C\n" << setprecision(3) << setw(6) << json1
       << endl;

  MyJSON test6;
  test6.m_a_string = "me";
  test6.m_a_time = PTime();
  test6.m_a_boolean = true;
  test6.m_an_integer = -1;
  test6.m_an_unsigned = 2;
  test6.m_a_long = -3;
  test6.m_an_int16 = -16;
  test6.m_a_uint16 = 16;
  test6.m_an_int32 = -32;
  test6.m_a_uint32 = 32;
  test6.m_an_int64 = -64;
  test6.m_a_uint64 = 64;
  test6.m_a_float = 2.718f;
  test6.m_a_double = 3.141;
  test6.m_a_big_double = 1E23;
  test6.m_an_object.m_a_string = "nested";
  test6.m_an_object.m_an_integer = 1234;
  test6.m_a_string_array.push_back("one");
  test6.m_a_string_array.push_back("two");
  test6.m_a_string_array.push_back("three");
  test6.m_an_object_array.resize(2);
  test6.m_an_object_array[0].m_a_string = "first";
  test6.m_an_object_array[0].m_an_integer = 12;
  test6.m_an_object_array[1].m_a_string = "second";
  test6.m_an_object_array[1].m_an_integer = 34;
  cout << "test6a " << test6.AsString() << "\n\n";
  bool ok = test6.FromString("{\"a_string\":\"them\",\"an_unsigned\":56,\"a_bool_array\":[false, true],\"an_object_array\":[{\"a_string\":\"replaced\"}]}");
  cout << "test6b " << ok << ' ' << test6.AsString() << "\n\n";
  cout << "test6c " << test6.FromString("[]") << endl;

#if P_SSL
  PJWT jwt;
  jwt.SetIssuer("Vox Lucida, Pty. Ltd.");
  jwt.SetIssuedAt(1516239022);
  jwt.SetPrivate("name", "Fred Nurk");
  PString enc = jwt.Encode("secret");
  bool good = enc == "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE1MTYyMzkwMjIsImlzcyI6IlZveCBMdWNpZGEsIFB0eS4gTHRkLiIsIm5hbWUiOiJGcmVkIE51cmsifQ.FVEXKAMmqRiOIozsmAZcSoGsN1GbPl4iZ_wCHRGjMQU";
  cout << "\nJWT: " << (good ? "(good) " : "(bad) ") << enc << endl;

  PJWT dec(enc, "secret");
  if (dec.IsValid())
    cout << "Decoded:\n" << dec << endl;
  else
    cout << "Invalid JWT\n";
#endif // P_SSL
}

