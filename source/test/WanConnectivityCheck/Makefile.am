##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
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

AM_CPPFLAGS = -Wall -g -Werror
AM_CXXFLAGS = -std=c++11
#AM_CXXFLAGS += --coverage

AUTOMAKE_OPTIONS = subdir-objects

ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

bin_PROGRAMS = WanConnectivityCheck_gtest.bin
WanConnectivityCheck_gtest_bin_CPPFLAGS = -I$(PKG_CONFIG_SYSROOT_DIR)$(includedir)/gtest \
                                          -I${top_srcdir}/gtest/include \
                                          -I${top_srcdir}/source \
                                          -I${top_srcdir}/source/include \
                                          -I$(PKG_CONFIG_SYSROOT_DIR)$(includedir)/dbus-1.0 \
                                          -I$(PKG_CONFIG_SYSROOT_DIR)$(libdir)/dbus-1.0/include \
                                          -I$(PKG_CONFIG_SYSROOT_DIR)$(includedir)/ccsp \
                                          -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rbus \
                                          -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/libsafec \
                                          -I${PKG_CONFIG_SYSROOT_DIR}$(includedir) \
                                          -I$(top_srcdir)/source/diagnostic/include \
                                          -I$(top_srcdir)/source/diagnostic/BbhmDiagIpPing \
                                          -I$(top_srcdir)/source/dmltad \
                                          -I$(top_srcdir)/source/TandDSsp

WanConnectivityCheck_gtest_bin_SOURCES =  WanConnectivityCheck.cpp \
                                  ../mocks/mock_utils.cpp \
                                  ../mocks/mock_rbus.cpp \
                                  ../mocks/mock_wanconnectivity.cpp \
                                  gtest_main.cpp
WanConnectivityCheck_gtest_bin_LDADD = ${top_builddir}/source/dmltad/libdmltad.la \
                                       $(top_builddir)/source-arm/dmltad/libdmlarm.la
WanConnectivityCheck_gtest_bin_LDFLAGS = -lgtest -lgmock -lgcov -lccsp_common -pthread

