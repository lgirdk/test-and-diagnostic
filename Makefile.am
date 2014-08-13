SUBDIRS = source

lib_LTLIBRARIES=libdiagnostic.la
libdiagnostic_la_CPPFLAGS= -D_ANSC_LINUX -D_ANSC_USER -D_ANSC_LITTLE_ENDIAN_ -D_COSA_SIM_ -I/var/tmp/pc-rdkb/include -I/var/tmp/pc-rdkb/include/net-snmp -I/var/tmp/pc-rdkb/include/dbus-1.0 -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/custom -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/include -I$(top_srcdir)/../CcspCommonLibrary/source/debug_api/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/include/linux -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/package/slap/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/ansc/include -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/common/MessageBusHelper/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/http/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/ansc/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/tls/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/web/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/stun/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/slap/components/SlapVarConverter -I$(top_srcdir)/source/diagnostic/include -I$(top_srcdir)/source/diagnostic/BbhmDiagIpPing -I$(top_srcdir)/source/dmltad -I$(top_srcdir)/source/TandDSsp -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/common/PoamIrepFolder
libdiagnostic_la_LDFLAGS= -L$(top_srcdir)/../CcspCommonLibrary/lib -lccsp_common
libdiagnostic_la_SOURCES= \
	source/diagnostic/BbhmDiagDownload/bbhm_download_interface.c \
	source/diagnostic/BbhmDiagDownload/bbhm_download_action.c \
	source/diagnostic/BbhmDiagDownload/bbhm_download_base.c \
	source/diagnostic/BbhmDiagDownload/bbhm_download_operation.c \
	source/diagnostic/BbhmDiagIpTraceroute/bbhm_diagit_tdo_base.c \
	source/diagnostic/BbhmDiagIpTraceroute/bbhm_diagit_sink_operation.c \
	source/diagnostic/BbhmDiagIpTraceroute/bbhm_diagit_interface.c \
	source/diagnostic/BbhmDiagIpTraceroute/bbhm_diagit_process.c \
	source/diagnostic/BbhmDiagIpTraceroute/bbhm_diagit_sink_states.c \
	source/diagnostic/BbhmDiagIpTraceroute/bbhm_diagit_tdo_operation.c \
	source/diagnostic/BbhmDiagIpTraceroute/bbhm_diagit_base.c \
	source/diagnostic/BbhmDiagIpTraceroute/bbhm_diagit_sink_base.c \
	source/diagnostic/BbhmDiagIpTraceroute/bbhm_diagit_operation.c \
	source/diagnostic/BbhmDiagIpTraceroute/bbhm_diagit_states.c \
	source/diagnostic/BbhmDiagExecutor/bbhm_diageo_interface.c \
	source/diagnostic/BbhmDiagExecutor/bbhm_diageo_process.c \
	source/diagnostic/BbhmDiagExecutor/bbhm_diageo_base.c \
	source/diagnostic/BbhmDiagExecutor/bbhm_diageo_states.c \
	source/diagnostic/BbhmDiagExecutor/bbhm_diageo_operation.c \
	source/diagnostic/BbhmUdpEchoServer/bbhm_udpecho_action.c \
	source/diagnostic/BbhmUdpEchoServer/bbhm_udpecho_base.c \
	source/diagnostic/BbhmUdpEchoServer/bbhm_udpecho_interface.c \
	source/diagnostic/BbhmUdpEchoServer/bbhm_udpecho_operation.c \
	source/diagnostic/bbhm_diag_lib.c \
	source/diagnostic/BbhmDiagIpPing/bbhm_diagip_sink_operation.c \
	source/diagnostic/BbhmDiagIpPing/bbhm_diagip_tdo_base.c \
	source/diagnostic/BbhmDiagIpPing/bbhm_diagip_process.c \
	source/diagnostic/BbhmDiagIpPing/bbhm_diagip_states.c \
	source/diagnostic/BbhmDiagIpPing/bbhm_diagip_sink_states.c \
	source/diagnostic/BbhmDiagIpPing/bbhm_diagip_tdo_operation.c \
	source/diagnostic/BbhmDiagIpPing/bbhm_diagip_base.c \
	source/diagnostic/BbhmDiagIpPing/bbhm_diagip_operation.c \
	source/diagnostic/BbhmDiagIpPing/bbhm_diagip_sink_base.c \
	source/diagnostic/BbhmDiagIpPing/bbhm_diagip_interface.c \
	source/diagnostic/BbhmDiagIpPing/bbhm_diagip_operation_priv.c \
	source/diagnostic/BbhmDiagUpload/bbhm_upload_interface.c \
	source/diagnostic/BbhmDiagUpload/bbhm_upload_action.c \
	source/diagnostic/BbhmDiagUpload/bbhm_upload_base.c \
	source/diagnostic/BbhmDiagUpload/bbhm_upload_operation.c \
	source/diagnostic/BbhmDiagNSLookup/bbhm_diagns_xsink_base.c \
	source/diagnostic/BbhmDiagNSLookup/bbhm_diagns_process.c \
	source/diagnostic/BbhmDiagNSLookup/bbhm_diagns_base.c \
	source/diagnostic/BbhmDiagNSLookup/bbhm_diagns_interface.c \
	source/diagnostic/BbhmDiagNSLookup/bbhm_diagns_tdo_base.c \
	source/diagnostic/BbhmDiagNSLookup/bbhm_diagns_xsink_operation.c \
	source/diagnostic/BbhmDiagNSLookup/bbhm_diagns_tdo_operation.c \
	source/diagnostic/BbhmDiagNSLookup/bbhm_diagns_operation.c \
	source/diagnostic/BbhmDiagNSLookup/bbhm_diagns_xsink_states.c \
	source/diagnostic/BbhmDiagNSLookup/bbhm_diagns_states.c
libdiagnostic_la_DEPENDENCIES=
libdiagnostic_la_LIBADD=$(libdiagnostic_la_DEPENDENCIES)
