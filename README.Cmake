Building PTLib with CMake
========================

CMake builds PTLib on macOS, Windows, Linux, MinGW-w64, Android, and iOS.
The library target is ptlib, and consumers use the imported target PTLib::ptlib.
On macOS and Linux the shared library is libpt, a debug build is libpt_d, and a
static build adds _s (libpt_s or libpt_d_s). 64-bit MSVC installs ptlib64
(ptlib64d for Debug); a static build is ptlib64s and ptlib64sd. Other Windows
toolchains keep pt.dll.

Requirements:
  CMake 3.20 or newer
  a C++20 compiler
  a C11 compiler

The default build is a shared Release library. Optional libraries are used
when CMake finds them. A missing optional library turns that feature off.
Set the matching option to ON to make a missing library a configure error,
or OFF to leave the feature out. Names are PTLIB_SSL, PTLIB_EXPAT,
PTLIB_SDL, PTLIB_FFMPEG, and the other entries printed by
"cmake -LH build".


Common commands
---------------

From the top of the PTLib tree:

  cmake -S . -B build
  cmake --build build
  cmake --install build --prefix /usr/local

Remove the files from that install with:

  cmake --build build --target uninstall

Use the same privileges as the install. A prefix such as /usr/local needs
sudo on both commands. uninstall reads install_manifest.txt in the build
tree, so it removes the last prefix this tree was installed to. Empty
directories created for those files are removed. The prefix itself is left
in place.

Useful options, passed on the first cmake command:

  -DBUILD_SHARED_LIBS=OFF       static library
  -DPTLIB_BUILD_SAMPLES=ON      sample programs
  -DPTLIB_MINSIZE=ON            smaller library (configure --enable-minsize)
  -DPTLIB_OPALMIN=ON            omit FTP, Telnet, LDAP, SOAP,
                                XML-RPC, SNMP, RFC1155, vCard, XMPP,
                                SASL, serial, and internet mail
  -DPTLIB_SSL=ON                require OpenSSL
  -DCMAKE_BUILD_TYPE=Debug      single-config generators (Ninja, Makefiles)

Single-config generators also accept:

  cmake --build build --target debug
  cmake --build build --target both

"debug" configures a second tree in build/debug. "both" builds Release and
Debug. Visual Studio and other multi-config generators use --config instead:

  cmake --build build --config Release
  cmake --build build --config Debug

Install produces:
  include/ptlib.h and include/ptlib_config.h
  the shared or static library
  lib/pkgconfig/ptlib.pc
  lib/cmake/PTLib/PTLibConfig.cmake

OPAL installs the same kind of files: lib/pkgconfig/opal.pc, which requires
ptlib, and lib/cmake/OPAL/OPALConfig.cmake.


Using PTLib from another project
--------------------------------

A CMake application uses the CMake package. PTLibConfig.cmake defines the
imported target PTLib::ptlib, and that target carries the public link
dependencies, including threads. The build directory is a CMake package
too, so a consumer can use PTLib before it is installed. Set PTLib_DIR to
the build directory or to an install prefix. A prefix already on
CMAKE_PREFIX_PATH is found with no extra variable.

  cmake_minimum_required(VERSION 3.20)
  project(myapp LANGUAGES CXX)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  find_package(PTLib CONFIG REQUIRED)
  add_executable(myapp main.cxx)
  target_link_libraries(myapp PRIVATE PTLib::ptlib)

Configure it against a build tree or an install prefix:

  cmake -S . -B build -DPTLib_DIR=<ptlib-build-or-prefix>

For an install under /usr/local, CMAKE_PREFIX_PATH=/usr/local is enough and
PTLib_DIR can be omitted. Compile as C++20.

OPAL's CMake package loads this one. OPAL_PTLIB_DIR accepts the PTLib build
directory or an install prefix:

  cmake -S opalvoip-opal -B opalvoip-opal/build -DOPAL_PTLIB_DIR=<ptlib>

A program that uses both libraries links both imported targets.
OPALConfig.cmake loads PTLib, so set PTLib_DIR when PTLib is outside the
default search path, and set OPAL_DIR the same way for OPAL.

  find_package(PTLib CONFIG REQUIRED)
  find_package(OPAL CONFIG REQUIRED)
  target_link_libraries(myapp PRIVATE OPAL::opal PTLib::ptlib)

