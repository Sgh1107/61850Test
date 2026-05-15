#ifndef TEST_MSG_LOGGER_H
#define TEST_MSG_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

struct TestMsgLoggerConfig
{
    const char* filePath;
    int enableStdout;
};
typedef struct TestMsgLoggerConfig TestMsgLoggerConfig;

typedef struct TestMsgLogger TestMsgLogger;

TestMsgLogger* TestMsgLogger_Create(const TestMsgLoggerConfig* config);
void TestMsgLogger_Destroy(TestMsgLogger* logger);
int TestMsgLogger_IsEnabled(TestMsgLogger* logger);
void TestMsgLogger_LogInfo(TestMsgLogger* logger, const char* message);
void TestMsgLogger_LogTx(TestMsgLogger* logger, const char* operation, const char* details);
void TestMsgLogger_LogRx(TestMsgLogger* logger, const char* operation, const char* details);
void TestMsgLogger_Logf(TestMsgLogger* logger, const char* direction, const char* operation, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
