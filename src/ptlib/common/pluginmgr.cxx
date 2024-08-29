/*
 * pluginmgr.cxx
 *
 * Plugin Manager Class
 *
 * Search path for PLugin is defined in this class, using the following 
 * define variables: P_DEFAULT_PLUGIN_DIR
 *
 * Also the following Environment variables:  "PTLIBPLUGINDIR" and "PWLIBPLUGINDIR"
 * It check 1st if PTLIBPLUGINDIR is defined, 
 * if not then check PWLIBPLUGINDIR is defined. 
 * If not use P_DEFAULT_PLUGIN_DIR (hard coded as DEFINE).
 *
 * Plugin must have suffixes:
 * #define PTPLUGIN_SUFFIX       "_ptplugin"
 * #define PWPLUGIN_SUFFIX       "_pwplugin"
 * 
 * Debugging plugin issue, set Trace level based on the following:
 * PTRACE
 * level 2 = list plugin that are not compatible (old version, not a PWLIB plugin etc).
 * level 4 = list directories.
 * level 5 = list plugin before checking suffix .
 *  * Portable Tools Library
 *
 * Contributor(s): Snark at GnomeMeeting
 */

#include <ptlib.h>
#include <ptlib/pluginmgr.h>

#if P_PLUGINMGR

#include <ptlib/pprocess.h>
#include <algorithm>


#ifndef P_DEFAULT_PLUGIN_DIR
#  if defined (_WIN32)
#    if _WIN64
#      define P_DEFAULT_PLUGIN_DIR ".;C:\\Program Files\\PTLib Plug Ins;C:\\PTLIB_PLUGINS;C:\\PWLIB_PLUGINS"
#    else
#      define P_DEFAULT_PLUGIN_DIR ".;C:\\Program Files (x86)\\PTLib Plug Ins;C:\\Program Files\\PTLib Plug Ins;C:\\PTLIB_PLUGINS;C:\\PWLIB_PLUGINS"
#    endif
#  elif defined (P_ANDROID)
#    define P_DEFAULT_PLUGIN_DIR ""
#  else
#    define P_DEFAULT_PLUGIN_DIR ".:/usr/lib/ptlib:/usr/lib/pwlib"
#  endif
#endif

#define PTPLUGIN_SUFFIX       "_ptplugin"
#define PWPLUGIN_SUFFIX       "_pwplugin"

const char PPluginServiceDescriptor::SeparatorChar = '\t';


class PluginLoaderStartup : public PProcessStartup
{
  PCLASSINFO(PluginLoaderStartup, PProcessStartup);
  public:
    void OnStartup() override;
    void OnShutdown() override;
};


#define new PNEW


////////////////////////////////////////////////////////////////////////////////////

bool PPluginServiceDescriptor::ValidateServiceName(const PString & name, intptr_t) const
{
  return GetServiceName() == name;
}


bool PPluginDeviceDescriptor::ValidateServiceName(const PString & name, intptr_t userData) const
{
  return ValidateDeviceName(name, userData);
}


PStringArray PPluginDeviceDescriptor::GetDeviceNames(intptr_t) const
{
  return GetServiceName();
}


bool PPluginDeviceDescriptor::ValidateDeviceName(const PString & name, intptr_t userData) const
{
  PStringArray devices = GetDeviceNames(userData);
  if (name.GetLength() == 2 && name[0] == '#' && isdigit(name[1]) && (PINDEX)(name[1]-'0') < devices.GetSize())
    return true;

  for (PINDEX i = 0; i < devices.GetSize(); i++) {
    if (devices[i] *= name)
      return true;
  }

  return false;
}


bool PPluginDeviceDescriptor::GetDeviceCapabilities(const PString & /*deviceName*/, void * /*capabilities*/) const
{
  return false;
}


//////////////////////////////////////////////////////

PPluginManager::PPluginManager()
{
  m_suffixes.push_back(PTPLUGIN_SUFFIX);
  m_suffixes.push_back(PWPLUGIN_SUFFIX);
}


PPluginManager & PPluginManager::GetPluginManager()
{
  static PPluginManager systemPluginMgr;
  return systemPluginMgr;
}


void PPluginManager::SetDirectories(const PString & dirs)
{
  SetDirectories(dirs.Tokenise(PPATH_SEPARATOR, false)); // split into directories on correct seperator
}


void PPluginManager::SetDirectories(const PStringArray & dirs)
{
  m_directories.clear();
  for (size_t i = 0; i < dirs.size(); ++i)
    AddDirectory(dirs[i]);
}


void PPluginManager::AddDirectory(const PDirectory & dir)
{
  PTRACE(4, "PLUGIN\tAdding directory \"" << dir << '"');
  if (std::find(m_directories.begin(), m_directories.end(), dir) == m_directories.end())
    m_directories.emplace_back(std::move(dir));
}