A Linux application can also build a binary .deb of itself. install()
names the files CPack puts in the package, and include(CPack) adds the
package target. Put this after the executable is defined. Add VERSION to
the project() call, as in project(myapp VERSION 1.0.0), to set the package
version.

  include(GNUInstallDirs)
  install(TARGETS myapp
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "myapp")
    set(CPACK_PACKAGE_CONTACT "Name <email@example.com>")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "My application")
    set(CPACK_DEBIAN_PACKAGE_SECTION "comm")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
    include(CPack)
  endif()

CPACK_DEBIAN_PACKAGE_SHLIBDEPS records a dependency on the installed ptlib
package, and on opal when the program links it. Install dpkg-dev so cpack
can run dpkg-shlibdeps, and install the PTLib and OPAL .deb packages first
so those SONAMEs map to package names. Configure with
-DCMAKE_INSTALL_PREFIX=/usr, then:

  cmake --build build --target package

The .deb is written in the build directory.


pkg-config
----------

pkg-config reads ptlib.pc after install. The copy written into the build
directory already contains the install prefix, so it describes the
installed tree. If that prefix is outside the default search path:

  export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
  pkg-config --cflags --libs ptlib

That prints the include directory and the library name for this build, such
as -lpt. The C++ standard is recorded in the cxxflags and cplusplus_std
variables. pkg-config --cflags leaves -std=c++20 out, so the program still
compiles as C++20. Libs.private is empty, so pkg-config --static does not
add OpenSSL, libdl, or the other libraries a static link needs.

opal.pc requires ptlib, so pkg-config --cflags --libs opal follows that and
prints both libraries. Its Cflags line is empty. The -I comes from ptlib.pc,
which means both libraries are installed under the same prefix.


vcpkg
-----

vcpkg.json lists the optional libraries. The default features are openssl,
expat, sdl2, and ffmpeg.

On Windows, CMake uses a vcpkg checkout for those libraries and for the
other dependencies the build probes when the port supports the Visual
Studio triplet: Cyrus SASL, libpcap, Lua, and GStreamer. The toolchain
is loaded automatically unless one was already given or
-DPTLIB_USE_VCPKG=OFF is set. CMake looks at VCPKG_ROOT, then vcpkg on
PATH, then the Visual Studio vcpkg component, then a checkout under the
user profile, %LOCALAPPDATA%\vcpkg, C:\vcpkg, or C:\src\vcpkg.

An explicit -DVCPKG_MANIFEST_FEATURES list replaces that automatic set.
The default features above are still installed. An empty list keeps only
the defaults:

  -DVCPKG_MANIFEST_FEATURES=

OpenLDAP and ncurses are not selected for Visual Studio: those ports do
not build for that triplet. ALSA and PulseAudio are Linux-only. Windows
ODBC is the SDK library, so unixodbc is not selected. The AWS SDK,
PortAudio, and V8 stay opt-in. On Windows, V8 is left out of the build
until -DPTLIB_V8=ON. See the Windows section. PortAudio and the AWS SDK
are features:

  -DVCPKG_MANIFEST_FEATURES=aws-sdk,portaudio

On other systems, point CMake at a checkout:

  cmake -S . -B build "-DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake"

-DPTLIB_MINSIZE=ON skips the default openssl, expat, sdl2, and ffmpeg
features and does not add the automatic Windows set. That matches
configure --enable-minsize. Still-image files (BMP, JPEG, PNG) and
JPEG/MJPEG colour conversion use FFmpeg when it is enabled
(PTLIB_FFMPEG=AUTO uses it when the library is present).

Windows does not use libiconv; character conversion uses the Win32 APIs.


macOS
-----

Install Xcode or the Xcode command line tools, then CMake. Homebrew is the
usual source of optional libraries. A typical set:

  brew install cmake pkgconf openssl expat sdl2 ffmpeg

Other optional features, installed the same way:

  openldap          OpenLDAP (PTLIB_OPENLDAP; pkg-config name ldap)
  cyrus-sasl        Cyrus SASL (PTLIB_SASL; pkg-config name libsasl2)
  libpcap           packet capture (PTLIB_PCAP)
  lua               Lua (PTLIB_LUA)
  v8                V8 JavaScript (PTLIB_V8)
  unixodbc          ODBC (PTLIB_ODBC)
  ncurses           curses UI (PTLIB_CURSES)
  gstreamer         GStreamer (PTLIB_GSTREAMER)

