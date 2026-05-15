#ifndef IEC61850_WRAPPER_H
#define IEC61850_WRAPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
  #ifdef IEC61850_WRAPPER_EXPORTS
    #define IEC61850_API __declspec(dllexport)
  #else
    #define IEC61850_API __declspec(dllimport)
  #endif
#else
  #define IEC61850_API
#endif

/* basic types */
typedef void* IEC61850_HANDLE;

enum IEC61850_RET
{
    IEC61850_RET_OK = 0,
    IEC61850_RET_FAIL = -1,
    IEC61850_RET_INVALID_PARAM = -2,
    IEC61850_RET_NOT_CONNECTED = -3,
    IEC61850_RET_TIMEOUT = -4,
    IEC61850_RET_UNSUPPORTED = -5,
    IEC61850_RET_NOT_FOUND = -6,
    IEC61850_RET_PERMISSION = -7,
    IEC61850_RET_STATE_ERROR = -8
};
typedef enum IEC61850_RET IEC61850_RET;

enum IEC61850_VALUE_TYPE
{
    IEC61850_TYPE_BOOL = 1,
    IEC61850_TYPE_INT32,
    IEC61850_TYPE_UINT32,
    IEC61850_TYPE_FLOAT,
    IEC61850_TYPE_DOUBLE,
    IEC61850_TYPE_ENUM,
    IEC61850_TYPE_TIMESTAMP_MS,
    IEC61850_TYPE_BITSTRING
};
typedef enum IEC61850_VALUE_TYPE IEC61850_VALUE_TYPE;

enum IEC61850_CTRL_MODE
{
    IEC61850_CTRL_DIRECT = 0,
    IEC61850_CTRL_SBO = 1
};
typedef enum IEC61850_CTRL_MODE IEC61850_CTRL_MODE;

enum IEC61850_QUALITY
{
    IEC61850_QUALITY_GOOD = 0,
    IEC61850_QUALITY_INVALID = 1,
    IEC61850_QUALITY_QUESTIONABLE = 2,
    IEC61850_QUALITY_RESERVED = 3
};
typedef enum IEC61850_QUALITY IEC61850_QUALITY;

struct IEC61850_TimeStamp
{
    int32_t year;
    int32_t month;
    int32_t day;
    int32_t hour;
    int32_t minute;
    int32_t second;
    int32_t msecond;
};
typedef struct IEC61850_TimeStamp IEC61850_TimeStamp;

struct IEC61850_ConnectParam
{
    const char* serverIp;
    int32_t     serverPort;
    int32_t     connectTimeoutMs;
    int32_t     requestTimeoutMs;
    int32_t     autoReconnect;
    int32_t     reconnectIntervalMs;
    const char* reserved1;
    const char* reserved2;
};
typedef struct IEC61850_ConnectParam IEC61850_ConnectParam;

struct IEC61850_Value
{
    IEC61850_VALUE_TYPE type;

    union
    {
        int32_t  b;
        int32_t  i32;
        uint32_t u32;
        float    f32;
        double   f64;
        int32_t  e;
        int64_t  tsMs;
        uint32_t bitString;
    } v;

    IEC61850_QUALITY quality;
    int64_t          sourceTsMs;
};
typedef struct IEC61850_Value IEC61850_Value;

struct IEC61850_ReportItem
{
    const char*    ref;
    IEC61850_Value value;
};
typedef struct IEC61850_ReportItem IEC61850_ReportItem;

struct IEC61850_ReportBatch
{
    const char*              rcbRef;
    const IEC61850_ReportItem* items;
    int32_t                  itemCount;
    int64_t                  reportTsMs;
};
typedef struct IEC61850_ReportBatch IEC61850_ReportBatch;

/* callbacks */
typedef void (*IEC61850_LOG_CB)(int32_t level, const char* msg, void* userData);
typedef void (*IEC61850_REPORT_CB)(const IEC61850_ReportBatch* batch, void* userData);
typedef void (*IEC61850_STATE_CB)(int32_t connected, int32_t reason, void* userData);

/* lifecycle */
IEC61850_API int32_t Iec61850_GlobalInit(void);
IEC61850_API int32_t Iec61850_GlobalUninit(void);
IEC61850_API IEC61850_HANDLE Iec61850_CreateClient(void);
IEC61850_API int32_t Iec61850_DestroyClient(IEC61850_HANDLE h);
IEC61850_API int32_t Iec61850_SetLogCallback(IEC61850_HANDLE h, IEC61850_LOG_CB cb, void* userData);
IEC61850_API int32_t Iec61850_SetStateCallback(IEC61850_HANDLE h, IEC61850_STATE_CB cb, void* userData);

/* connection */
IEC61850_API int32_t Iec61850_Connect(IEC61850_HANDLE h, const IEC61850_ConnectParam* param);
IEC61850_API int32_t Iec61850_Disconnect(IEC61850_HANDLE h);
IEC61850_API int32_t Iec61850_IsConnected(IEC61850_HANDLE h, int32_t* connected);
IEC61850_API int32_t Iec61850_KeepAlive(IEC61850_HANDLE h);

/* read/write */
IEC61850_API int32_t Iec61850_ReadValue(IEC61850_HANDLE h, const char* ref, IEC61850_VALUE_TYPE type, IEC61850_Value* outVal);
IEC61850_API int32_t Iec61850_ReadValues(IEC61850_HANDLE h,
                                         const char** refs,
                                         const IEC61850_VALUE_TYPE* types,
                                         int32_t count,
                                         IEC61850_Value* outVals);
IEC61850_API int32_t Iec61850_WriteValue(IEC61850_HANDLE h, const char* ref, const IEC61850_Value* inVal);

/* control */
IEC61850_API int32_t Iec61850_Operate(IEC61850_HANDLE h,
                                      const char* ctlRef,
                                      const IEC61850_Value* ctlVal,
                                      IEC61850_CTRL_MODE mode,
                                      int32_t timeoutMs);
IEC61850_API int32_t Iec61850_Select(IEC61850_HANDLE h, const char* ctlRef, int32_t timeoutMs);
IEC61850_API int32_t Iec61850_Cancel(IEC61850_HANDLE h, const char* ctlRef);

/* report */
IEC61850_API int32_t Iec61850_SetReportCallback(IEC61850_HANDLE h, IEC61850_REPORT_CB cb, void* userData);
IEC61850_API int32_t Iec61850_EnableReport(IEC61850_HANDLE h, const char* rcbRef);
IEC61850_API int32_t Iec61850_DisableReport(IEC61850_HANDLE h, const char* rcbRef);

/* time sync */
IEC61850_API int32_t Iec61850_WriteTimeByRef(IEC61850_HANDLE h, const char* timeRef, const IEC61850_TimeStamp* ts);
IEC61850_API int32_t Iec61850_TimeSync(IEC61850_HANDLE h, const IEC61850_TimeStamp* ts);

/* diagnostic */
IEC61850_API const char* Iec61850_GetLastErrorText(IEC61850_HANDLE h);
IEC61850_API int32_t Iec61850_GetLastErrorCode(IEC61850_HANDLE h);

/* @TODO 其它待补充... */

#ifdef __cplusplus
}
#endif

#endif