void PPluginManager::LoadDirectories()
{
  PTRACE(4, "PLUGIN\tEnumerating plugin directories "
         << setfill(PPATH_SEPARATOR) << PPrintValues(m_directories));
  for (const auto & it : m_directories)
    LoadDirectory(it);
}


void PPluginManager::LoadDirectory(const PDirectory & directory)
{
  PDirectory dir = directory;
  if (!dir.Open()) {
    PTRACE(4, "PLUGIN\tCannot open plugin directory " << dir);
    return;
  }

  PTRACE(3, "PLUGIN", "Enumerating plugin directory " << dir
         << (directory.IsRoot() ? " - not recursing" : ""));

  do {
    PString entry = dir + dir.GetEntryName();
    PDirectory subdir = entry;
    if (subdir.Open()) {
      if (!directory.IsRoot())
        LoadDirectory(entry);
    }
    else {
      PFilePath fn(entry);
      for (const PString & suffix : m_suffixes) {
        PTRACE(5, "PLUGIN\tChecking " << fn << " against suffix " << suffix);
        if ((fn.GetType() *= PDynaLink::GetExtension()) && (fn.GetTitle().Right(strlen(suffix)) *= suffix)) 
          LoadPlugin(entry);
      }
    }
  } while (dir.Next());
}


bool PPluginManager::LoadPlugin(const PString & fileName)
{
  PDynaLink dll(fileName);
  if (!dll.IsLoaded()) {
    PTRACE(4, "PLUGIN\tFailed to open " << fileName << " error: " << dll.GetLastError());
  }

  else {
    PDynaLink::Function fn;
    if (!dll.GetFunction("PWLibPlugin_GetAPIVersion", fn))
      PTRACE(2, "PLUGIN\t" << fileName << " is not a PWLib plugin");

    else {
      unsigned (*GetAPIVersion)() = (unsigned (*)())fn;
      int version = (*GetAPIVersion)();
      PTRACE(5, "PLUGIN\t" << fileName << " API version " << version);
      switch (version) {
        case 0 : // old-style service plugins, and old-style codec plugins
          {
            // call the register function (if present)
            if (!dll.GetFunction("PWLibPlugin_TriggerRegister", fn)) 
              PTRACE(2, "PLUGIN\t" << fileName << " has no registration-trigger function");
            else {
              void (*triggerRegister)(PPluginManager *) = (void (*)(PPluginManager *))fn;
              (*triggerRegister)(this);
              PTRACE(4, "PLUGIN\t" << fileName << " has been triggered");
            }
          }
          // fall through to new version

        case 1 : // factory style plugins
          // call the notifier
          CallNotifier(dll, LoadingPlugIn);

          // add the plugin to the list of plugins
          m_pluginsMutex.Wait();
          m_plugins.emplace_back(std::move(dll));
          m_pluginsMutex.Signal();
          return true;

        default:
          PTRACE(2, "PLUGIN\t" << fileName << " uses version " << version << " of the PWLIB PLUGIN API, which is not supported");
          break;
      }
    }
  }

  return false;
}


PStringArray PPluginManager::GetServiceTypes() const
{
  PWaitAndSignal mutex(m_servicesMutex);

  PStringSet distinct;
  for (ServiceMap::const_iterator it = m_services.begin(); it != m_services.end(); ++it)
    distinct += it->first;

  PINDEX count = 0;
  PStringArray result(distinct.GetSize());
  for (PStringSet::iterator it = distinct.begin(); it != distinct.end(); ++it)
    result[count++] = *it;

  return result;
}


PStringArray PPluginManager::GetPluginsProviding(const PString & serviceType, bool friendlyName) const
{
  PWaitAndSignal mutex(m_servicesMutex);

  PStringArray result;
  for (ServiceMap::const_iterator it = m_services.find(serviceType); it != m_services.end() && it->first == serviceType; ++it)
    result.AppendString(friendlyName ? it->second->GetFriendlyName() : it->second->GetServiceName());

  return result;
}


const PPluginServiceDescriptor * PPluginManager::GetServiceDescriptor(const PString & serviceName,
                                                                      const PString & serviceType) const
{
  PWaitAndSignal mutex(m_servicesMutex);

  for (ServiceMap::const_iterator it = m_services.find(serviceType); it != m_services.end() && it->first == serviceType; ++it) {
    if (it->second->GetServiceName() == serviceName)
      return it->second;
  }
  return NULL;
}


