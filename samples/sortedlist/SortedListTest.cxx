
#include <ptlib.h>
#include "SortedListTest.h"
#include <ptclib/random.h>


PCREATE_PROCESS(SortedListTest);

class Fred : public PObject
{
    char m_character;
  public:
    Fred(char c)
      : m_character(c)
    {
    }

    Comparison Compare(const PObject & obj) const override
    {
      return Compare2(m_character, dynamic_cast<const Fred &>(obj).m_character);
    }

    void PrintOn(ostream & strm) const override
    {
      strm << m_character;
    }

    PINDEX HashFunction() const override
    {
      return std::abs(128-m_character)%23;
    }
};

class Safe : public PSafeObject
{
  char m_character;
public:
  Safe(char c)
    : m_character(c)
  { }

  Comparison Compare(const PObject & obj) const override
  {
    return Compare2(m_character, dynamic_cast<const Safe &>(obj).m_character);
  }

  void PrintOn(ostream & strm) const override
  {
    strm << m_character;
  }

  PINDEX HashFunction() const override
  {
    return std::abs(128-m_character)%23;
  }
};


PMutex coutMutex;

SortedListTest::SortedListTest()
  : PProcess("Reitek S.p.A.", "SortedListTest", 0, 0, BetaCode, 0)
{
}


void SortedListTest::Main()
{
  {
    PString str("fred");
    PCharArray chars("fred", 4);
    PBYTEArray bytes((const uint8_t *)"fred", 4);
    PShortArray shorts((const short *)L"fred", 4);
    PWORDArray words((const uint16_t *)L"fred", 4);
    PArray<Fred> a_fred; a_fred.Append(new Fred('f')); a_fred.Append(new Fred('r')); a_fred.Append(new Fred('e')); a_fred.Append(new Fred('d'));
    PStringArray a_strings; a_strings.AppendString('f'); a_strings.AppendString('r'); a_strings.AppendString('e'); a_strings.AppendString('d');
    PList<Fred> l_fred; l_fred.Append(new Fred('f')); l_fred.Append(new Fred('r')); l_fred.Append(new Fred('e')); l_fred.Append(new Fred('d'));
    PStringList l_strings; l_strings.AppendString('f'); l_strings.AppendString('r'); l_strings.AppendString('e'); l_strings.AppendString('d');
    PSortedList<Fred> sl_fred; sl_fred.Append(new Fred('f')); sl_fred.Append(new Fred('r')); sl_fred.Append(new Fred('e')); sl_fred.Append(new Fred('d'));
    PSortedStringList ls_strings; ls_strings.AppendString('f'); ls_strings.AppendString('r'); ls_strings.AppendString('e'); ls_strings.AppendString('d');
    PSet<Fred> s_fred; s_fred.Append(new Fred('f')); s_fred.Append(new Fred('r')); s_fred.Append(new Fred('e')); s_fred.Append(new Fred('d'));
    PStringSet s_strings; s_strings+='f'; s_strings+='r'; s_strings+='e'; s_strings+='d';
    PStringToString ssd_strings; ssd_strings.SetAt('f','F'); ssd_strings.SetAt('r','R'); ssd_strings.SetAt('e','E'); ssd_strings.SetAt('d','D');

    PSafeDictionary<PString, Safe> safe_dict;
    safe_dict.SetAt('f', new Safe('F'));
    safe_dict.SetAt('r', new Safe('R'));
    safe_dict.SetAt('e', new Safe('E'));
    safe_dict.SetAt('d', new Safe('D'));
    cout << "PSafeDictionary<PString, Safe> ";
    for (PSafeDictionary<PString, Safe>::iterator it = safe_dict.begin(); it != safe_dict.end(); ++it)
      cout << it->first << '=' << *it->second << ' ';
    cout << endl;
  }
  cin.get();

  {
    PSortedStringList ss;
    PAssert(ss.begin() == ss.end(), "Bad PSortedStringList implementation");
    ss.AppendString("fred");
    ss.AppendString("nurk");
    ss.AppendString("rocky");
    ss.AppendString("bullwinkle");
    ss.AppendString("boris");
    ss.AppendString("natasha");

    cout << "front()=\"" << ss.front() << "\", back()=\"" << ss.back() << "\"\n"
            "Forwards, iteration:\n";
    for (PSortedStringList::iterator it = ss.begin(); it != ss.end(); ++it)
      cout << "  \"" << *it << "\"\n";

    cout << "Reverse iteration:\n";
    for (PSortedStringList::reverse_iterator it = ss.rbegin(); it != ss.rend(); ++it)
      cout << "  \"" << *it << "\"\n";
    cout << endl;

    PSortedStringList::iterator found = ss.find("fred");
    PAssert(found != ss.end() && *found == "fred", "Bad PSortedStringList implementation");
    ss.erase(found);
  }

  for (PINDEX i = 10; i < 25; i++) {
    if (i < 20)
      new DoSomeThing1(i);
    else
      new DoSomeThing2(i);
  }
}


