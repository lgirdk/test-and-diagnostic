#include "test/mocks/mock_rbus.h"

extern rbusMock *g_rbusMock;

extern "C" rbusError_t rbus_open(rbusHandle_t* handle, char const* value)
{
    if (!g_rbusMock)
    {
        return RBUS_ERROR_SUCCESS;
    }
    return g_rbusMock->rbus_open( handle, value);
}

extern "C" rbusError_t rbus_close(rbusHandle_t handle)
{
    if (!g_rbusMock)
    {
        return RBUS_ERROR_SUCCESS;
    }
    return g_rbusMock->rbus_close(handle);
}

extern "C" rbusError_t rbus_get( rbusHandle_t handle, char const* name, rbusValue_t* value)
{
    if (!g_rbusMock)
    {
        return RBUS_ERROR_SUCCESS;
    }
    return g_rbusMock->rbus_get( handle, name, value);
}

extern "C" char* rbusValue_ToString(rbusValue_t value, char* buf, size_t buflen)
{
    if (!g_rbusMock)
    {
        return (char *)NULL;
    }
    return g_rbusMock->rbusValue_ToString( value, buf, buflen);
}

extern "C" rbusObject_t rbusObject_Init(rbusObject_t* object, char const* value)
{
    if (!g_rbusMock)
    {
        return NULL;
    }
    return g_rbusMock->rbusObject_Init(object, value);
}

extern "C" rbusValue_t rbusValue_Init(rbusValue_t* value)
{
    if (!g_rbusMock)
    {
        return NULL;
    }
    return g_rbusMock->rbusValue_Init(value);
}

extern "C" void rbusValue_SetString(rbusValue_t value, char const* var)
{
    if (!g_rbusMock)
    {
        return;
    }
    return g_rbusMock->rbusValue_SetString(value, var);
}

extern "C" char const* rbusValue_GetString(rbusValue_t value, int* len)
{
    if (!g_rbusMock)
    {
        return NULL;
    }
    return g_rbusMock->rbusValue_GetString(value, len);
}

extern "C" void rbusObject_SetValue(rbusObject_t object, char const* var, rbusValue_t value)
{
    if (!g_rbusMock)
    {
        return;
    }
    return g_rbusMock->rbusObject_SetValue(object, var, value);
}

extern "C" rbusValue_t rbusObject_GetValue(rbusObject_t object, char const* var) 
{
    if (!g_rbusMock)
    {
        return NULL;
    }
    return g_rbusMock->rbusObject_GetValue(object, var);
}

extern "C" void rbusValue_Release(rbusValue_t value)
{
    if (!g_rbusMock)
    {
        return;
    }
    return g_rbusMock->rbusValue_Release(value);
}

extern "C" void rbusObject_Release(rbusObject_t object)
{
    if (!g_rbusMock)
    {
        return;
    }
    return g_rbusMock->rbusObject_Release(object);
}

extern "C" void rbusValue_SetInt32(rbusValue_t value, int32_t i32)
{
    if(!g_rbusMock)
    {
       return;
    }
    return g_rbusMock->rbusValue_SetInt32(value, i32);
}

extern "C" rbusError_t rbusMethod_Invoke(rbusHandle_t handle, char const* value, rbusObject_t object, rbusObject_t* objectpt)
{
    if (!g_rbusMock)
    {
        return RBUS_ERROR_SUCCESS;
    }
    return g_rbusMock->rbusMethod_Invoke(handle, value, object, objectpt);
}

extern "C" const char* rbusError_ToString(rbusError_t e)
{
    if (!g_rbusMock)
    {
        return "RBUS_ERROR_INVALID_OPERATION";
    }
    else{
    #define rbusError_String(E, S) case E: s = S; break;

  char const * s = NULL;
  switch (e)
  {
    rbusError_String(RBUS_ERROR_SUCCESS, "ok");
    rbusError_String(RBUS_ERROR_BUS_ERROR, "generic error");
    rbusError_String(RBUS_ERROR_INVALID_INPUT, "invalid input");
    rbusError_String(RBUS_ERROR_NOT_INITIALIZED, "not initialized");
    rbusError_String(RBUS_ERROR_OUT_OF_RESOURCES, "out of resources");
    rbusError_String(RBUS_ERROR_DESTINATION_NOT_FOUND, "destination not found");
    rbusError_String(RBUS_ERROR_DESTINATION_NOT_REACHABLE, "destination not reachable");
    rbusError_String(RBUS_ERROR_DESTINATION_RESPONSE_FAILURE, "destination response failure");
    rbusError_String(RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION, "invalid response from destination");
    rbusError_String(RBUS_ERROR_INVALID_OPERATION, "invalid operation");
    rbusError_String(RBUS_ERROR_INVALID_EVENT, "invalid event");
    rbusError_String(RBUS_ERROR_INVALID_HANDLE, "invalid handle");
    rbusError_String(RBUS_ERROR_SESSION_ALREADY_EXIST, "session already exists");
    rbusError_String(RBUS_ERROR_COMPONENT_NAME_DUPLICATE, "duplicate component name");
    rbusError_String(RBUS_ERROR_ELEMENT_NAME_DUPLICATE, "duplicate element name");
    rbusError_String(RBUS_ERROR_ELEMENT_NAME_MISSING, "name missing");
    rbusError_String(RBUS_ERROR_COMPONENT_DOES_NOT_EXIST, "component does not exist");
    rbusError_String(RBUS_ERROR_ELEMENT_DOES_NOT_EXIST, "element name does not exist");
    rbusError_String(RBUS_ERROR_ACCESS_NOT_ALLOWED, "access denied");
    rbusError_String(RBUS_ERROR_INVALID_CONTEXT, "invalid context");
    rbusError_String(RBUS_ERROR_TIMEOUT, "timeout");
    rbusError_String(RBUS_ERROR_ASYNC_RESPONSE, "async operation in progress");
    default:
      s = "unknown error";
  }
  return s;
    }
}

extern "C" bool rbusCheckMethodExists(const char* rbusMethodName)
{
    if (!g_rbusMock)
    {
        return false;
    }
    return g_rbusMock->rbusCheckMethodExists(rbusMethodName);
}

extern "C" int rbusMethodCaller(char *methodName, rbusObject_t* inputParams, char* payload)
{
    if (!g_rbusMock)
    {
        return -1;
    }
    return g_rbusMock->rbusMethodCaller(methodName, inputParams, payload);
}
