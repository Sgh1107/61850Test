#include "test_msg_logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>

struct TestMsgLogger
{
    FILE* fp;
    int enableStdout;
    HANDLE console;
    WORD defaultAttr;
};

static void logger_get_timestamp(char* buffer, size_t bufferSize)
{
    SYSTEMTIME st;

    if (!buffer || bufferSize == 0) return;

    GetLocalTime(&st);
    _snprintf(buffer, bufferSize,
        "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        (int)st.wYear,
        (int)st.wMonth,
        (int)st.wDay,
        (int)st.wHour,
        (int)st.wMinute,
        (int)st.wSecond,
        (int)st.wMilliseconds);
    buffer[bufferSize - 1] = '\0';
}

static WORD logger_get_color(const char* direction, const char* operation, const char* details, WORD defaultAttr)
{
    const char* detailText = details ? details : "";

    if (direction) {
        if (strcmp(direction, "TX") == 0) {
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        }
        else if (strcmp(direction, "INFO") == 0) {
            return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        }
        else if (strcmp(direction, "RX") == 0) {
            if (strstr(detailText, "ret=0") != NULL || strstr(detailText, "success") != NULL || strstr(detailText, "connected=1") != NULL) {
                return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            }
            if (strstr(detailText, "ret=-") != NULL || strstr(detailText, "failed") != NULL || strstr(detailText, "denied") != NULL || strstr(detailText, "not exist") != NULL) {
                return FOREGROUND_RED | FOREGROUND_INTENSITY;
            }
            if (operation && (strcmp(operation, "REPORT") == 0 || strcmp(operation, "REPORT_ITEM") == 0)) {
                return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            }
            return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        }
    }

    return defaultAttr;
}

static void logger_write_line(TestMsgLogger* logger, const char* direction, const char* operation, const char* details)
{
    char ts[64];
    const char* dirText = direction ? direction : "MSG";
    const char* opText = operation ? operation : "UNKNOWN";
    const char* detailText = details ? details : "";

    if (!logger) return;

    logger_get_timestamp(ts, sizeof(ts));

    if (logger->enableStdout) {
        WORD color = logger_get_color(direction, operation, details, logger->defaultAttr);
        if (logger->console) {
            SetConsoleTextAttribute(logger->console, color);
        }
        printf("[MSG][%s][%s][%s] %s\n", ts, dirText, opText, detailText);
        if (logger->console) {
            SetConsoleTextAttribute(logger->console, logger->defaultAttr);
        }
    }

    if (logger->fp) {
        fprintf(logger->fp, "[MSG][%s][%s][%s] %s\n", ts, dirText, opText, detailText);
        fflush(logger->fp);
    }
}

TestMsgLogger* TestMsgLogger_Create(const TestMsgLoggerConfig* config)
{
    TestMsgLogger* logger = (TestMsgLogger*)malloc(sizeof(TestMsgLogger));

    if (!logger) return NULL;

    memset(logger, 0, sizeof(*logger));

    if (config) {
        CONSOLE_SCREEN_BUFFER_INFO csbi;

        logger->enableStdout = config->enableStdout;
        logger->console = GetStdHandle(STD_OUTPUT_HANDLE);
        logger->defaultAttr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

        if (logger->console == INVALID_HANDLE_VALUE) {
            logger->console = NULL;
        }
        else if (GetConsoleScreenBufferInfo(logger->console, &csbi)) {
            logger->defaultAttr = csbi.wAttributes;
        }

        if (config->filePath && config->filePath[0] != '\0') {
            logger->fp = fopen(config->filePath, "a");
        }
    }

    return logger;
}

void TestMsgLogger_Destroy(TestMsgLogger* logger)
{
    if (!logger) return;

    if (logger->fp) {
        fclose(logger->fp);
        logger->fp = NULL;
    }

    free(logger);
}

int TestMsgLogger_IsEnabled(TestMsgLogger* logger)
{
    if (!logger) return 0;

    return (logger->enableStdout || logger->fp) ? 1 : 0;
}

void TestMsgLogger_LogInfo(TestMsgLogger* logger, const char* message)
{
    logger_write_line(logger, "INFO", "INFO", message);
}

void TestMsgLogger_LogTx(TestMsgLogger* logger, const char* operation, const char* details)
{
    logger_write_line(logger, "TX", operation, details);
}

void TestMsgLogger_LogRx(TestMsgLogger* logger, const char* operation, const char* details)
{
    logger_write_line(logger, "RX", operation, details);
}

void TestMsgLogger_Logf(TestMsgLogger* logger, const char* direction, const char* operation, const char* fmt, ...)
{
    char buffer[1024];
    va_list ap;

    if (!logger || !fmt) return;

    va_start(ap, fmt);
    _vsnprintf(buffer, sizeof(buffer), fmt, ap);
    buffer[sizeof(buffer) - 1] = '\0';
    va_end(ap);

    logger_write_line(logger, direction, operation, buffer);
}
