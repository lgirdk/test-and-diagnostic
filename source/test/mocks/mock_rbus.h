#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <rbus/rbus.h>

class rbusInterface
{
    public:
        virtual ~rbusInterface() {}
        virtual rbusError_t rbus_open(rbusHandle_t*, char const*) = 0;
        virtual rbusError_t rbus_close(rbusHandle_t) = 0;
        virtual rbusError_t rbus_get( rbusHandle_t , char const* , rbusValue_t* ) = 0;
        virtual char* rbusValue_ToString(rbusValue_t , char* , size_t ) = 0;
        virtual rbusObject_t rbusObject_Init(rbusObject_t*, char const*) = 0;
        virtual rbusValue_t rbusValue_Init(rbusValue_t*) = 0;
        virtual void rbusValue_SetString(rbusValue_t, char const*) = 0;
        virtual char const* rbusValue_GetString(rbusValue_t, int*) = 0;
        virtual void rbusObject_SetValue(rbusObject_t, char const*, rbusValue_t) = 0;
        virtual rbusValue_t rbusObject_GetValue(rbusObject_t, char const*) = 0;
        virtual void rbusValue_SetInt32(rbusValue_t, int32_t) = 0;
        virtual void rbusValue_Release(rbusValue_t) = 0;
        virtual void rbusObject_Release(rbusObject_t) = 0;
        virtual rbusError_t rbusMethod_Invoke(rbusHandle_t, char const*, rbusObject_t, rbusObject_t*) = 0;
        virtual const char* rbusError_ToString(rbusError_t) = 0;
        virtual bool rbusCheckMethodExists(const char*) = 0;
        virtual int rbusMethodCaller(char *, rbusObject_t*, char*) = 0;
};

class rbusMock: public rbusInterface
{
    public:
        virtual ~rbusMock() {}
        MOCK_METHOD1(rbusError_ToString, const char*(rbusError_t));
        MOCK_METHOD2(rbus_open, rbusError_t(rbusHandle_t*, char const*));
        MOCK_METHOD1(rbus_close, rbusError_t(rbusHandle_t));
        MOCK_METHOD3(rbus_get, rbusError_t( rbusHandle_t , char const* , rbusValue_t* ));
        MOCK_METHOD3(rbusValue_ToString, char* (rbusValue_t , char* , size_t ));
        MOCK_METHOD2(rbusObject_Init, rbusObject_t(rbusObject_t*, char const*));
        MOCK_METHOD1(rbusValue_Init, rbusValue_t(rbusValue_t*));
        MOCK_METHOD2(rbusValue_SetString, void(rbusValue_t, char const*));
        MOCK_METHOD2(rbusValue_GetString, char const*(rbusValue_t, int*));
        MOCK_METHOD3(rbusObject_SetValue, void(rbusObject_t, char const*, rbusValue_t));
        MOCK_METHOD2(rbusObject_GetValue, rbusValue_t(rbusObject_t, char const*));
        MOCK_METHOD2(rbusValue_SetInt32, void(rbusValue_t, int32_t));
        MOCK_METHOD1(rbusValue_Release, void(rbusValue_t));
        MOCK_METHOD1(rbusObject_Release, void(rbusObject_t));
        MOCK_METHOD4(rbusMethod_Invoke, rbusError_t(rbusHandle_t, char const*, rbusObject_t, rbusObject_t*));
        MOCK_METHOD1(rbusCheckMethodExists, bool(const char*));
        MOCK_METHOD3(rbusMethodCaller, int(char *, rbusObject_t*, char*));
};


