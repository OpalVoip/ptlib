# add sasl support
#if (HAS_SASL)
#  set(sources
#    ${PTLIB_COMPONENT_SRC_DIR}/psasl.cxx
#  )
#endif()

# ldap support
if (${HAS_LDAP})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/pldap.cxx
    ${PTLIB_COMPONENT_SRC_DIR}/pils.cxx
  )
endif()

# ssl support
if (${HAS_SSL})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/pssl.cxx 
  )
endif()

# sdl support
if (${HAS_SDL})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/vsdl.cxx
  )
endif()

# gstreamer support
if (${HAS_GSTREAMER})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/gstreamer.cxx
  )
endif()

# odbc support
if (${HAS_ODBC})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/podbc.cxx
  )
endif()

# video support
if (${HAS_VIDEO})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMMON_SRC_DIR}/vfakeio.cxx 
    ${PTLIB_COMMON_SRC_DIR}/videoio.cxx 
    ${PTLIB_COMMON_SRC_DIR}/vconvert.cxx 
    ${PTLIB_COMMON_SRC_DIR}/pvidchan.cxx 
    ${PTLIB_COMMON_SRC_DIR}/tinyjpeg.cxx 
    ${PTLIB_COMMON_SRC_DIR}/jidctflt.cxx
  )

  if (${HAS_SHM_VIDEO})
    target_sources(ptlib PRIVATE
      ${PTLIB_PLATFORM_SRC_DIR}/shmvideo.cxx
    )
  endif()

  if (${HAS_VFW_CAPTURE})
    target_sources(ptlib PRIVATE
      ${PTLIB_PLATFORM_SRC_DIR}/vfw.cxx
    )
  endif()

  if (${HAS_DIRECTSHOW}) 
    target_sources(ptlib PRIVATE
      ${PTLIB_PLATFORM_SRC_DIR}/directshow.cxx
    )
  endif()

  if (${P_DARWIN})
    target_sources(ptlib PRIVATE
      ${PTLIB_PLATFORM_SRC_DIR}/macvidcap.mm
    )
  endif() # Darwin

endif() # HAS_VIDEO

# sound support
# Note this is mostly handled by the plugin system
if (${HAS_AUDIO})

  target_sources(ptlib PRIVATE
    ${PTLIB_COMMON_SRC_DIR}/sound.cxx 
  )

  if (${HAS_PORTAUDIO})
    target_sources(ptlib PRIVATE
      ${PTLIB_COMPONENT_SRC_DIR}/portaudio.cxx
    )
  endif()

  # TODO: add mac audio support
  #ifneq (,$(findstring $(target_os),Darwin iPhoneOS iPhoneSimulator))
  #  set(sources
  #    ${PTLIB_PLATFORM_SRC_DIR}/macaudio.mm
  #  )
  #endif() # Darwin

endif()  # HAS_AUDIO

# xml support 
if (${HAS_EXPAT})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/pxml.cxx 
  )
endif()

# xmlrpc support
if (${HAS_XMLRPC})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/pxmlrpc.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/pxmlrpcs.cxx 
  )

  if(${HAS_SOAP})
    target_sources(ptlib PRIVATE
      ${PTLIB_COMPONENT_SRC_DIR}/psoap.cxx 
    )
  endif()

  if (${HAS_VXML})
    target_sources(ptlib PRIVATE
      ${PTLIB_COMPONENT_SRC_DIR}/vxml.cxx 
    )
  endif()

  if (${HAS_SASL} OR ${HAS_SASL2})
    target_sources(ptlib PRIVATE
      ${PTLIB_COMPONENT_SRC_DIR}/xmpp.cxx 
      ${PTLIB_COMPONENT_SRC_DIR}/xmpp_c2s.cxx 
      ${PTLIB_COMPONENT_SRC_DIR}/xmpp_muc.cxx 
      ${PTLIB_COMPONENT_SRC_DIR}/xmpp_roster.cxx
    )
  endif()
endif() # HAS_EXPAT

if (${HAS_LUA})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/lua.cxx
  )
endif()

if (${HAS_V8})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/jscript.cxx
  )
endif()

if (${HAS_DNS_RESOLVER})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/pdns.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/enum.cxx 
  )
endif()

if (${HAS_AWS_SDK})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/aws_sdk.cxx 
  )
