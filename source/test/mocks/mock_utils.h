/*
* Copyright 2020 Comcast Cable Communications Management, LLC
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef MOCK_UTILS_H
#define MOCK_UTILS_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdarg>

class UtilsInterface {
public:
   virtual ~UtilsInterface() {}
   virtual int system(const char *) = 0;
   virtual int access(const char *, int) = 0;
   virtual int v_secure_system(const char *) = 0;
};

class UtilsMock : public UtilsInterface {
public:
   virtual ~UtilsMock() {}
   MOCK_METHOD1(system, int(const char *));
   MOCK_METHOD2(access, int(const char *, int));
   MOCK_METHOD1(v_secure_system, int(const char *));
};

#endif