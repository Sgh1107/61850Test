#include "iec61850_wrapper.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

static HANDLE g_console = NULL;
static WORD g_defaultAttr = 0;

static void init_console_color(void)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    g_console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (g_console != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(g_console, &csbi)) {
        g_defaultAttr = csbi.wAttributes;
    }
    else {
        g_console = NULL;
        g_defaultAttr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
}

static void set_console_color(WORD attr)
{
    if (g_console) {
        SetConsoleTextAttribute(g_console, attr);
    }
}

static void reset_console_color(void)
{
    set_console_color(g_defaultAttr);
}

static void print_status_prefix(const char* tag, int ok)
{
    set_console_color(ok ? (FOREGROUND_GREEN | FOREGROUND_INTENSITY) : (FOREGROUND_RED | FOREGROUND_INTENSITY));
    printf("[%s] ", tag);
    reset_console_color();
}

static void print_usage(const char* exe)
{
    printf("Usage:\n");
    printf("  %s [options]\n", exe ? exe : "iec61850_wrapper_test.exe");
    printf("Options:\n");
    printf("  --server <ip>\n");
    printf("  --port <port>\n");
    printf("  --read-ref <ref>\n");
    printf("  --read-type <bool|int32|uint32|float|double|enum|timestamp|bitstring>\n");
    printf("  --write-ref <ref>\n");
    printf("  --write-type <bool|int32|uint32|float|double|enum|timestamp|bitstring>\n");
    printf("  --write-value <value>\n");
    printf("  --control-ref <ref>\n");
    printf("  --control-mode <direct|sbo>\n");
    printf("  --control-type <bool|int32|uint32|float|double|enum|timestamp|bitstring>\n");
    printf("  --control-value <value>\n");
    printf("  --report-ref <ref>\n");
    printf("  --skip-read | --skip-write | --skip-control | --skip-report\n");
    printf("  --help\n");
}

static int equals_ignore_case(const char* a, const char* b)
{
    if (!a || !b) return 0;

    while (*a && *b) {
        char ca = *a;
        char cb = *b;

        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return 0;
        ++a;
        ++b;
    }

    return (*a == '\0' && *b == '\0') ? 1 : 0;
}

static int parse_type(const char* text, IEC61850_VALUE_TYPE* outType)
{
    if (!text || !outType) return 0;

    if (equals_ignore_case(text, "bool")) *outType = IEC61850_TYPE_BOOL;
    else if (equals_ignore_case(text, "int32")) *outType = IEC61850_TYPE_INT32;
    else if (equals_ignore_case(text, "uint32")) *outType = IEC61850_TYPE_UINT32;
    else if (equals_ignore_case(text, "float")) *outType = IEC61850_TYPE_FLOAT;
    else if (equals_ignore_case(text, "double")) *outType = IEC61850_TYPE_DOUBLE;
    else if (equals_ignore_case(text, "enum")) *outType = IEC61850_TYPE_ENUM;
    else if (equals_ignore_case(text, "timestamp")) *outType = IEC61850_TYPE_TIMESTAMP_MS;
    else if (equals_ignore_case(text, "bitstring")) *outType = IEC61850_TYPE_BITSTRING;
    else return 0;

    return 1;
}

static int parse_ctrl_mode(const char* text, IEC61850_CTRL_MODE* outMode)
{
    if (!text || !outMode) return 0;

    if (equals_ignore_case(text, "direct")) *outMode = IEC61850_CTRL_DIRECT;
    else if (equals_ignore_case(text, "sbo")) *outMode = IEC61850_CTRL_SBO;
    else return 0;

    return 1;
}