endif()

if (${HAS_TEXT_TO_SPEECH} OR ${HAS_SPEECH_RECOGNITION})
  target_sources(ptlib PRIVATE
    ${PTLIB_PLATFORM_SRC_DIR}/speech.cxx
    if (${HAS_AWS_SDK})
      ${PTLIB_COMPONENT_SRC_DIR}/speech_aws.cxx 
    endif
  )
endif()

if (${HAS_TEXT_TO_SPEECH})
  target_sources(ptlib PRIVATE
    ${PTLIB_PLATFORM_SRC_DIR}/speech_festival.cxx 
  )
endif()

if (${HAS_ASN})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/asner.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/pasn.cxx 
  )
endif()

if (${HAS_SNMP})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/snmpclnt.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/snmpserv.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/psnmp.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/snmp.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/rfc1155.cxx 
  )
endif()

if (${HAS_FTP})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/ftpclnt.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/ftpsrvr.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/ftp.cxx 
  )
endif()

if (${HAS_TELNET})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/telnet.cxx
  )
endif()

if (${HAS_CLI})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/cli.cxx
  )
endif()

if (${HAS_NAT})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/pnat.cxx
  )
endif()

if (${HAS_STUN})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/pstun.cxx
  )
endif()

if (${HAS_STUNSRVR})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/pstunsrvr.cxx
  )
endif()


if (${HAS_SOCKS})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/socks.cxx 
  )
endif()

if (${HAS_PIPECHAN})
  # TODO: add pipechan support for mingw
  #if (${target_os} STREQUAL "mingw")
  #  target_sources(ptlib PRIVATE
  #    ${PTLIB_PLATFORM_SRC_DIR}/pipe.cxx 
  #    ${PTLIB_COMMON_SRC_DIR}/pipechan.cxx
  #  )
  #else()
    target_sources(ptlib PRIVATE
      ${PTLIB_PLATFORM_SRC_DIR}/pipechan.cxx 
    )
  #endif()
endif()

if (${HAS_REMCONN})
  target_sources(ptlib PRIVATE
    ${PTLIB_PLATFORM_SRC_DIR}/remconn.cxx 
  )
endif()

if (${HAS_WAVFILE})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/pwavfile.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/pwavfiledev.cxx
  )
endif()

if (${HAS_DTMF})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/dtmf.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/tonedev.cxx 
  )
endif()

if (${HAS_VCARD})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/vcard.cxx 
  )
endif()

if (${HAS_SERIAL})
  # TODO: add serial support for mingw
  #if (${target_os} STREQUAL "mingw")
  #  target_sources(ptlib PRIVATE
  #    ${PTLIB_PLATFORM_SRC_DIR}/winserial.cxx 
  #    ${PTLIB_COMMON_SRC_DIR}/serial.cxx
  #  )
  #else()
    target_sources(ptlib PRIVATE
      ${PTLIB_PLATFORM_SRC_DIR}/serchan.cxx
    )
  #endif()
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/modem.cxx 
  )
endif()

if (${HAS_POP3SMTP})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/inetmail.cxx 
  )
endif()

if (${HAS_URL})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/url.cxx 
  )
endif()

if (${HAS_HTTP})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/http.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/httpclnt.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/html.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/httpsrvr.cxx
  )

  if (${HAS_SSDP})
    target_sources(ptlib PRIVATE
      ${PTLIB_COMPONENT_SRC_DIR}/ssdp.cxx
    )
  endif()
endif()

if (${HAS_HTTPFORMS})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/httpform.cxx
  )
endif()

if (${HAS_HTTPSVC})
  target_sources(ptlib PRIVATE
    ${PTLIB_PLATFORM_SRC_DIR}/svcproc.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/httpsvc.cxx
  )
  if (${HAS_SSL})
    target_sources(ptlib PRIVATE
      ${PTLIB_COMPONENT_SRC_DIR}/shttpsvc.cxx
    )
  endif()
endif()

if (${HAS_CONFIG_FILE})
  # TODO: add config file support for mingw
  #if (${target_os} STREQUAL "mingw")
  #  target_sources(ptlib PRIVATE
  #    ${PTLIB_PLATFORM_SRC_DIR}/wincfg.cxx 
  #    ${PTLIB_COMMON_SRC_DIR}/pconfig.cxx
  #  )
  #else()
    target_sources(ptlib PRIVATE
      ${PTLIB_PLATFORM_SRC_DIR}/config.cxx 
    )
  #endif()
