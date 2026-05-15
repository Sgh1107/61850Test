#include "iec61850_wrapper.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "iec61850_client.h"
#include "iec61850_common.h"
#include "mms_value.h"

// ============================================================
// 内部结构与辅助
// ============================================================

#define IEC61850_ERRTXT_LEN 256

typedef struct IEC61850_ClientContext
{
    IEC61850_ConnectParam conn;
    int32_t connected;
    int32_t reportEnabled;
    int32_t stateNotified;

    int32_t lastErrCode;
    char    lastErrText[IEC61850_ERRTXT_LEN];

    IEC61850_LOG_CB    logCb;
    void*              logUser;
    IEC61850_STATE_CB  stateCb;
    void*              stateUser;
    IEC61850_REPORT_CB rptCb;
    void*              rptUser;

    IedConnection iedConn;

    ClientReportControlBlock rcb;
    char activeRcbRef[256];
    char activeRptId[128];

    int64_t lastKeepAliveMs;

} IEC61850_ClientContext;

static int64_t now_ms(void);
static void set_error(IEC61850_ClientContext* ctx, int32_t code, const char* text);
static int32_t set_error_ret(IEC61850_ClientContext* ctx, int32_t code, const char* text);
static void log_msg(IEC61850_ClientContext* ctx, int32_t level, const char* msg);
static int32_t map_ied_err(IedClientError err);
static const char* ied_error_to_string(IedClientError err);

static void notify_state(IEC61850_ClientContext* ctx, int32_t connected, int32_t reason)
{
    if (!ctx) return;

    ctx->stateNotified = connected;

    if (ctx->stateCb) {
        ctx->stateCb(connected, reason, ctx->stateUser);
    }
}

static void destroy_report_subscription(IEC61850_ClientContext* ctx)
{
    if (!ctx) return;

    if (ctx->rcb) {
        ClientReportControlBlock_setRptEna(ctx->rcb, false);

        if (ctx->iedConn) {
            IedClientError err = IED_ERROR_OK;
            IedConnection_setRCBValues(ctx->iedConn, &err, ctx->rcb, RCB_ELEMENT_RPT_ENA, true);
            if (ctx->activeRcbRef[0] != '\0') {
                IedConnection_uninstallReportHandler(ctx->iedConn, ctx->activeRcbRef);
            }
        }

        ClientReportControlBlock_destroy(ctx->rcb);
        ctx->rcb = NULL;
    }

    ctx->activeRcbRef[0] = '\0';
    ctx->activeRptId[0] = '\0';
    ctx->reportEnabled = 0;
}

static void destroy_connection_only(IEC61850_ClientContext* ctx)
{
    if (!ctx) return;

    if (ctx->iedConn) {
        IedConnection_close(ctx->iedConn);
        IedConnection_destroy(ctx->iedConn);
        ctx->iedConn = NULL;
    }

    ctx->connected = 0;
}

static int32_t connect_internal(IEC61850_ClientContext* ctx)
{
    IedClientError err = IED_ERROR_OK;
    int32_t ret;

    if (!ctx || !ctx->conn.serverIp) return IEC61850_RET_INVALID_PARAM;

    destroy_report_subscription(ctx);
    destroy_connection_only(ctx);

    ctx->iedConn = IedConnection_create();
    if (!ctx->iedConn) {
        return set_error_ret(ctx, IEC61850_RET_FAIL, "IedConnection_create failed");
    }

    IedConnection_setConnectTimeout(ctx->iedConn, (uint32_t)ctx->conn.connectTimeoutMs);
    IedConnection_setRequestTimeout(ctx->iedConn, (uint32_t)ctx->conn.requestTimeoutMs);

    IedConnection_connect(ctx->iedConn, &err, ctx->conn.serverIp, ctx->conn.serverPort);

    ret = map_ied_err(err);
    if (ret != IEC61850_RET_OK) {
        IedConnection_destroy(ctx->iedConn);
        ctx->iedConn = NULL;
        ctx->connected = 0;
        return set_error_ret(ctx, ret, ied_error_to_string(err));
    }

    ctx->connected = 1;
    ctx->lastKeepAliveMs = now_ms();
    set_error(ctx, IEC61850_RET_OK, "connect success");
    log_msg(ctx, 2, "Iec61850_Connect success");
    notify_state(ctx, 1, 0);

    return IEC61850_RET_OK;
}