openldap, cyrus-sasl, and libpcap are keg-only on Homebrew because macOS
already ships copies. Their .pc files are not on the default
PKG_CONFIG_PATH. For OpenLDAP and SASL, point pkg-config at the kegs
before configure (Apple silicon paths shown; on Intel use /usr/local):

  export PKG_CONFIG_PATH="/opt/homebrew/opt/openldap/lib/pkgconfig:/opt/homebrew/opt/cyrus-sasl/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"

libpcap and V8 are also found without pkg-config when the libraries are
present. Pass -DCMAKE_PREFIX_PATH=/opt/homebrew (or /usr/local on Intel)
so find_package and find_library see Homebrew.

Configure and build from the PTLib tree:

  cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew
  cmake --build build
  cmake --install build --prefix "$HOME/ptlib"

This selects the macOS sources and defines P_MACOSX. The library links the
Apple audio and video frameworks.


Windows
-------

Use Visual Studio 2022 or later, with the C++ desktop workload, and CMake.
From this directory, configure_vs.bat selects the newest installed Visual
Studio that has the C++ workload and writes build\PTLib.sln plus the
vcxproj files:

  configure_vs.bat

If VCPKG_ROOT is set, the script passes that vcpkg toolchain to CMake.
Otherwise CMake looks for a checkout itself. See the vcpkg section.
The same configure for Visual Studio 2022 from a command prompt:

  cmake -S . -B build -G "Visual Studio 17 2022" -A x64
  cmake --build build --config Release
  cmake --install build --config Release --prefix C:\ptlib

"cmake --help" lists the generator name for a newer Visual Studio. This
selects the Windows sources. The library links ws2_32 and iphlpapi.

Optional libraries come from vcpkg when that toolchain is found. See the
vcpkg section. Pass the toolchain file only when CMake should use a
checkout it would not find on its own:

  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

V8 JavaScript is off by default. There is no vcpkg port. -DPTLIB_V8=ON
turns it on and looks for a Windows V8 build the same way configure.exe
does. The match is the first directory that contains include\v8.h and
v8_monolith.lib under out.gn\x64.release or out.gn\x64.debug (ia32 when
the build is 32-bit). Relative to the PTLib tree, the search checks
%V8DIR%, ..\v8, ..\v8\v8, ..\external\v8, ..\..\external\v8,
..\external\v8\v8, ..\..\external\v8\v8, then directories on the INCLUDE
path. A fetch v8 tree at C:\Work\external\v8 is found when PTLib is in
C:\Work\ptlib, because include\v8.h lives in external\v8\v8.

If that search finds nothing and -DPTLIB_V8_DIR is not set, V8 stays out
of the build. -DPTLIB_V8_DIR enables V8 on its own and points at a build
that is somewhere else: the directory that contains include\v8.h, or the
fetch root above the inner v8 directory. A path that does not contain
those files is an error. -DPTLIB_V8=AUTO uses the same search.
-DPTLIB_V8=OFF leaves V8 out even when a build is present. OPAL uses the
V8 support compiled into this PTLib; it does not search for V8 again.

Multi-config builds pass --config Release or --config Debug on both the
build and the install command. Debug and Release can link different
v8_monolith.lib files when both were built.


Linux
-----

Install CMake, a C++20 compiler, pkg-config, and the development packages
for the features you want. On Debian and Ubuntu a typical base set is:

  sudo apt install build-essential cmake pkg-config \
    libssl-dev libexpat1-dev libsdl2-dev

Other optional features:

  libavcodec-dev libavutil-dev libavformat-dev \
    libswscale-dev libswresample-dev   FFmpeg (PTLIB_FFMPEG)
  libldap2-dev                         OpenLDAP (PTLIB_OPENLDAP)
  libsasl2-dev                         Cyrus SASL (PTLIB_SASL)
  libpcap-dev                          packet capture (PTLIB_PCAP)
  liblua5.4-dev                        Lua (PTLIB_LUA; liblua5.3-dev also works)
  libnode-dev                          V8 JavaScript (PTLIB_V8; older
                                       releases may use libv8-dev)
  unixodbc-dev                         ODBC (PTLIB_ODBC)
  libncurses-dev                       curses UI (PTLIB_CURSES)
  libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev   GStreamer (PTLIB_GSTREAMER)
  libasound2-dev                       ALSA plugin
  libpulse-dev                         PulseAudio plugin (PTLIB_PULSE)
  libv4l-dev                           Video4Linux2 plugin

  cmake -S . -B build
  cmake --build build
  sudo cmake --install build --prefix /usr/local