static int fill_value_from_text(IEC61850_VALUE_TYPE type, const char* text, IEC61850_Value* outVal)
{
    if (!text || !outVal) return 0;

    memset(outVal, 0, sizeof(*outVal));
    outVal->type = type;
    outVal->quality = IEC61850_QUALITY_GOOD;

    switch (type)
    {
    case IEC61850_TYPE_BOOL:
        if (equals_ignore_case(text, "1") || equals_ignore_case(text, "true") || equals_ignore_case(text, "on"))
            outVal->v.b = 1;
        else if (equals_ignore_case(text, "0") || equals_ignore_case(text, "false") || equals_ignore_case(text, "off"))
            outVal->v.b = 0;
        else
            return 0;
        break;
    case IEC61850_TYPE_INT32:
        outVal->v.i32 = (int32_t)strtol(text, NULL, 0);
        break;
    case IEC61850_TYPE_UINT32:
    case IEC61850_TYPE_BITSTRING:
        outVal->v.u32 = (uint32_t)strtoul(text, NULL, 0);
        if (type == IEC61850_TYPE_BITSTRING)
            outVal->v.bitString = outVal->v.u32;
        break;
    case IEC61850_TYPE_FLOAT:
        outVal->v.f32 = (float)atof(text);
        break;
    case IEC61850_TYPE_DOUBLE:
        outVal->v.f64 = atof(text);
        break;
    case IEC61850_TYPE_ENUM:
        outVal->v.e = (int32_t)strtol(text, NULL, 0);
        break;
    case IEC61850_TYPE_TIMESTAMP_MS:
        outVal->v.tsMs = _strtoi64(text, NULL, 0);
        outVal->sourceTsMs = outVal->v.tsMs;
        break;
    default:
        return 0;
    }

    return 1;
}