static int32_t ensure_connection(IEC61850_ClientContext* ctx)
{
    IedConnectionState st;

    if (!ctx || !ctx->iedConn) {
        if (ctx && ctx->conn.autoReconnect) {
            return connect_internal(ctx);
        }
        return IEC61850_RET_NOT_CONNECTED;
    }

    st = IedConnection_getState(ctx->iedConn);
    if (st == IED_STATE_CONNECTED) {
        ctx->connected = 1;
        return IEC61850_RET_OK;
    }

    ctx->connected = 0;

    if (ctx->stateNotified != 0) {
        notify_state(ctx, 0, 1);
    }

    if (ctx->conn.autoReconnect) {
        log_msg(ctx, 1, "connection lost - auto reconnect");
        return connect_internal(ctx);
    }

    return IEC61850_RET_NOT_CONNECTED;
}

static int64_t now_ms(void)
{
    return (int64_t)time(NULL) * 1000;
}

static int32_t timestamp_to_ms(const IEC61850_TimeStamp* ts, int64_t* outMs)
{
    struct tm t;
    time_t sec;

    if (!ts || !outMs) return IEC61850_RET_INVALID_PARAM;

    if (ts->month < 1 || ts->month > 12 ||
        ts->day < 1 || ts->day > 31 ||
        ts->hour < 0 || ts->hour > 23 ||
        ts->minute < 0 || ts->minute > 59 ||
        ts->second < 0 || ts->second > 60 ||
        ts->msecond < 0 || ts->msecond > 999) {
        return IEC61850_RET_INVALID_PARAM;
    }

    memset(&t, 0, sizeof(t));
    t.tm_year = ts->year - 1900;
    t.tm_mon = ts->month - 1;
    t.tm_mday = ts->day;
    t.tm_hour = ts->hour;
    t.tm_min = ts->minute;
    t.tm_sec = ts->second;
    t.tm_isdst = 0;

#if defined(_WIN32) || defined(_WIN64)
    sec = _mkgmtime(&t);
#else
    sec = timegm(&t);
#endif

    if (sec == (time_t)-1) {
        return IEC61850_RET_INVALID_PARAM;
    }

    *outMs = ((int64_t)sec * 1000) + ts->msecond;
    return IEC61850_RET_OK;
}

static void set_error(IEC61850_ClientContext* ctx, int32_t code, const char* text)
{
    if (!ctx) return;

    ctx->lastErrCode = code;
    if (text) {
        strncpy(ctx->lastErrText, text, sizeof(ctx->lastErrText) - 1);
        ctx->lastErrText[sizeof(ctx->lastErrText) - 1] = '\0';
    }
    else {
        ctx->lastErrText[0] = '\0';
    }
}

static int32_t set_error_ret(IEC61850_ClientContext* ctx, int32_t code, const char* text)
{
    set_error(ctx, code, text);
    return code;
}

static void log_msg(IEC61850_ClientContext* ctx, int32_t level, const char* msg)
{
    if (ctx && ctx->logCb) {
        ctx->logCb(level, msg ? msg : "", ctx->logUser);
    }
}

static int32_t is_valid_handle(IEC61850_HANDLE h)
{
    return (h != NULL) ? 1 : 0;
}

static IEC61850_ClientContext* as_ctx(IEC61850_HANDLE h)
{
    return (IEC61850_ClientContext*)h;
}

static void set_default_conn_param(IEC61850_ConnectParam* p)
{
    if (!p) return;

    p->serverIp = NULL;
    p->serverPort = 102;
    p->connectTimeoutMs = 3000;
    p->requestTimeoutMs = 3000;
    p->autoReconnect = 0;
    p->reconnectIntervalMs = 3000;
    p->reserved1 = NULL;
    p->reserved2 = NULL;
}

static int32_t is_supported_value_type(IEC61850_VALUE_TYPE type)
{
    switch (type)
    {
    case IEC61850_TYPE_BOOL:
    case IEC61850_TYPE_INT32:
    case IEC61850_TYPE_UINT32:
    case IEC61850_TYPE_FLOAT:
    case IEC61850_TYPE_DOUBLE:
    case IEC61850_TYPE_ENUM:
    case IEC61850_TYPE_TIMESTAMP_MS:
    case IEC61850_TYPE_BITSTRING:
        return 1;
    default:
        return 0;
    }
}