This defines P_LINUX. ALSA and Video4Linux2 plugins are probed
automatically. PulseAudio stays off unless -DPTLIB_PULSE=ON or
-DPTLIB_PULSE=AUTO finds it.


Debian packages
---------------

A Linux configure can also build one binary .deb. It contains the same
files as cmake --install: the shared library, plugins, headers, ptlib.pc,
and the CMake package files. There is no separate development package and
no source package.

Install dpkg-dev so cpack can run dpkg-shlibdeps. Configure with a /usr
prefix so the package installs libraries where Debian's ldconfig trigger
sees them. A normal /usr/local install does not need this.

  cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
  cmake --build build
  cmake --build build --target package

The package target writes ptlib_<version>_<arch>.deb in the build
directory. "cpack -G DEB" from that directory does the same thing. The
Debian version uses ~alpha or ~beta in place of the hyphen in the PTLib
version, so 4.0.0-alpha is packaged as 4.0.0~alpha.

Depends lists the shared libraries this build actually linked. Install
the package with:

  sudo dpkg -i build/ptlib_*.deb


MinGW-w64
---------

Use the 64-bit MinGW-w64 toolchain. The build defines P_MINGW and compiles
the Windows sources. Plugins are installed into a plugins directory next to
the process.

From an MSYS2 mingw64 shell, where gcc is already x86_64-w64-mingw32-gcc:

  cmake -S . -B build -G Ninja
  cmake --build build
  cmake --install build --prefix /mingw64

Cross compiling from Linux:

  cmake -S . -B build-mingw \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32
  cmake --build build-mingw

CMAKE_FIND_ROOT_PATH is the prefix that holds the MinGW builds of OpenSSL
and the other optional libraries. CMake will not use the host Linux copies.

Cross compiling from macOS, after "brew install mingw-w64". Delete
build-mingw first if a previous configure already cached the host ar.
CMake otherwise picks /usr/bin/ar, which cannot read the response files
it writes for a Windows link, and the compiler test fails.

  cmake -S . -B build-mingw \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
    -DCMAKE_C_COMPILER=/opt/homebrew/bin/x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/x86_64-w64-mingw32-g++ \
    -DCMAKE_RC_COMPILER=/opt/homebrew/bin/x86_64-w64-mingw32-windres \
    -DCMAKE_AR=/opt/homebrew/bin/x86_64-w64-mingw32-ar \
    -DCMAKE_RANLIB=/opt/homebrew/bin/x86_64-w64-mingw32-ranlib
  cmake --build build-mingw

On an Intel Mac the Homebrew prefix is /usr/local instead of /opt/homebrew.
CMAKE_AR and CMAKE_RANLIB must be set on this first configure.


Android
-------

Use the Android NDK toolchain file. It sets CMAKE_SYSTEM_NAME to Android,
which selects the Android sources and defines P_ANDROID. Audio uses OpenSL
ES. A static library is the usual choice for an app.

  cmake -S . -B build-android \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DBUILD_SHARED_LIBS=OFF
  cmake --build build-android

ANDROID_ABI is one of arm64-v8a, armeabi-v7a, x86_64, or x86. Repeat the
configure and build once per ABI. Desktop packages on the host are not used.
Pass -DPTLIB_SSL=OFF and the same for other features you do not have an
Android build of, or put those libraries on CMAKE_PREFIX_PATH and
CMAKE_FIND_ROOT_PATH.


iOS
---

Configure with CMake's iOS system name. This requires Xcode and defines
P_IOS. The library links the same Apple audio and video frameworks as the
macOS build.

  cmake -S . -B build-ios -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
    -DCMAKE_OSX_ARCHITECTURES=arm64
  cmake --build build-ios --config Release

The device SDK is the default. The simulator uses:

  -DCMAKE_OSX_SYSROOT=iphonesimulator

Host Homebrew libraries are not used for an iOS build. Disable features
you have not built for the device, for example -DPTLIB_SSL=OFF.