PObject * PPluginManager::CreatePlugin(const PString & serviceName,
                                       const PString & serviceType,
                                       intptr_t userData) const
{
  PWaitAndSignal mutex(m_servicesMutex);

  {
    // If have tab character, then have explicit driver name in device
    PINDEX tab = serviceName.Find(PPluginServiceDescriptor::SeparatorChar);
    const PPluginServiceDescriptor * descriptor = GetServiceDescriptor(tab != P_MAX_INDEX ? serviceName.Left(tab) : serviceName, serviceType);
    if (descriptor != NULL)
      return descriptor->CreateInstance(userData);
  }

  for (ServiceMap::const_iterator it = m_services.find(serviceType); it != m_services.end() && it->first == serviceType; ++it) {
    if (it->second->ValidateServiceName(serviceName, userData))
      return it->second->CreateInstance(userData);
  }

  return NULL;
}


PStringArray PPluginManager::GetPluginDeviceNames(const PString & serviceName,
                                                  const PString & serviceType,
                                                  intptr_t userData,
                                                  const char * const * prioritisedDrivers) const
{
  PWaitAndSignal mutex(m_servicesMutex);

  if (!serviceName.IsEmpty() && serviceName.Find('*') == P_MAX_INDEX) {
    const PPluginDeviceDescriptor * descriptor = dynamic_cast<const PPluginDeviceDescriptor *>(GetServiceDescriptor(serviceName, serviceType));
    return descriptor != NULL ? descriptor->GetDeviceNames(userData) : PStringArray();
  }

  PStringToString deviceToPluginMap;
  bool alwaysShowDriver = serviceName.Find(PPluginServiceDescriptor::SeparatorChar) != P_MAX_INDEX;

  // First we run through all of the drivers and their lists of devices and
  // use the dictionary to assure all names are unique
  for (ServiceMap::const_iterator it = m_services.find(serviceType); it != m_services.end() && it->first == serviceType; ++it) {
    const PPluginDeviceDescriptor * descriptor = dynamic_cast<const PPluginDeviceDescriptor *>(it->second);
    if (descriptor != NULL) {
      PCaselessString driver = descriptor->GetServiceName();
      PStringArray devices = descriptor->GetDeviceNames(userData);
      for (PINDEX j = 0; j < devices.GetSize(); j++) {
        PCaselessString device = devices[j];
        if (alwaysShowDriver)
            deviceToPluginMap.SetAt(driver+PPluginServiceDescriptor::SeparatorChar+device, driver);
        else if (deviceToPluginMap.Contains(device)) {
          PString oldDriver = deviceToPluginMap[device];
          if (!oldDriver.IsEmpty()) {
            // Make name unique by prepending driver name and a tab character
            deviceToPluginMap.SetAt(oldDriver+PPluginServiceDescriptor::SeparatorChar+device, driver);
            // Reset the original to empty string so we dont add it multiple times
            deviceToPluginMap.SetAt(device, "");
          }
          // Now add the new one
          deviceToPluginMap.SetAt(driver+PPluginServiceDescriptor::SeparatorChar+device, driver);
        }
        else
          deviceToPluginMap.SetAt(device, driver);
      }
    }
  }

  PStringArray allDevices;

  if (prioritisedDrivers != NULL) {
    while (*prioritisedDrivers != NULL) {
      for (PStringToString::iterator it = deviceToPluginMap.begin(); it != deviceToPluginMap.end(); ++it) {
        if (it->second == *prioritisedDrivers)
          allDevices.AppendString(it->first);
      }
      ++prioritisedDrivers;
    }
  }

  for (PStringToString::iterator it = deviceToPluginMap.begin(); it != deviceToPluginMap.end(); ++it) {
    if (!it->second.IsEmpty() && allDevices.GetValuesIndex(it->first) == P_MAX_INDEX)
      allDevices.AppendString(it->first);
  }

  return allDevices;
}


bool PPluginManager::GetPluginsDeviceCapabilities(const PString & serviceType,
                                                      const PString & serviceName,
                                                      const PString & deviceName,
                                                      void * capabilities) const
{
  if (serviceType.IsEmpty() || deviceName.IsEmpty()) 
    return false;

  if (serviceName.IsEmpty() || serviceName == "*") {
    PWaitAndSignal mutex(m_servicesMutex);
    for (ServiceMap::const_iterator it = m_services.find(serviceType); it != m_services.end() && it->first == serviceType; ++it) {
      const PPluginDeviceDescriptor * desc = dynamic_cast<const PPluginDeviceDescriptor *>(it->second);
      if (desc != NULL && desc->ValidateDeviceName(deviceName, 0))
        return desc->GetDeviceCapabilities(deviceName,capabilities);
    }
  }
  else {
    const PPluginDeviceDescriptor * desc = dynamic_cast<const PPluginDeviceDescriptor *>(GetServiceDescriptor(serviceName, serviceType));
    if (desc != NULL && desc->ValidateDeviceName(deviceName, 0))
      return desc->GetDeviceCapabilities(deviceName,capabilities);
  }

  return false;
}