static FunctionalConstraint infer_fc_from_ref(const char* ref, IEC61850_VALUE_TYPE type, int isWrite)
{
    (void)isWrite;

    if (!ref) return IEC61850_FC_ST;

    if (strstr(ref, ".stVal") != NULL) return IEC61850_FC_ST;
    if (strstr(ref, ".q") != NULL) return IEC61850_FC_ST;
    if (strstr(ref, ".t") != NULL) return IEC61850_FC_ST;
    if (strstr(ref, ".mag") != NULL || strstr(ref, ".cVal") != NULL) return IEC61850_FC_MX;
    if (strstr(ref, ".setVal") != NULL || strstr(ref, ".spVal") != NULL) return IEC61850_FC_SP;
    if (strstr(ref, ".ctlVal") != NULL || strstr(ref, ".Oper") != NULL) return IEC61850_FC_CO;

    if (type == IEC61850_TYPE_FLOAT || type == IEC61850_TYPE_DOUBLE)
        return IEC61850_FC_MX;

    return IEC61850_FC_ST;
}

static int32_t map_ied_err(IedClientError err)
{
    switch (err)
    {
    case IED_ERROR_OK:
        return IEC61850_RET_OK;
    case IED_ERROR_TIMEOUT:
        return IEC61850_RET_TIMEOUT;
    case IED_ERROR_CONNECTION_LOST:
    case IED_ERROR_NOT_CONNECTED:
        return IEC61850_RET_NOT_CONNECTED;
    case IED_ERROR_OBJECT_DOES_NOT_EXIST:
    case IED_ERROR_OBJECT_REFERENCE_INVALID:
        return IEC61850_RET_NOT_FOUND;
    case IED_ERROR_ACCESS_DENIED:
        return IEC61850_RET_PERMISSION;
    case IED_ERROR_SERVICE_NOT_SUPPORTED:
    case IED_ERROR_ENABLE_REPORT_FAILED_DATASET_MISMATCH:
        return IEC61850_RET_UNSUPPORTED;
    case IED_ERROR_USER_PROVIDED_INVALID_ARGUMENT:
        return IEC61850_RET_INVALID_PARAM;
    default:
        return IEC61850_RET_FAIL;
    }
}

static const char* ied_error_to_string(IedClientError err)
{
    switch (err)
    {
    case IED_ERROR_OK:
        return "ok";
    case IED_ERROR_NOT_CONNECTED:
        return "not connected";
    case IED_ERROR_ALREADY_CONNECTED:
        return "already connected";
    case IED_ERROR_CONNECTION_LOST:
        return "connection lost";
    case IED_ERROR_SERVICE_NOT_SUPPORTED:
        return "service not supported";
    case IED_ERROR_CONNECTION_REJECTED:
        return "connection rejected";
    case IED_ERROR_OUTSTANDING_CALL_LIMIT_REACHED:
        return "outstanding call limit reached";
    case IED_ERROR_USER_PROVIDED_INVALID_ARGUMENT:
        return "invalid argument";
    case IED_ERROR_ENABLE_REPORT_FAILED_DATASET_MISMATCH:
        return "enable report failed dataset mismatch";
    case IED_ERROR_OBJECT_REFERENCE_INVALID:
        return "object reference invalid";
    case IED_ERROR_TIMEOUT:
        return "timeout";
    case IED_ERROR_ACCESS_DENIED:
        return "access denied";
    case IED_ERROR_OBJECT_DOES_NOT_EXIST:
        return "object does not exist";
    case IED_ERROR_OBJECT_EXISTS:
        return "object exists";
    case IED_ERROR_OBJECT_ACCESS_UNSUPPORTED:
        return "object access unsupported";
    case IED_ERROR_TYPE_INCONSISTENT:
        return "type inconsistent";
    case IED_ERROR_TEMPORARILY_UNAVAILABLE:
        return "temporarily unavailable";
    case IED_ERROR_OBJECT_UNDEFINED:
        return "object undefined";
    case IED_ERROR_HARDWARE_FAULT:
        return "hardware fault";
    case IED_ERROR_OBJECT_ATTRIBUTE_INCONSISTENT:
        return "object attribute inconsistent";
    case IED_ERROR_OBJECT_VALUE_INVALID:
        return "object value invalid";
    case IED_ERROR_OBJECT_INVALIDATED:
        return "object invalidated";
    default:
        return "ied client error";
    }
}

static IEC61850_VALUE_TYPE infer_value_type_from_mms(const MmsValue* mv)
{
    if (!mv) return IEC61850_TYPE_INT32;

    switch (MmsValue_getType(mv))
    {
    case MMS_BOOLEAN:
        return IEC61850_TYPE_BOOL;
    case MMS_INTEGER:
        return IEC61850_TYPE_INT32;
    case MMS_UNSIGNED:
        return IEC61850_TYPE_UINT32;
    case MMS_FLOAT:
        return IEC61850_TYPE_FLOAT;
    case MMS_UTC_TIME:
        return IEC61850_TYPE_TIMESTAMP_MS;
    case MMS_BIT_STRING:
        return IEC61850_TYPE_BITSTRING;
    default:
        return IEC61850_TYPE_INT32;
    }
}