endif()

if (${HAS_VIDFILE})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/pvidfile.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/pvfiledev.cxx 
  )
endif()

if (${HAS_MEDIAFILE})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/mediafile.cxx
  )
endif()

if (${HAS_CYPHER})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/cypher.cxx
  )
endif()

if (${HAS_VARTYPE})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/vartype.cxx
  )
endif()

if (${HAS_GUID})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/guid.cxx
  )
endif()

if (${HAS_SCRIPTS})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/script.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/simplescript.cxx
  )
endif()

if (${HAS_SPOOLDIR})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/spooldir.cxx
  )
endif()

if (${HAS_SYSTEMLOG})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/syslog.cxx
  )
endif()

if (${HAS_PLUGINMGR})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMMON_SRC_DIR}/pluginmgr.cxx
  )
endif()

if (${HAS_CHANNEL_UTILS})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/qchannel.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/delaychan.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/memfile.cxx
  )
endif()

if (${HAS_NETWORKING})
  target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/ipacl.cxx 
    ${PTLIB_COMPONENT_SRC_DIR}/inetprot.cxx 
    ${PTLIB_COMMON_SRC_DIR}/psockbun.cxx 
    ${PTLIB_COMMON_SRC_DIR}/sockets.cxx
    ${PTLIB_PLATFORM_SRC_DIR}/uicmp.cxx
    ${PTLIB_PLATFORM_SRC_DIR}/socket.cxx
  )
  # TODO: add networking support for mingw
  #if (${target_os} STREQUAL "mingw")
  #  target_sources(ptlib PRIVATE
  #    ${PTLIB_PLATFORM_SRC_DIR}/icmp.cxx 
  #    ${PTLIB_PLATFORM_SRC_DIR}/winsock.cxx 
  #    ${PTLIB_COMMON_SRC_DIR}/pethsock.cxx
  #  )
  #else()
    target_sources(ptlib PRIVATE
      ${PTLIB_PLATFORM_SRC_DIR}/uicmp.cxx 
      ${PTLIB_PLATFORM_SRC_DIR}/socket.cxx
    )
  #endif()
endif()

# TODO: add toplevel support for mingw
#if (${target_os} STREQUAL "mingw")
#  target_sources(ptlib PRIVATE
#    ${PTLIB_PLATFORM_SRC_DIR}/ptlib.cxx 
#    ${PTLIB_PLATFORM_SRC_DIR}/win32.cxx 
#    ${PTLIB_PLATFORM_SRC_DIR}/dllmain.cxx 
#    ${PTLIB_COMMON_SRC_DIR}/pchannel.cxx
#  )
#else()
  target_sources(ptlib PRIVATE
    ${PTLIB_PLATFORM_SRC_DIR}/udll.cxx 
    ${PTLIB_PLATFORM_SRC_DIR}/channel.cxx 
    ${PTLIB_PLATFORM_SRC_DIR}/osutil.cxx 
    ${PTLIB_PLATFORM_SRC_DIR}/tlib.cxx
  )
#endif()

# todo: add date support

target_sources(ptlib PRIVATE
    ${PTLIB_COMPONENT_SRC_DIR}/json.cxx
    ${PTLIB_COMPONENT_SRC_DIR}/textdata.cxx
    ${PTLIB_COMPONENT_SRC_DIR}/threadpool.cxx
    ${PTLIB_COMPONENT_SRC_DIR}/random.cxx
    ${PTLIB_PLATFORM_SRC_DIR}/assert.cxx
    ${PTLIB_COMMON_SRC_DIR}/notifier_ext.cxx
    ${PTLIB_COMMON_SRC_DIR}/safecoll.cxx
    ${PTLIB_COMMON_SRC_DIR}/ptime.cxx
    ${PTLIB_COMMON_SRC_DIR}/osutils.cxx
    ${PTLIB_COMMON_SRC_DIR}/collect.cxx
    ${PTLIB_COMMON_SRC_DIR}/contain.cxx
    ${PTLIB_COMMON_SRC_DIR}/object.cxx  # must be last module
    )