bool PPluginManager::RegisterService(const char * name)
{
  PTRACE(5, "PLUGIN\tRegistering \"" << name << '"');
  PPluginServiceDescriptor * descriptor = PPluginFactory::CreateInstance(name);
  if (descriptor == NULL) {
    PTRACE(3, "PLUGIN\tCould not create factory instance for \"" << name << '"');
    return false;
  }

  PWaitAndSignal mutex(m_servicesMutex);

  // first, check if it something didn't already register that name and type
  if (GetServiceDescriptor(descriptor->GetServiceName(), descriptor->GetServiceType()) != NULL) {
    PTRACE(3, "PLUGIN\tDuplicate \"" << name << '"');
    return false;
  }

  m_services.insert(ServiceMap::value_type(descriptor->GetServiceType(), descriptor));

  return true;
}


void PPluginManager::OnShutdown()
{
  PWaitAndSignal mutex(m_pluginsMutex);

  for (auto & it : m_plugins)
    CallNotifier(it, UnloadingPlugIn);

  m_notifiersMutex.Wait();
  m_notifiers.RemoveAll();
  m_notifiersMutex.Signal();

  m_services.clear();

  m_plugins.clear();
}


void PPluginManager::AddNotifier(const PNotifier & notifyFunction, bool existing)
{
  m_notifiersMutex.Wait();
  m_notifiers.Add(notifyFunction);
  m_notifiersMutex.Signal();

  if (existing) {
    PWaitAndSignal mutex(m_pluginsMutex);
    for (auto & it : m_plugins)
      CallNotifier(it, LoadingPlugIn);
  }
}


void PPluginManager::RemoveNotifier(const PNotifier & notifyFunction)
{
  m_notifiersMutex.Wait();
  m_notifiers.Remove(notifyFunction);
  m_notifiersMutex.Signal();
}


void PPluginManager::CallNotifier(PDynaLink & dll, NotificationCode code)
{
  PWaitAndSignal mutex(m_notifiersMutex);
  m_notifiers(dll, code);
}


////////////////////////////////////////////////////////////////////////////////////

PPluginModuleManager::PPluginModuleManager(const char * signatureFunctionName, PPluginManager * pluginMgr)
  : m_signatureFunctionName(signatureFunctionName)
  , m_pluginMgr(pluginMgr != NULL ? *pluginMgr : PPluginManager::GetPluginManager())
{
}

void PPluginModuleManager::OnLoadModule(PDynaLink & dll, intptr_t code)
{
  PDynaLink::Function dummyFunction;
  if (!dll.GetFunction(m_signatureFunctionName, dummyFunction))
    return;

  switch (code) {
    case 0:
      m_pluginDLLs.emplace(dll.GetName(), std::move(dll));
      break;

    case 1: 
      m_pluginDLLs.erase(dll.GetName());
      break;

    default:
      break;
  }

  OnLoadPlugin(dll, code);
}


////////////////////////////////////////////////////////////////////////////////////

void PluginLoaderStartup::OnStartup()
{
  PPluginManager & pluginMgr = PPluginManager::GetPluginManager();
  if (pluginMgr.GetDirectories().empty()) {
    PString env = PConfig::GetEnv(P_PTLIB_PLUGIN_DIR_ENV_VAR);
    if (env.IsEmpty())
      env = PConfig::GetEnv(P_PWLIB_PLUGIN_DIR_ENV_VAR);
    if (env.IsEmpty()) {
      env = P_DEFAULT_PLUGIN_DIR;
      PDirectory appdir = PProcess::Current().GetFile().GetDirectory();
      if (!appdir.IsRoot())
        env += PPATH_SEPARATOR + appdir;
    }
    pluginMgr.SetDirectories(env);
  }

  // load the plugin module managers
  for (const auto & it : PFactory<PPluginModuleManager>::GetKeyList())
    PFactory<PPluginModuleManager>::CreateInstance(it)->OnStartup();

  // load the actual DLLs, which will also load the system plugins
  pluginMgr.LoadDirectories();

  // load the static plug in services/devices
  for (const auto & it : PPluginFactory::GetKeyList())
    pluginMgr.RegisterService(it.c_str());
}


void PluginLoaderStartup::OnShutdown()
{
  PPluginManager::GetPluginManager().OnShutdown();

  // load the plugin module managers, and construct the list of suffixes
  for (const auto & it : PFactory<PPluginModuleManager>::GetKeyList())
    PFactory<PPluginModuleManager>::CreateInstance(it)->OnShutdown();
}

PFACTORY_CREATE(PProcessStartupFactory, PluginLoaderStartup, PLUGIN_LOADER_STARTUP_NAME, true);

#endif // P_PLUGINMGR