static MmsValue* to_mms_value(const IEC61850_Value* inVal)
{
    if (!inVal) return NULL;

    switch (inVal->type)
    {
    case IEC61850_TYPE_BOOL:
        return MmsValue_newBoolean(inVal->v.b ? true : false);
    case IEC61850_TYPE_INT32:
        return MmsValue_newIntegerFromInt32(inVal->v.i32);
    case IEC61850_TYPE_UINT32:
        return MmsValue_newUnsignedFromUint32(inVal->v.u32);
    case IEC61850_TYPE_FLOAT:
        return MmsValue_newFloat(inVal->v.f32);
    case IEC61850_TYPE_DOUBLE:
        return MmsValue_newDouble(inVal->v.f64);
    case IEC61850_TYPE_ENUM:
        return MmsValue_newIntegerFromInt32(inVal->v.e);
    case IEC61850_TYPE_TIMESTAMP_MS:
        return MmsValue_newUtcTimeByMsTime((uint64_t)inVal->v.tsMs);
    case IEC61850_TYPE_BITSTRING:
    {
        MmsValue* mv = MmsValue_newBitString(32);
        if (mv)
            MmsValue_setBitStringFromInteger(mv, inVal->v.bitString);
        return mv;
    }
    default:
        return NULL;
    }
}

static int32_t from_mms_value(MmsValue* mv, IEC61850_VALUE_TYPE expectType, IEC61850_Value* outVal)
{
    if (!mv || !outVal) return IEC61850_RET_INVALID_PARAM;

    memset(outVal, 0, sizeof(*outVal));
    outVal->type = expectType;
    outVal->quality = IEC61850_QUALITY_GOOD;
    outVal->sourceTsMs = now_ms();

    switch (expectType)
    {
    case IEC61850_TYPE_BOOL:
        outVal->v.b = MmsValue_getBoolean(mv) ? 1 : 0;
        break;
    case IEC61850_TYPE_INT32:
        outVal->v.i32 = MmsValue_toInt32(mv);
        break;
    case IEC61850_TYPE_UINT32:
        outVal->v.u32 = MmsValue_toUint32(mv);
        break;
    case IEC61850_TYPE_FLOAT:
        outVal->v.f32 = MmsValue_toFloat(mv);
        break;
    case IEC61850_TYPE_DOUBLE:
        outVal->v.f64 = MmsValue_toDouble(mv);
        break;
    case IEC61850_TYPE_ENUM:
        outVal->v.e = MmsValue_toInt32(mv);
        break;
    case IEC61850_TYPE_TIMESTAMP_MS:
        outVal->v.tsMs = (int64_t)MmsValue_getUtcTimeInMs(mv);
        outVal->sourceTsMs = outVal->v.tsMs;
        break;
    case IEC61850_TYPE_BITSTRING:
        outVal->v.bitString = MmsValue_getBitStringAsInteger(mv);
        break;
    default:
        return IEC61850_RET_UNSUPPORTED;
    }

    return IEC61850_RET_OK;
}

static void report_callback_adapter(void* parameter, ClientReport report)
{
    IEC61850_ClientContext* ctx = (IEC61850_ClientContext*)parameter;
    IEC61850_ReportBatch batch;
    IEC61850_ReportItem* items = NULL;
    char** refs = NULL;
    const char* dsName;
    MmsValue* dsValues;
    int itemCount = 0;
    int i;

    if (!ctx || !ctx->rptCb || !report) return;

    memset(&batch, 0, sizeof(batch));
    batch.rcbRef = ClientReport_getRcbReference(report);
    batch.reportTsMs = ClientReport_hasTimestamp(report) ? (int64_t)ClientReport_getTimestamp(report) : now_ms();

    dsName = ClientReport_getDataSetName(report);
    dsValues = ClientReport_getDataSetValues(report);

    if (dsValues && MmsValue_getType(dsValues) == MMS_ARRAY) {
        itemCount = (int)MmsValue_getArraySize(dsValues);
    }

    if (itemCount > 0) {
        items = (IEC61850_ReportItem*)calloc((size_t)itemCount, sizeof(IEC61850_ReportItem));
        refs = (char**)calloc((size_t)itemCount, sizeof(char*));

        if (items && refs) {
            for (i = 0; i < itemCount; ++i) {
                MmsValue* element = MmsValue_getElement(dsValues, i);
                IEC61850_VALUE_TYPE vt = infer_value_type_from_mms(element);
                char refBuf[256];

                if (dsName && dsName[0] != '\0')
                    _snprintf(refBuf, sizeof(refBuf), "%s[%d]", dsName, i);
                else
                    _snprintf(refBuf, sizeof(refBuf), "reportItem[%d]", i);

                refs[i] = (char*)malloc(strlen(refBuf) + 1);
                if (refs[i]) {
                    strcpy(refs[i], refBuf);
                    items[i].ref = refs[i];
                }

                if (element) {
                    (void)from_mms_value(element, vt, &items[i].value);
                }
            }

            batch.items = items;
            batch.itemCount = itemCount;
        }
    }

    ctx->rptCb(&batch, ctx->rptUser);

    if (refs) {
        for (i = 0; i < itemCount; ++i) {
            free(refs[i]);
        }
        free(refs);
    }

    free(items);
}