static void print_value(const IEC61850_Value* value)
{
    if (!value) return;

    set_console_color(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    switch (value->type)
    {
    case IEC61850_TYPE_BOOL:
        printf("value(bool)=%d\n", (int)value->v.b);
        break;
    case IEC61850_TYPE_INT32:
        printf("value(int32)=%d\n", (int)value->v.i32);
        break;
    case IEC61850_TYPE_UINT32:
        printf("value(uint32)=%u\n", (unsigned int)value->v.u32);
        break;
    case IEC61850_TYPE_FLOAT:
        printf("value(float)=%f\n", value->v.f32);
        break;
    case IEC61850_TYPE_DOUBLE:
        printf("value(double)=%.12f\n", value->v.f64);
        break;
    case IEC61850_TYPE_ENUM:
        printf("value(enum)=%d\n", (int)value->v.e);
        break;
    case IEC61850_TYPE_TIMESTAMP_MS:
        printf("value(timestamp_ms)=%lld\n", (long long)value->v.tsMs);
        break;
    case IEC61850_TYPE_BITSTRING:
        printf("value(bitstring)=0x%08X\n", (unsigned int)value->v.bitString);
        break;
    default:
        printf("value(unknown)\n");
        break;
    }

    reset_console_color();
}

static void test_log_cb(int32_t level, const char* msg, void* userData)
{
    (void)userData;

    if (level >= 2)
        set_console_color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    else if (level == 1)
        set_console_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    else
        set_console_color(FOREGROUND_RED | FOREGROUND_INTENSITY);

    printf("[LOG][%d] %s\n", (int)level, msg ? msg : "");
    reset_console_color();
}

static void test_state_cb(int32_t connected, int32_t reason, void* userData)
{
    (void)userData;
    set_console_color(connected ? (FOREGROUND_GREEN | FOREGROUND_INTENSITY) : (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY));
    printf("[STATE] connected=%d reason=%d\n", (int)connected, (int)reason);
    reset_console_color();
}

static void test_report_cb(const IEC61850_ReportBatch* batch, void* userData)
{
    int32_t i;
    (void)userData;

    set_console_color(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    if (!batch) {
        printf("[REPORT] null batch\n");
        reset_console_color();
        return;
    }

    printf("[REPORT] rcb=%s itemCount=%d ts=%lld\n",
        batch->rcbRef ? batch->rcbRef : "",
        (int)batch->itemCount,
        (long long)batch->reportTsMs);

    for (i = 0; i < batch->itemCount; ++i)
    {
        const IEC61850_ReportItem* item = &batch->items[i];
        if (!item) continue;

        printf("  - ref=%s type=%d quality=%d\n",
            item->ref ? item->ref : "",
            (int)item->value.type,
            (int)item->value.quality);
    }

    reset_console_color();
}

static void print_ret(const char* tag, int32_t ret, IEC61850_HANDLE h)
{
    print_status_prefix(tag, (ret == IEC61850_RET_OK));
    printf("ret=%d errCode=%d errText=%s\n",
        (int)ret,
        (int)Iec61850_GetLastErrorCode(h),
        Iec61850_GetLastErrorText(h));
}

int main(int argc, char** argv)
{
    int32_t ret;
    int32_t connected = 0;
    int argi;
    IEC61850_HANDLE h = NULL;
    IEC61850_ConnectParam conn;
    IEC61850_Value val;
    IEC61850_Value writeVal;
    IEC61850_Value controlVal;
    const char* serverIp = "127.0.0.1";
    int serverPort = 102;
    const char* readRef = "LD0/MMXU1.TotW.mag.f";
    IEC61850_VALUE_TYPE readType = IEC61850_TYPE_FLOAT;
    const char* writeRef = "LD0/GGIO1.SPCSO1.stVal";
    IEC61850_VALUE_TYPE writeType = IEC61850_TYPE_BOOL;
    const char* writeValueText = "1";
    const char* controlRef = "LD0/CSWI1.Pos";
    IEC61850_CTRL_MODE controlMode = IEC61850_CTRL_DIRECT;
    IEC61850_VALUE_TYPE controlType = IEC61850_TYPE_BOOL;
    const char* controlValueText = "1";
    const char* reportRef = "LD0/LLN0.RP.RP01";
    int doRead = 1;
    int doWrite = 0;
    int doControl = 0;
    int doReport = 0;

    init_console_color();

    for (argi = 1; argi < argc; ++argi)
    {
        const char* arg = argv[argi];

        if (strcmp(arg, "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(arg, "--server") == 0 && argi + 1 < argc) {
            serverIp = argv[++argi];
        }
        else if (strcmp(arg, "--port") == 0 && argi + 1 < argc) {
            serverPort = atoi(argv[++argi]);
        }
        else if (strcmp(arg, "--read-ref") == 0 && argi + 1 < argc) {
            readRef = argv[++argi];
        }
        else if (strcmp(arg, "--read-type") == 0 && argi + 1 < argc) {
            if (!parse_type(argv[++argi], &readType)) {
                printf("invalid --read-type\n");
                return 2;
            }
        }
        else if (strcmp(arg, "--write-ref") == 0 && argi + 1 < argc) {
            writeRef = argv[++argi];
            doWrite = 1;
        }
        else if (strcmp(arg, "--write-type") == 0 && argi + 1 < argc) {
            if (!parse_type(argv[++argi], &writeType)) {
                printf("invalid --write-type\n");
                return 2;
            }
            doWrite = 1;
        }
        else if (strcmp(arg, "--write-value") == 0 && argi + 1 < argc) {
            writeValueText = argv[++argi];
            doWrite = 1;
        }
        else if (strcmp(arg, "--control-ref") == 0 && argi + 1 < argc) {
            controlRef = argv[++argi];
            doControl = 1;
        }
        else if (strcmp(arg, "--control-mode") == 0 && argi + 1 < argc) {
            if (!parse_ctrl_mode(argv[++argi], &controlMode)) {
                printf("invalid --control-mode\n");
                return 2;
            }
            doControl = 1;
        }
        else if (strcmp(arg, "--control-type") == 0 && argi + 1 < argc) {
            if (!parse_type(argv[++argi], &controlType)) {
                printf("invalid --control-type\n");
                return 2;
            }
            doControl = 1;
        }
        else if (strcmp(arg, "--control-value") == 0 && argi + 1 < argc) {
            controlValueText = argv[++argi];
            doControl = 1;
        }
        else if (strcmp(arg, "--report-ref") == 0 && argi + 1 < argc) {
            reportRef = argv[++argi];
            doReport = 1;
        }
        else if (strcmp(arg, "--skip-read") == 0) {
            doRead = 0;
        }
        else if (strcmp(arg, "--skip-write") == 0) {
            doWrite = 0;
        }
        else if (strcmp(arg, "--skip-control") == 0) {
            doControl = 0;
        }
        else if (strcmp(arg, "--skip-report") == 0) {
            doReport = 0;
        }
        else {
            printf("unknown or incomplete option: %s\n", arg);
            print_usage(argv[0]);
            return 2;
        }
    }

    if (doWrite && !fill_value_from_text(writeType, writeValueText, &writeVal)) {
        printf("invalid write value\n");
        return 2;
    }

    if (doControl && !fill_value_from_text(controlType, controlValueText, &controlVal)) {
        printf("invalid control value\n");
        return 2;
    }

    set_console_color(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("====== IEC61850 wrapper cmd Test start ======\n");
    reset_console_color();
    printf("serverIp=%s\n", serverIp);
    printf("serverPort=%d\n", serverPort);
    printf("doRead=%d doWrite=%d doControl=%d doReport=%d\n", doRead, doWrite, doControl, doReport);

    ret = Iec61850_GlobalInit();
    print_status_prefix("GlobalInit", (ret == IEC61850_RET_OK));
    printf("ret=%d\n", (int)ret);

    h = Iec61850_CreateClient();
    if (!h) {
        set_console_color(FOREGROUND_RED | FOREGROUND_INTENSITY);
        printf("CreateClient failed\n");
        reset_console_color();
        return 1;
    }

    Iec61850_SetLogCallback(h, test_log_cb, NULL);
    Iec61850_SetStateCallback(h, test_state_cb, NULL);
    Iec61850_SetReportCallback(h, test_report_cb, NULL);

    memset(&conn, 0, sizeof(conn));
    conn.serverIp = serverIp;
    conn.serverPort = serverPort;
    conn.connectTimeoutMs = 3000;
    conn.requestTimeoutMs = 3000;
    conn.autoReconnect = 0;
    conn.reconnectIntervalMs = 3000;

    ret = Iec61850_Connect(h, &conn);
    print_ret("Connect", ret, h);

    if (ret == IEC61850_RET_OK) {
        ret = Iec61850_IsConnected(h, &connected);
        print_status_prefix("IsConnected", (ret == IEC61850_RET_OK && connected));
        printf("ret=%d connected=%d\n", (int)ret, (int)connected);

        ret = Iec61850_KeepAlive(h);
        print_ret("KeepAlive", ret, h);

        if (doRead) {
            memset(&val, 0, sizeof(val));
            ret = Iec61850_ReadValue(h, readRef, readType, &val);
            print_ret("ReadValue", ret, h);
            if (ret == IEC61850_RET_OK) {
                print_value(&val);
                set_console_color(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                printf("sourceTsMs=%lld\n", (long long)val.sourceTsMs);
                reset_console_color();
            }
        }

        if (doWrite) {
            ret = Iec61850_WriteValue(h, writeRef, &writeVal);
            print_ret("WriteValue", ret, h);
        }

        if (doControl) {
            ret = Iec61850_Operate(h, controlRef, &controlVal, controlMode, 3000);
            print_ret("Operate", ret, h);
        }

        if (doReport) {
            ret = Iec61850_EnableReport(h, reportRef);
            print_ret("EnableReport", ret, h);

            ret = Iec61850_DisableReport(h, reportRef);
            print_ret("DisableReport", ret, h);
        }
    }

    ret = Iec61850_Disconnect(h);
    print_ret("Disconnect", ret, h);

    ret = Iec61850_DestroyClient(h);
    print_status_prefix("DestroyClient", (ret == IEC61850_RET_OK));
    printf("ret=%d\n", (int)ret);

    ret = Iec61850_GlobalUninit();
    print_status_prefix("GlobalUninit", (ret == IEC61850_RET_OK));
    printf("ret=%d\n", (int)ret);

    set_console_color(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("====== IEC61850 wrapper cmd Test end ======\n");
    reset_console_color();
    return 0;
}
