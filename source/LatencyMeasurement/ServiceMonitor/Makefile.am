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

noinst_LTLIBRARIES = libCcspTandDSsp_ServiceMonitor.la

libCcspTandDSsp_ServiceMonitor_la_CPPFLAGS = -I$(top_srcdir)/source/LatencyMeasurement/ServiceMonitor/include \
                                             -I$(top_srcdir)/source/LatencyMeasurement/TR-181 \
                                             -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rbus

libCcspTandDSsp_ServiceMonitor_la_SOURCES = ServiceMonitor.c

libCcspTandDSsp_ServiceMonitor_la_LDFLAGS = -lsysevent -lsyscfg -lsecure_wrapper -lpthread 

libCcspTandDSsp_ServiceMonitor_DEPENDENCIES = ${top_builddir}/source/LatencyMeasurement/TR-181/libLowLatency.la

libCcspTandDSsp_ServiceMonitor_LDADD = $(libCcspTandDSsp_ServiceMonitor_DEPENDENCIES)