DoSomeThing1::DoSomeThing1(PINDEX _index)
  : PThread(AutoDeleteThread, NormalPriority, psprintf("DoSomeThing%u", _index))
  , m_index(_index)
{
  Start();
}


void DoSomeThing1::Main()
{
  PRandom rand(PRandom::Number());

  PINDEX i;

  for (i = 0; i < 5000; i++) {
    PString * p = new PString(rand.Generate());
    list.Append(p);
//    coutMutex.Wait();
//    cout << GetThreadName() << ": Added " << *p << " element to sorted list" << endl;
//    coutMutex.Signal();
  }

  for (;;) {

    PINDEX remove = rand.Generate(list.GetSize());
    for (i = 0; i < remove; i++) {
      PINDEX index = rand.Generate(list.GetSize() - 1);
      auto it = list.begin();
      std::advance(it, index);
      coutMutex.Wait();
      cout << GetThreadName() << ": Removing element " << *it << " at index position " << index << endl;
      coutMutex.Signal();
      list.erase(it);
    }

    PINDEX add = rand.Generate() % 1000 + 300;
    for (i = 0; i < add; i++) {
      PString * p = new PString(rand.Generate());
      coutMutex.Wait();
      cout << GetThreadName() << ": Adding element " << *p << "to sorted list" << endl;
      coutMutex.Signal();
      list.Append(p);
    }
  }
}


PSafeString::PSafeString(const PString & _string)
  : m_string(_string)
{
}


PObject::Comparison PSafeString::Compare(const PObject & obj) const
{
  const PSafeString & other = dynamic_cast<const PSafeString &>(obj);
  return m_string.Compare(other.m_string);
}

void PSafeString::PrintOn(ostream &strm) const
{
  strm << m_string;
}


DoSomeThing2::DoSomeThing2(PINDEX index)
  : PThread(AutoDeleteThread, NormalPriority, psprintf("DoSomeThing%u", index))
  , m_index(index)
{
  Start();
}


void DoSomeThing2::Main()
{
  PRandom rand(PRandom::Number());

  PINDEX i;

  for (i = 0; i < 1000; i++) {
    PSafeString * p = new PSafeString(rand.Generate());
    coutMutex.Wait();
    cout << GetThreadName() << ": Added " << *p << " element to sorted list" << endl;
    coutMutex.Signal();
    list.Append(p);
  }

  for (;;) {

    PINDEX remove = rand.Generate(list.GetSize());
    for (i = 0; i < remove; i++) {
      PINDEX index = rand.Generate(list.GetSize() - 1);
      auto it = list.begin();
      std::advance(it, index);
      coutMutex.Wait();
      cout << GetThreadName() << ": Removing element " << *it << " at index position " << index << endl;
      coutMutex.Signal();
      PAssert(list.Remove(&*it), PLogicError);
    }

    PINDEX add = rand.Generate() % 1000 + 300;
    for (i = 0; i < add; i++) {
      PSafeString * p = new PSafeString(rand.Generate());
      coutMutex.Wait();
      cout << GetThreadName() << ": Adding element " << *p << "to sorted list" << endl;
      coutMutex.Signal();
      list.Append(p);
    }

    list.DeleteObjectsToBeRemoved();
  }
}


