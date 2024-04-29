//
// hello.cxx
//
// Equivalence Pty. Ltd.
//

#include <ptlib.h>
#include <ptlib/pprocess.h>

class Hello : public PProcess
{
  PCLASSINFO(Hello, PProcess)
  public:
    virtual void Main() override;
};

PCREATE_PROCESS(Hello)

void Hello::Main()
{
  MemoryUsage usage;
  GetMemoryUsage(usage);
  cout << "Hello world!\n\n"
          "From " << GetOSClass() << ' ' << GetOSName() << " (" << GetOSVersion() << ")"
          " on " << GetOSHardware() << ", PTLib version " << GetLibVersion() << "\n\n"
          "Memory total    : " << PScaleSI(usage.m_total,     4, "B") << "\n"
          "Memory available: " << PScaleSI(usage.m_available, 4, "B") << "\n"
          "Virtual used    : " << PScaleSI(usage.m_virtual,   4, "B") << "\n"
          "Resident used   : " << PScaleSI(usage.m_resident,  4, "B") << "\n"
       << endl;
}

// End of hello.cxx