// ============================================================
// 生命周期
// ============================================================

int32_t Iec61850_GlobalInit(void)
{
    return IEC61850_RET_OK;
}

int32_t Iec61850_GlobalUninit(void)
{
    return IEC61850_RET_OK;
}

IEC61850_HANDLE Iec61850_CreateClient(void)
{
    IEC61850_ClientContext* ctx = (IEC61850_ClientContext*)malloc(sizeof(IEC61850_ClientContext));
    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(*ctx));
    set_default_conn_param(&ctx->conn);
    ctx->connected = 0;
    ctx->reportEnabled = 0;
    ctx->stateNotified = 0;
    ctx->lastKeepAliveMs = 0;
    ctx->iedConn = NULL;
    ctx->rcb = NULL;
    set_error(ctx, IEC61850_RET_OK, "");

    return (IEC61850_HANDLE)ctx;
}

int32_t Iec61850_DestroyClient(IEC61850_HANDLE h)
{
    IEC61850_ClientContext* ctx;

    if (!is_valid_handle(h)) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);

    Iec61850_Disconnect(h);

    free(ctx);
    return IEC61850_RET_OK;
}

int32_t Iec61850_SetLogCallback(IEC61850_HANDLE h, IEC61850_LOG_CB cb, void* userData)
{
    IEC61850_ClientContext* ctx;

    if (!is_valid_handle(h)) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    ctx->logCb = cb;
    ctx->logUser = userData;
    return IEC61850_RET_OK;
}

int32_t Iec61850_SetStateCallback(IEC61850_HANDLE h, IEC61850_STATE_CB cb, void* userData)
{
    IEC61850_ClientContext* ctx;

    if (!is_valid_handle(h)) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    ctx->stateCb = cb;
    ctx->stateUser = userData;
    return IEC61850_RET_OK;
}

// ============================================================
// 连接管理
// ============================================================

int32_t Iec61850_Connect(IEC61850_HANDLE h, const IEC61850_ConnectParam* param)
{
    IEC61850_ClientContext* ctx;

    if (!is_valid_handle(h) || !param || !param->serverIp) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);

    ctx->conn = *param;
    if (ctx->conn.serverPort <= 0) ctx->conn.serverPort = 102;
    if (ctx->conn.connectTimeoutMs <= 0) ctx->conn.connectTimeoutMs = 3000;
    if (ctx->conn.requestTimeoutMs <= 0) ctx->conn.requestTimeoutMs = 3000;
    if (ctx->conn.reconnectIntervalMs <= 0) ctx->conn.reconnectIntervalMs = 3000;

    return connect_internal(ctx);
}

int32_t Iec61850_Disconnect(IEC61850_HANDLE h)
{
    IEC61850_ClientContext* ctx;

    if (!is_valid_handle(h)) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);

    destroy_report_subscription(ctx);
    destroy_connection_only(ctx);

    set_error(ctx, IEC61850_RET_OK, "disconnect success");
    notify_state(ctx, 0, 0);
    log_msg(ctx, 2, "Iec61850_Disconnect success");
    return IEC61850_RET_OK;
}

int32_t Iec61850_IsConnected(IEC61850_HANDLE h, int32_t* connected)
{
    IEC61850_ClientContext* ctx;

    if (!is_valid_handle(h) || !connected) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);

    if (ctx->iedConn) {
        IedConnectionState st = IedConnection_getState(ctx->iedConn);
        ctx->connected = (st == IED_STATE_CONNECTED) ? 1 : 0;

        if (!ctx->connected && ctx->stateNotified != 0)
            notify_state(ctx, 0, 1);
    }

    *connected = ctx->connected;
    return IEC61850_RET_OK;
}

