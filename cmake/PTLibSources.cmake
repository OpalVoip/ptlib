# Source list transcribed from make/toplevel.mak, using the files that exist.
# msos is used for MSVC and MinGW. Every other kept host uses the unix tree.

function(ptlib_collect_sources out_var)
  set(root "${PTLIB_SOURCE_DIR}")
  set(common "${root}/src/ptlib/common")
  set(comp "${root}/src/ptclib")
  set(plat "${root}/src/ptlib/${PTLIB_PLATFORM_SOURCES}")
  set(msos FALSE)
  if(PTLIB_PLATFORM_SOURCES STREQUAL "msos")
    set(msos TRUE)
  endif()

  set(sources
    "${comp}/pxml.cxx"
    "${comp}/json.cxx"
    "${comp}/aws_sdk.cxx"
    "${comp}/textdata.cxx"
    "${comp}/threadpool.cxx"
    "${comp}/random.cxx"
    "${common}/notifier_ext.cxx"
    "${common}/safecoll.cxx"
    "${common}/ptime.cxx"
    "${common}/osutils.cxx"
    "${plat}/assert.cxx"
    "${common}/collect.cxx"
    "${common}/contain.cxx"
    "${common}/object.cxx")

  if(PTLIB_GETDATE_SOURCE)
    list(APPEND sources "${PTLIB_GETDATE_SOURCE}")
  endif()

  if(msos)
    list(APPEND sources
      "${plat}/ptlib.cxx"
      "${plat}/win32.cxx"
      "${plat}/dllmain.cxx"
      "${common}/pchannel.cxx")
  else()
    list(APPEND sources
      "${plat}/udll.cxx"
      "${plat}/channel.cxx"
      "${plat}/osutil.cxx"
      "${plat}/tlib.cxx")
  endif()

  if(PTLIB_SASL_ENABLED)
    list(APPEND sources "${comp}/psasl.cxx")
  endif()
  if(PTLIB_OPENLDAP_ENABLED)
    list(APPEND sources "${comp}/pldap.cxx" "${comp}/pils.cxx")
  endif()
  if(PTLIB_SSL_ENABLED)
    list(APPEND sources "${comp}/pssl.cxx")
  endif()
  if(PTLIB_SDL_ENABLED)
    list(APPEND sources "${comp}/vsdl.cxx")
  endif()
  if(PTLIB_GSTREAMER_ENABLED)
    list(APPEND sources "${comp}/gstreamer.cxx")
  endif()
  if(PTLIB_ODBC_ENABLED)
    list(APPEND sources "${comp}/podbc.cxx")
  endif()

  if(PTLIB_VIDEO)
    list(APPEND sources
      "${common}/vfakeio.cxx"
      "${common}/videoio.cxx"
      "${common}/vconvert.cxx"
      "${common}/tinyjpeg.cxx"
      "${common}/jidctflt.cxx")
    if(PTLIB_SHM_VIDEO AND NOT msos)
      list(APPEND sources "${plat}/shmvideo.cxx")
    endif()
    if(msos)
      if(EXISTS "${plat}/vfw.cxx")
        list(APPEND sources "${plat}/vfw.cxx")
      endif()
      if(EXISTS "${plat}/directshow.cxx")
        list(APPEND sources "${plat}/directshow.cxx")
      endif()
    endif()
    if(PTLIB_OS_TAG STREQUAL "P_MACOSX")
      list(APPEND sources "${plat}/macvidcap.mm")
    endif()
  endif()

  if(PTLIB_AUDIO)
    list(APPEND sources "${common}/sound.cxx")
    if(msos)
      list(APPEND sources "${plat}/sound_win32.cxx")
      if(EXISTS "${plat}/directsound.cxx")
        list(APPEND sources "${plat}/directsound.cxx")
      endif()
    endif()
    if(PTLIB_PORTAUDIO_ENABLED)
      list(APPEND sources "${comp}/portaudio.cxx")
    endif()
    if(PTLIB_IS_APPLE)
      list(APPEND sources "${plat}/macaudio.mm")
    endif()
    if(PTLIB_OS_TAG STREQUAL "P_ANDROID")
      list(APPEND sources "${root}/src/ptlib/unix/opensl_es.cxx")
    endif()
  endif()

  if(PTLIB_EXPAT_ENABLED)
    if(PTLIB_XMLRPC)
      list(APPEND sources "${comp}/pxmlrpc.cxx" "${comp}/pxmlrpcs.cxx")
    endif()
    if(PTLIB_SOAP)
      list(APPEND sources "${comp}/psoap.cxx")
    endif()
    if(PTLIB_VXML)
      list(APPEND sources "${comp}/vxml.cxx")
    endif()
    if(PTLIB_JABBER AND PTLIB_SASL_ENABLED)
      list(APPEND sources
        "${comp}/xmpp.cxx"
        "${comp}/xmpp_c2s.cxx"
        "${comp}/xmpp_muc.cxx"
        "${comp}/xmpp_roster.cxx")
    endif()
  endif()

  if(PTLIB_LUA_ENABLED)
    list(APPEND sources "${comp}/lua.cxx")
  endif()
  if(PTLIB_V8_ENABLED)
    list(APPEND sources "${comp}/jscript.cxx")
  endif()
  if(PTLIB_RESOLVER_ENABLED)
    list(APPEND sources "${comp}/pdns.cxx" "${comp}/enum.cxx")
  endif()

  if(PTLIB_TEXT_TO_SPEECH OR PTLIB_SPEECH_RECOGNITION)
    list(APPEND sources "${common}/speech.cxx")
    if(msos)
      list(APPEND sources "${plat}/speech_sapi.cxx")
    endif()
    if(PTLIB_AWS_SDK_ENABLED)
      list(APPEND sources "${comp}/speech_aws.cxx")
    endif()
    if(PTLIB_SSL_ENABLED AND PTLIB_SPEECH_RECOGNITION)
      list(APPEND sources "${comp}/speech_deep.cxx")
    endif()
    if(PTLIB_SSL_ENABLED AND PTLIB_TEXT_TO_SPEECH)
      list(APPEND sources "${comp}/speech_11labs.cxx")
    endif()
  endif()
  if(PTLIB_TEXT_TO_SPEECH AND NOT msos AND PTLIB_FESTIVAL_ENABLED)
    list(APPEND sources "${plat}/speech_festival.cxx")
  endif()

  if(PTLIB_ASN)
    list(APPEND sources "${comp}/asner.cxx" "${comp}/pasn.cxx")
  endif()
  if(PTLIB_SNMP)
    list(APPEND sources
      "${comp}/snmpclnt.cxx"
      "${comp}/snmpserv.cxx"
      "${comp}/psnmp.cxx"
      "${comp}/snmp.cxx"
      "${comp}/rfc1155.cxx")
  endif()
  if(PTLIB_FTP)
    list(APPEND sources "${comp}/ftpclnt.cxx" "${comp}/ftpsrvr.cxx" "${comp}/ftp.cxx")
  endif()
  if(PTLIB_TELNET)
    list(APPEND sources "${comp}/telnet.cxx")
  endif()
  if(PTLIB_CLI)
    list(APPEND sources "${comp}/cli.cxx")
  endif()
  if(PTLIB_NAT)
    list(APPEND sources "${comp}/pnat.cxx")
  endif()
  if(PTLIB_STUN)
    list(APPEND sources "${comp}/pstun.cxx")
  endif()
  if(PTLIB_STUNSRVR)
    list(APPEND sources "${comp}/pstunsrvr.cxx")
  endif()
  if(PTLIB_SOCKS)
    list(APPEND sources "${comp}/socks.cxx")
  endif()
  if(PTLIB_PIPECHAN)
    if(msos)
      list(APPEND sources "${plat}/pipe.cxx" "${common}/pipechan.cxx")
    else()
      list(APPEND sources "${plat}/pipechan.cxx")
    endif()
  endif()
  if(PTLIB_REMCONN)
    list(APPEND sources "${plat}/remconn.cxx")
  endif()
  if(PTLIB_WAVFILE)
    list(APPEND sources "${comp}/pwavfile.cxx" "${comp}/pwavfiledev.cxx")
  endif()
  if(PTLIB_DTMF)
    list(APPEND sources "${comp}/dtmf.cxx" "${comp}/tonedev.cxx")
  endif()
  if(PTLIB_VCARD)
    list(APPEND sources "${comp}/vcard.cxx")
  endif()
  if(PTLIB_SERIAL)
    if(msos)
      list(APPEND sources "${plat}/winserial.cxx" "${common}/serial.cxx")
    else()
      list(APPEND sources "${plat}/serchan.cxx")
    endif()
    list(APPEND sources "${comp}/modem.cxx")
  endif()
  if(PTLIB_POP3SMTP)
    list(APPEND sources "${comp}/inetmail.cxx")
  endif()
  if(PTLIB_URL)
    list(APPEND sources "${comp}/url.cxx")
  endif()
  if(PTLIB_HTTP)
    list(APPEND sources
      "${comp}/http.cxx"
      "${comp}/httpclnt.cxx"
      "${comp}/html.cxx"
      "${comp}/httpsrvr.cxx")
    if(PTLIB_SSDP)
      list(APPEND sources "${comp}/ssdp.cxx")
    endif()
  endif()
  if(PTLIB_HTTPFORMS)
    list(APPEND sources "${comp}/httpform.cxx")
  endif()
  if(PTLIB_HTTPSVC)
    list(APPEND sources "${plat}/svcproc.cxx" "${comp}/httpsvc.cxx")
    if(PTLIB_SSL_ENABLED)
      list(APPEND sources "${comp}/shttpsvc.cxx")
    endif()
  endif()
  if(PTLIB_CONFIG_FILE)
    if(msos)
      list(APPEND sources "${plat}/wincfg.cxx" "${common}/pconfig.cxx")
    else()
      list(APPEND sources "${plat}/config.cxx")
    endif()
  endif()
  if(PTLIB_VIDFILE)
    list(APPEND sources "${comp}/pvidfile.cxx" "${comp}/pvfiledev.cxx")
  endif()
  if(PTLIB_MEDIAFILE)
    list(APPEND sources "${comp}/mediafile.cxx")
  endif()
  if(PTLIB_CYPHER)
    list(APPEND sources "${comp}/cypher.cxx")
  endif()
  if(PTLIB_VARTYPE)
    list(APPEND sources "${comp}/vartype.cxx")
  endif()
  if(PTLIB_GUID)
    list(APPEND sources "${comp}/guid.cxx")
  endif()
  if(PTLIB_SCRIPTS)
    list(APPEND sources "${comp}/script.cxx" "${comp}/simplescript.cxx")
  endif()
  if(PTLIB_SPOOLDIR)
    list(APPEND sources "${comp}/spooldir.cxx")
  endif()
  if(PTLIB_SYSTEMLOG)
    list(APPEND sources "${common}/syslog.cxx")
  endif()
  if(PTLIB_PLUGINMGR)
    list(APPEND sources "${common}/pluginmgr.cxx")
  endif()
  if(PTLIB_CHANNEL_UTILS)
    list(APPEND sources
      "${comp}/qchannel.cxx"
      "${comp}/delaychan.cxx"
      "${comp}/memfile.cxx")
  endif()
  if(PTLIB_NETWORKING)
    list(APPEND sources
      "${comp}/ipacl.cxx"
      "${comp}/inetprot.cxx"
      "${comp}/psockbun.cxx"
      "${common}/sockets.cxx")
    if(msos)
      list(APPEND sources
        "${plat}/icmp.cxx"
        "${plat}/winsock.cxx"
        "${common}/pethsock.cxx")
    else()
      list(APPEND sources "${plat}/uicmp.cxx" "${plat}/socket.cxx")
    endif()
  endif()

  if(NOT PTLIB_USE_SYSTEM_REGEX)
    list(APPEND sources
      "${common}/regex/regcomp.c"
      "${common}/regex/regexec.c"
      "${common}/regex/regerror.c"
      "${common}/regex/regfree.c")
  endif()

  set(${out_var} "${sources}" PARENT_SCOPE)
endfunction()
