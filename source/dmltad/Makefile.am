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
AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_LDFLAGS = -lccsp_common

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

lib_LTLIBRARIES = libdmltad.la
libdmltad_la_CPPFLAGS = -I$(top_srcdir)/source/diagnostic/include -I$(top_srcdir)/source/diagnostic/BbhmDiagIpPing -I$(top_srcdir)/source/dmltad -I$(top_srcdir)/source/TandDSsp $(CPPFLAGS)
libdmltad_la_SOURCES = cosa_apis_util.c cosa_apis_deviceinfo.c plugin_main.c cosa_apis_util_priv.c cosa_ip_dml_priv.c cosa_apis_busutil.c cosa_dns_dml.c plugin_main_apis.c cosa_apis_vendorlogfile.c cosa_deviceinfo_util_priv.c cosa_ip_dml.c cosa_diagnostic_apis_priv.c cosa_deviceinfo_util.c cosa_diagnostic_apis.c plugin_main_priv.c diag.c diag_ping.c diag_traceroute.c
libdmltad_la_LDFLAGS = -lccsp_common