int32_t Iec61850_KeepAlive(IEC61850_HANDLE h)
{
    IEC61850_ClientContext* ctx;

    if (!is_valid_handle(h)) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    if (!ctx->connected || !ctx->iedConn) {
        return set_error_ret(ctx, IEC61850_RET_NOT_CONNECTED, "not connected");
    }

    if (ensure_connection(ctx) != IEC61850_RET_OK) {
        return set_error_ret(ctx, IEC61850_RET_NOT_CONNECTED, "connection lost");
    }

    ctx->lastKeepAliveMs = now_ms();
    return set_error_ret(ctx, IEC61850_RET_OK, "keepalive ok");
}

// ============================================================
// 读写
// ============================================================

int32_t Iec61850_ReadValue(IEC61850_HANDLE h, const char* ref, IEC61850_VALUE_TYPE type, IEC61850_Value* outVal)
{
    IEC61850_ClientContext* ctx;
    IedClientError err = IED_ERROR_OK;
    MmsValue* mv;
    int32_t ret;
    FunctionalConstraint fc;

    int32_t connRet;

    if (!is_valid_handle(h) || !ref || !outVal) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    connRet = ensure_connection(ctx);
    if (connRet != IEC61850_RET_OK) {
        return set_error_ret(ctx, connRet, "not connected");
    }

    if (!is_supported_value_type(type)) {
        return set_error_ret(ctx, IEC61850_RET_UNSUPPORTED, "unsupported value type");
    }

    fc = infer_fc_from_ref(ref, type, 0);

    mv = IedConnection_readObject(ctx->iedConn, &err, ref, fc);
    if (err != IED_ERROR_OK || !mv) {
        return set_error_ret(ctx, map_ied_err(err), ied_error_to_string(err));
    }

    ret = from_mms_value(mv, type, outVal);
    MmsValue_delete(mv);

    if (ret != IEC61850_RET_OK) {
        return set_error_ret(ctx, ret, "mms value convert failed");
    }

    return set_error_ret(ctx, IEC61850_RET_OK, "read ok");
}

int32_t Iec61850_ReadValues(IEC61850_HANDLE h,
                            const char** refs,
                            const IEC61850_VALUE_TYPE* types,
                            int32_t count,
                            IEC61850_Value* outVals)
{
    int32_t i;
    int32_t ret;

    if (!is_valid_handle(h) || !refs || !types || !outVals || count <= 0) return IEC61850_RET_INVALID_PARAM;

    for (i = 0; i < count; ++i)
    {
        if (!refs[i]) return IEC61850_RET_INVALID_PARAM;

        ret = Iec61850_ReadValue(h, refs[i], types[i], &outVals[i]);
        if (ret != IEC61850_RET_OK) return ret;
    }

    return IEC61850_RET_OK;
}

int32_t Iec61850_WriteValue(IEC61850_HANDLE h, const char* ref, const IEC61850_Value* inVal)
{
    IEC61850_ClientContext* ctx;
    IedClientError err = IED_ERROR_OK;
    MmsValue* mv;
    FunctionalConstraint fc;

    int32_t connRet;

    if (!is_valid_handle(h) || !ref || !inVal) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    connRet = ensure_connection(ctx);
    if (connRet != IEC61850_RET_OK) {
        return set_error_ret(ctx, connRet, "not connected");
    }

    if (!is_supported_value_type(inVal->type)) {
        return set_error_ret(ctx, IEC61850_RET_UNSUPPORTED, "unsupported value type");
    }

    mv = to_mms_value(inVal);
    if (!mv) {
        return set_error_ret(ctx, IEC61850_RET_FAIL, "create mms value failed");
    }

    fc = infer_fc_from_ref(ref, inVal->type, 1);
    IedConnection_writeObject(ctx->iedConn, &err, ref, fc, mv);

    MmsValue_delete(mv);

    if (err != IED_ERROR_OK) {
        return set_error_ret(ctx, map_ied_err(err), ied_error_to_string(err));
    }

    return set_error_ret(ctx, IEC61850_RET_OK, "write ok");
}

// ============================================================
// 遥控
// ============================================================

