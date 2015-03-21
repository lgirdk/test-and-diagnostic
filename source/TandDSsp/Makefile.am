AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -D_COSA_SIM_
AM_LDFLAGS = -lccsp_common -ldl

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu
bin_PROGRAMS = CcspTandDSsp
CcspTandDSsp_CPPFLAGS = -I$(top_srcdir)/source/diagnostic/include -I$(top_srcdir)/source/diagnostic/BbhmDiagIpPing -I$(top_srcdir)/source/dmltad -I$(top_srcdir)/source/TandDSsp $(CPPFLAGS)
CcspTandDSsp_SOURCES = ssp_messagebus_interface.c ssp_action_priv.c ssp_main.c ssp_action.c ssp_messagebus_interface_priv.c
CcspTandDSsp_LDFLAGS = -lccsp_common -ldl
