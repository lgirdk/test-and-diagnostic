AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_LDFLAGS = -L$(top_srcdir)/../CcspCommonLibrary/lib
AM_LDFLAGS += -lccsp_common

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

lib_LTLIBRARIES = libdmltad.la
libdmltad_la_CPPFLAGS = -I/var/tmp/pc-rdkb/include -I/var/tmp/pc-rdkb/include/net-snmp -I/var/tmp/pc-rdkb/include/dbus-1.0 -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/custom -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/include -I$(top_srcdir)/../CcspCommonLibrary/source/debug_api/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/include/linux -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/package/slap/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/ansc/include -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/common/MessageBusHelper/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/http/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/ansc/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/tls/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/web/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/stun/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/slap/components/SlapVarConverter -I$(top_srcdir)/source/diagnostic/include -I$(top_srcdir)/source/diagnostic/BbhmDiagIpPing -I$(top_srcdir)/source/dmltad -I$(top_srcdir)/source/TandDSsp -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/common/PoamIrepFolder $(CPPFLAGS)
libdmltad_la_SOURCES = cosa_apis_util.c cosa_apis_deviceinfo.c plugin_main.c cosa_apis_util_priv.c cosa_ip_dml_priv.c cosa_apis_busutil.c cosa_dns_dml.c plugin_main_apis.c cosa_apis_vendorlogfile.c cosa_deviceinfo_util_priv.c cosa_ip_dml.c cosa_diagnostic_apis_priv.c cosa_deviceinfo_util.c cosa_diagnostic_apis.c plugin_main_priv.c
libdmltad_la_LDFLAGS =  -L$(top_srcdir)/../CcspCommonLibrary/lib -lccsp_common