int32_t Iec61850_Operate(IEC61850_HANDLE h,
                         const char* ctlRef,
                         const IEC61850_Value* ctlVal,
                         IEC61850_CTRL_MODE mode,
                         int32_t timeoutMs)
{
    IEC61850_ClientContext* ctx;
    ControlObjectClient coc;
    MmsValue* mmsCtlVal;
    bool ok = false;

    int32_t connRet;

    (void)timeoutMs;

    if (!is_valid_handle(h) || !ctlRef || !ctlVal) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    connRet = ensure_connection(ctx);
    if (connRet != IEC61850_RET_OK) {
        return set_error_ret(ctx, connRet, "not connected");
    }

    if (mode != IEC61850_CTRL_DIRECT && mode != IEC61850_CTRL_SBO) {
        return set_error_ret(ctx, IEC61850_RET_INVALID_PARAM, "invalid ctrl mode");
    }

    coc = ControlObjectClient_create(ctlRef, ctx->iedConn);
    if (!coc) {
        return set_error_ret(ctx, IEC61850_RET_FAIL, "create control object failed");
    }

    mmsCtlVal = to_mms_value(ctlVal);
    if (!mmsCtlVal) {
        ControlObjectClient_destroy(coc);
        return set_error_ret(ctx, IEC61850_RET_FAIL, "create ctl mms value failed");
    }

    if (mode == IEC61850_CTRL_SBO) {
        if (!ControlObjectClient_selectWithValue(coc, mmsCtlVal)) {
            MmsValue_delete(mmsCtlVal);
            ControlObjectClient_destroy(coc);
            return set_error_ret(ctx, IEC61850_RET_FAIL, "select failed");
        }
    }

    ok = ControlObjectClient_operate(coc, mmsCtlVal, 0);

    MmsValue_delete(mmsCtlVal);
    ControlObjectClient_destroy(coc);

    if (!ok) {
        return set_error_ret(ctx, IEC61850_RET_FAIL, "operate failed");
    }

    return set_error_ret(ctx, IEC61850_RET_OK, "operate ok");
}

int32_t Iec61850_Select(IEC61850_HANDLE h, const char* ctlRef, int32_t timeoutMs)
{
    IEC61850_ClientContext* ctx;
    ControlObjectClient coc;
    bool ok;

    int32_t connRet;

    (void)timeoutMs;

    if (!is_valid_handle(h) || !ctlRef) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    connRet = ensure_connection(ctx);
    if (connRet != IEC61850_RET_OK) {
        return set_error_ret(ctx, connRet, "not connected");
    }

    coc = ControlObjectClient_create(ctlRef, ctx->iedConn);
    if (!coc) {
        return set_error_ret(ctx, IEC61850_RET_FAIL, "create control object failed");
    }

    ok = ControlObjectClient_select(coc);
    ControlObjectClient_destroy(coc);

    if (!ok) {
        return set_error_ret(ctx, IEC61850_RET_FAIL, "select failed");
    }

    return set_error_ret(ctx, IEC61850_RET_OK, "select ok");
}

int32_t Iec61850_Cancel(IEC61850_HANDLE h, const char* ctlRef)
{
    IEC61850_ClientContext* ctx;
    ControlObjectClient coc;
    bool ok;

    int32_t connRet;

    if (!is_valid_handle(h) || !ctlRef) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    connRet = ensure_connection(ctx);
    if (connRet != IEC61850_RET_OK) {
        return set_error_ret(ctx, connRet, "not connected");
    }

    coc = ControlObjectClient_create(ctlRef, ctx->iedConn);
    if (!coc) {
        return set_error_ret(ctx, IEC61850_RET_FAIL, "create control object failed");
    }

    ok = ControlObjectClient_cancel(coc);
    ControlObjectClient_destroy(coc);

    if (!ok) {
        return set_error_ret(ctx, IEC61850_RET_FAIL, "cancel failed");
    }

    return set_error_ret(ctx, IEC61850_RET_OK, "cancel ok");
}

// ============================================================
// 报告订阅
// ============================================================

int32_t Iec61850_SetReportCallback(IEC61850_HANDLE h, IEC61850_REPORT_CB cb, void* userData)
{
    IEC61850_ClientContext* ctx;

    if (!is_valid_handle(h)) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    ctx->rptCb = cb;
    ctx->rptUser = userData;
    return set_error_ret(ctx, IEC61850_RET_OK, "set report callback ok");
}

