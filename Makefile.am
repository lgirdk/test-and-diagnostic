##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2015 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

libdiagnostic_la_CPPFLAGS = -D_ANSC_LINUX -D_ANSC_USER -D_COSA_SIM_ -I$(top_srcdir)/source/diagnostic/include -I$(top_srcdir)/source/diagnostic/BbhmDiagIpPing -I$(top_srcdir)/source/dmltad -I$(top_srcdir)/source/TandDSsp

if CCSP_ARCH_ARM
libdiagnostic_la_CPPFLAGS += -D_ANSC_LITTLE_ENDIAN_
OPT_SRCS=source-arm/diagnostic/BbhmDiagIpPing source-arm/dmltad
EXTRA_DEPENDENCIES = $(top_builddir)/source-arm/diagnostic/BbhmDiagIpPing/libdiagprivarm.la
endif

if CCSP_ARCH_PC
libdiagnostic_la_CPPFLAGS += -D_ANSC_LITTLE_ENDIAN_
noinst_LTLIBRARIES = libdiagprivpc.la libdmlpc.la
libdiagprivpc_la_SOURCES = $(top_srcdir)/source/diagnostic/BbhmDiagIpPing/bbhm_diagip_operation_priv.c
libdmlpc_la_SOURCES = $(top_srcdir)/source/dmltad/cosa_diagnostic_apis_priv.c $(top_srcdir)/source/dmltad/plugin_main_priv.c
EXTRA_DEPENDENCIES = libdiagprivpc.la libdmlpc.la
endif

if CCSP_ARCH_MIPS
OPT_SRCS=source-mips/diagnostic/BbhmDiagIpPing source-mips/dmltad
EXTRA_DEPENDENCIES = $(top_builddir)/source-mips/diagnostic/BbhmDiagIpPing/libdiagprivarm.la
endif

SUBDIRS = $(OPT_SRCS) source


lib_LTLIBRARIES=libdiagnostic.la
libdiagnostic_la_LDFLAGS= -lccsp_common
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
libdiagnostic_la_DEPENDENCIES=$(EXTRA_DEPENDENCIES)
libdiagnostic_la_LIBADD=$(libdiagnostic_la_DEPENDENCIES)
