
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
    PBYTEArray bytes((const BYTE *)"fred", 4);
    PShortArray shorts((const short *)L"fred", 4);
    PWORDArray words((const WORD *)L"fred", 4);

    PArray<Fred> a_fred;
    a_fred.Append(new Fred('f'));
    a_fred.Append(new Fred('r'));
    a_fred.Append(new Fred('e'));
    a_fred.Append(new Fred('d'));
    cout << "PArray<Fred> ";
    for (PArray<Fred>::iterator it = a_fred.begin(); it != a_fred.end(); ++it)
      cout << *it;
    cout << endl;

    PStringArray a_strings;
    a_strings.AppendString('f');
    a_strings.AppendString('r');
    a_strings.AppendString('e');
    a_strings.AppendString('d');
    cout << "PStringArray ";
    for (PStringArray::iterator it = a_strings.begin(); it != a_strings.end(); ++it)
      cout << *it;
    cout << endl;

    PList<Fred> l_fred;
    l_fred.Append(new Fred('f'));
    l_fred.Append(new Fred('r'));
    l_fred.Append(new Fred('e'));
    l_fred.Append(new Fred('d'));
    cout << "PList<Fred> ";
    for (PList<Fred>::iterator it = l_fred.begin(); it != l_fred.end(); ++it)
      cout << *it;
    cout << endl;

    PStringList l_strings;
    l_strings.AppendString('f');
    l_strings.AppendString('r');
    l_strings.AppendString('e');
    l_strings.AppendString('d');
    cout << "PStringList ";
    for (PStringList::iterator it = l_strings.begin(); it != l_strings.end(); ++it)
      cout << *it;
    cout << endl;

    PSortedList<Fred> sl_fred;
    sl_fred.Append(new Fred('f'));
    sl_fred.Append(new Fred('r'));
    sl_fred.Append(new Fred('e'));
    sl_fred.Append(new Fred('d'));
    cout << "PSortedList<Fred> ";
    for (PSortedList<Fred>::iterator it = sl_fred.begin(); it != sl_fred.end(); ++it)
      cout << *it;
    cout << endl;

    PSet<Fred> s_fred;
    s_fred.Append(new Fred('f'));
    s_fred.Append(new Fred('r'));
    s_fred.Append(new Fred('e'));
    s_fred.Append(new Fred('d'));
    cout << "PSet<Fred> ";
    for (PSet<Fred>::iterator it = s_fred.begin(); it != s_fred.end(); ++it)
      cout << *it;
    cout << endl;

    PStringSet s_strings;
    s_strings += 'f';
    s_strings += 'r';
    s_strings += 'e';
    s_strings += 'd';
    cout << "PStringSet ";
    for (PStringSet::iterator it = s_strings.begin(); it != s_strings.end(); ++it)
      cout << *it;
    cout << endl;

    PStringToString ssd_strings;
    ssd_strings.SetAt('f','F');
    ssd_strings.SetAt('r','R');
    ssd_strings.SetAt('e','E');
    ssd_strings.SetAt('d','D');
    cout << "PStringToString ";
    for (PStringToString::iterator it = ssd_strings.begin(); it != ssd_strings.end(); ++it)
      cout << it->first << '=' << it->second << ' ';
    cout << endl;
  }
  cin.get();

  {
    PSortedStringList ss;
    PAssert(ss.begin() == ss.end(), "Bad PSortedStringList implemetation");
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
    for (PSortedStringList::iterator it = ss.rbegin(); it != ss.rend(); --it)
      cout << "  \"" << *it << "\"\n";
    cout << endl;

    PSortedStringList::iterator found = ss.find("fred");
    PAssert(found != ss.end() && *found == "fred", "Bad PSortedStringList implemetation");
    ss.erase(found);
  }

  for (PINDEX i = 0; i < 15; i++) {
    if (i < 10)
      new DoSomeThing1(i);
    else
      new DoSomeThing2(i);
  }

  Suspend();
}


DoSomeThing1::DoSomeThing1(PINDEX _index)
  : PThread(1000, AutoDeleteThread, NormalPriority, psprintf("DoSomeThing1 %u", _index))
  , m_index(_index)
{
  Resume();
}


void DoSomeThing1::Main()
{
  list.AllowDeleteObjects();

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

    PINDEX remove = rand.Generate() % (list.GetSize() + 1);
    for (i = 0; i < remove; i++) {
      PINDEX index = rand.Generate() % list.GetSize();
      coutMutex.Wait();
      cout << GetThreadName() << ": Removing element " << list[index] << " at index position " << index << endl;
      coutMutex.Signal();
      if (index%2)
        list.Remove(&list[index]);
      else
        list.RemoveAt(index);
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


void PSafeString::PrintOn(ostream &strm) const
{
  strm << m_string;
}


DoSomeThing2::DoSomeThing2(PINDEX index)
  : PThread(1000, AutoDeleteThread, NormalPriority, psprintf("DoSomeThing2 %u", index))
  , m_index(index)
{
  Resume();
}


void DoSomeThing2::Main()
{
  PRandom rand(PRandom::Number());

  PINDEX i;

  for (i = 0; i < 5000; i++) {
    PSafeString * p = new PSafeString(rand.Generate());
    list.Append(p);
//    coutMutex.Wait();
//    cout << GetThreadName() << ": Added " << *p << " element to sorted list" << endl;
//    coutMutex.Signal();
  }

  for (;;) {

    PINDEX remove = rand.Generate() % (list.GetSize() + 1);
    for (i = 0; i < remove; i++) {
      PINDEX index = rand.Generate() % list.GetSize();
      coutMutex.Wait();
      PSafePtr<PSafeString> str = list.GetAt(index, PSafeReference);
      cout << GetThreadName() << ": Removing element " << *str << " at index position " << index << endl;
      coutMutex.Signal();
      list.Remove(&(*str));
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