int32_t Iec61850_EnableReport(IEC61850_HANDLE h, const char* rcbRef)
{
    IEC61850_ClientContext* ctx;
    IedClientError err = IED_ERROR_OK;
    const char* rptId;

    int32_t connRet;

    if (!is_valid_handle(h) || !rcbRef) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    connRet = ensure_connection(ctx);
    if (connRet != IEC61850_RET_OK) {
        return set_error_ret(ctx, connRet, "not connected");
    }

    destroy_report_subscription(ctx);

    ctx->rcb = IedConnection_getRCBValues(ctx->iedConn, &err, rcbRef, NULL);
    if (err != IED_ERROR_OK || !ctx->rcb) {
        return set_error_ret(ctx, map_ied_err(err), ied_error_to_string(err));
    }

    rptId = ClientReportControlBlock_getRptId(ctx->rcb);
    if (!rptId) rptId = "";

    strncpy(ctx->activeRcbRef, rcbRef, sizeof(ctx->activeRcbRef) - 1);
    ctx->activeRcbRef[sizeof(ctx->activeRcbRef) - 1] = '\0';
    strncpy(ctx->activeRptId, rptId, sizeof(ctx->activeRptId) - 1);
    ctx->activeRptId[sizeof(ctx->activeRptId) - 1] = '\0';

    IedConnection_installReportHandler(ctx->iedConn, ctx->activeRcbRef, ctx->activeRptId, report_callback_adapter, ctx);

    ClientReportControlBlock_setRptEna(ctx->rcb, true);
    IedConnection_setRCBValues(ctx->iedConn, &err, ctx->rcb, RCB_ELEMENT_RPT_ENA, true);

    if (err != IED_ERROR_OK) {
        return set_error_ret(ctx, map_ied_err(err), ied_error_to_string(err));
    }

    ctx->reportEnabled = 1;
    return set_error_ret(ctx, IEC61850_RET_OK, "enable report ok");
}

int32_t Iec61850_DisableReport(IEC61850_HANDLE h, const char* rcbRef)
{
    IEC61850_ClientContext* ctx;
    IedClientError err = IED_ERROR_OK;

    int32_t connRet;

    (void)rcbRef;

    if (!is_valid_handle(h) || !rcbRef) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    connRet = ensure_connection(ctx);
    if (connRet != IEC61850_RET_OK) {
        return set_error_ret(ctx, connRet, "not connected");
    }

    if (ctx->rcb) {
        ClientReportControlBlock_setRptEna(ctx->rcb, false);
        IedConnection_setRCBValues(ctx->iedConn, &err, ctx->rcb, RCB_ELEMENT_RPT_ENA, true);
        destroy_report_subscription(ctx);
    }

    if (err != IED_ERROR_OK) {
        return set_error_ret(ctx, map_ied_err(err), ied_error_to_string(err));
    }

    ctx->reportEnabled = 0;
    ctx->activeRcbRef[0] = '\0';
    ctx->activeRptId[0] = '\0';

    return set_error_ret(ctx, IEC61850_RET_OK, "disable report ok");
}

// ============================================================
// 时间同步
// ============================================================

int32_t Iec61850_WriteTimeByRef(IEC61850_HANDLE h, const char* timeRef, const IEC61850_TimeStamp* ts)
{
    IEC61850_ClientContext* ctx;
    IEC61850_Value v;
    int32_t ret;
    int64_t tsMs;

    int32_t connRet;

    if (!is_valid_handle(h) || !timeRef || !ts) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    connRet = ensure_connection(ctx);
    if (connRet != IEC61850_RET_OK) {
        return set_error_ret(ctx, connRet, "not connected");
    }

    ret = timestamp_to_ms(ts, &tsMs);
    if (ret != IEC61850_RET_OK) {
        return set_error_ret(ctx, ret, "invalid timestamp");
    }

    memset(&v, 0, sizeof(v));
    v.type = IEC61850_TYPE_TIMESTAMP_MS;
    v.v.tsMs = tsMs;
    v.quality = IEC61850_QUALITY_GOOD;
    v.sourceTsMs = tsMs;

    return Iec61850_WriteValue(h, timeRef, &v);
}

int32_t Iec61850_TimeSync(IEC61850_HANDLE h, const IEC61850_TimeStamp* ts)
{
    IEC61850_ClientContext* ctx;

    int32_t connRet;

    if (!is_valid_handle(h) || !ts) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    connRet = ensure_connection(ctx);
    if (connRet != IEC61850_RET_OK) {
        return set_error_ret(ctx, connRet, "not connected");
    }

    /* TODO: 根据你的点表配置替换默认对时引用 */
    return Iec61850_WriteTimeByRef(h, "LD0/LLN0.Mod.t", ts);
}

// ============================================================
// 诊断
// ============================================================

const char* Iec61850_GetLastErrorText(IEC61850_HANDLE h)
{
    IEC61850_ClientContext* ctx;

    if (!is_valid_handle(h)) return "invalid handle";

    ctx = as_ctx(h);
    return ctx->lastErrText;
}

int32_t Iec61850_GetLastErrorCode(IEC61850_HANDLE h)
{
    IEC61850_ClientContext* ctx;

    if (!is_valid_handle(h)) return IEC61850_RET_INVALID_PARAM;

    ctx = as_ctx(h);
    return ctx->lastErrCode;
}
