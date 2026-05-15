# 测试程序使用说明

本文档说明 `iec61850_wrapper_test.exe` 的用途、参数、典型命令以及使用建议。

---

## 1. 程序位置
编译成功后，测试程序通常位于：

```text
build/Release/iec61850_wrapper_test.exe
```

---

## 2. 程序用途
该测试程序用于快速验证 wrapper 动态库的基本功能是否正常，包括：

- 连接 IED
- 检查连接状态
- KeepAlive
- 读取点值
- 写入点值
- 遥控操作
- 报告使能/撤销

默认情况下，程序更偏向**安全测试**：

- 默认执行连接与读取
- 默认**不执行写入、遥控、报告订阅**
- 只有在显式传入相关参数时，才会执行这些操作

这样可以降低误操作风险。

---

## 3. 获取帮助
先执行：

```powershell
.\build\Release\iec61850_wrapper_test.exe --help
```

程序会打印支持的命令行参数。

---

## 4. 参数总览

```text
--server <ip>
--port <port>
--read-ref <ref>
--read-type <bool|int32|uint32|float|double|enum|timestamp|bitstring>
--write-ref <ref>
--write-type <bool|int32|uint32|float|double|enum|timestamp|bitstring>
--write-value <value>
--control-ref <ref>
--control-mode <direct|sbo>
--control-type <bool|int32|uint32|float|double|enum|timestamp|bitstring>
--control-value <value>
--report-ref <ref>
--msg-log-file <path>
--msg-log-stdout
--skip-read
--skip-write
--skip-control
--skip-report
--help
```

---

## 5. 参数详细说明

### 5.1 连接参数

#### `--server <ip>`
指定目标 IED 的 IP 地址。

示例：

```powershell
--server 192.168.1.100
```

如果不传，默认值为：

```text
127.0.0.1
```

---

#### `--port <port>`
指定 TCP 端口，IEC 61850 MMS 通常为 `102`。

示例：

```powershell
--port 102
```

默认值：

```text
102
```

---

### 5.2 读取参数

#### `--read-ref <ref>`
指定要读取的对象引用。

示例：

```powershell
--read-ref "LD0/MMXU1.TotW.mag.f"
```

---

#### `--read-type <type>`
指定读取值的期望类型。

支持值：

- `bool`
- `int32`
- `uint32`
- `float`
- `double`
- `enum`
- `timestamp`
- `bitstring`

示例：

```powershell
--read-type float
```

默认读取行为是开启的，除非你加上：

```powershell
--skip-read
```

---

### 5.3 写入参数

#### `--write-ref <ref>`
指定写入对象引用。

示例：

```powershell
--write-ref "LD0/GGIO1.SPCSO1.stVal"
```

---

#### `--write-type <type>`
指定写入值类型。

支持值：

- `bool`
- `int32`
- `uint32`
- `float`
- `double`
- `enum`
- `timestamp`
- `bitstring`

示例：

```powershell
--write-type bool
```

---

#### `--write-value <value>`
指定写入值。

不同类型写法示例：

##### bool
```powershell
--write-value 1
```
或
```powershell
--write-value true
```

##### int32
```powershell
--write-value -10
```

##### uint32
```powershell
--write-value 100
```

##### float
```powershell
--write-value 12.34
```

##### double
```powershell
--write-value 123.456789
```

##### enum
```powershell
--write-value 2
```

##### timestamp
时间戳使用**毫秒时间戳**：

```powershell
--write-value 1718000000000
```

##### bitstring
可使用十进制或十六进制：

```powershell
--write-value 15
```
或
```powershell
--write-value 0x0F
```

---

### 5.4 遥控参数

#### `--control-ref <ref>`
指定控制对象引用。

示例：

```powershell
--control-ref "LD0/CSWI1.Pos"
```

---

#### `--control-mode <direct|sbo>`
指定控制模式：

- `direct`：直接操作
- `sbo`：Select Before Operate

示例：

```powershell
--control-mode direct
```

---

#### `--control-type <type>`
指定控制值类型，支持：

- `bool`
- `int32`
- `uint32`
- `float`
- `double`
- `enum`
- `timestamp`
- `bitstring`

示例：

```powershell
--control-type bool
```

---

#### `--control-value <value>`
指定控制值。

示例：

```powershell
--control-value 1
```

---

### 5.5 报告参数

#### `--report-ref <ref>`
指定报告控制块引用。

示例：

```powershell
--report-ref "LD0/LLN0.RP.RP01"
```

传入该参数后，程序会尝试：

1. 使能报告
2. 再撤销报告

注意：
当前测试程序主要用于验证报告相关 API 是否可正常调用，不是长期在线报告监听工具。

---

### 5.6 消息记录参数

#### `--msg-log-file <path>`
将测试过程中的 TX/RX 风格消息记录追加写入到指定文件。

这些记录不是底层抓包得到的原始二进制网络帧，而是测试程序按操作过程输出的通信记录，适合做联调、问题排查和后续导入自定义 viewer。

示例：

```powershell
--msg-log-file msg.log
```

---

#### `--msg-log-stdout`
将同样的消息记录额外输出到当前终端。

启用后，终端中会看到类似：

```text
[MSG][2025-08-08 12:00:00.123][TX][Connect] server=192.168.31.68 port=102 connectTimeoutMs=3000 requestTimeoutMs=3000
[MSG][2025-08-08 12:00:00.456][RX][Connect] ret=0 errCode=0 errText=connect success
```

如果同时指定 `--msg-log-file`，则会同时输出到终端并写入文件。

---

### 5.7 跳过参数

#### `--skip-read`
跳过读取测试。

#### `--skip-write`
跳过写入测试。

#### `--skip-control`
跳过遥控测试。

#### `--skip-report`
跳过报告测试。

这些参数适合用于组合测试，避免不必要操作。

---

## 6. 默认行为说明
如果你只执行：

```powershell
.\build\Release\iec61850_wrapper_test.exe --server 192.168.1.100
```

则程序会：

1. 初始化 wrapper
2. 创建 client
3. 连接 IED
4. 检查连接状态
5. 执行 keepalive
6. 执行一次默认读取
7. 断开连接
8. 销毁 client
9. 反初始化 wrapper

默认不会进行：

- 写入
- 遥控
- 报告测试

---

## 7. 常见使用示例

### 7.1 只测试连接和默认读取

```powershell
.\build\Release\iec61850_wrapper_test.exe --server 192.168.1.100
```

---

### 7.2 读取指定 float 点

```powershell
.\build\Release\iec61850_wrapper_test.exe --server 192.168.1.100 --read-ref "LD0/MMXU1.TotW.mag.f" --read-type float
```

---

### 7.3 读取 bool 点

```powershell
.\build\Release\iec61850_wrapper_test.exe --server 192.168.1.100 --read-ref "LD0/GGIO1.SPCSO1.stVal" --read-type bool
```

---

### 7.4 写 bool 值

```powershell
.\build\Release\iec61850_wrapper_test.exe --server 192.168.1.100 --write-ref "LD0/GGIO1.SPCSO1.stVal" --write-type bool --write-value 1 --skip-read
```

---

### 7.5 写 float 值

```powershell
.\build\Release\iec61850_wrapper_test.exe --server 192.168.1.100 --write-ref "LD0/SomeNode.mag.f" --write-type float --write-value 12.5 --skip-read
```

---

### 7.6 执行 direct 遥控

```powershell
.\build\Release\iec61850_wrapper_test.exe --server 192.168.1.100 --control-ref "LD0/CSWI1.Pos" --control-mode direct --control-type bool --control-value 1 --skip-read
```

---

### 7.7 执行 SBO 遥控

```powershell
.\build\Release\iec61850_wrapper_test.exe --server 192.168.1.100 --control-ref "LD0/CSWI1.Pos" --control-mode sbo --control-type bool --control-value 1 --skip-read
```

---

### 7.8 测试报告使能/撤销

```powershell
.\build\Release\iec61850_wrapper_test.exe --server 192.168.1.100 --report-ref "LD0/LLN0.RP.RP01" --skip-read
```

---

### 7.9 同时读写组合测试

```powershell
.\build\Release\iec61850_wrapper_test.exe --server 192.168.1.100 --read-ref "LD0/MMXU1.TotW.mag.f" --read-type float --write-ref "LD0/GGIO1.SPCSO1.stVal" --write-type bool --write-value 1
```

---

### 7.10 输出消息记录到文件

```powershell
.\build\Release\iec61850_wrapper_test.exe --server 192.168.1.100 --read-ref "LD0/MMXU1.TotW.mag.f" --read-type float --msg-log-file msg.log
```

---

### 7.11 同时输出消息记录到终端和文件

```powershell
.\build\Release\iec61850_wrapper_test.exe --server 192.168.1.100 --read-ref "LD0/MMXU1.TotW.mag.f" --read-type float --msg-log-file msg.log --msg-log-stdout
```

---

## 8. 输出说明
程序执行时通常会输出以下类型信息：

### 8.1 日志回调
示例：

```text
[LOG][2] Iec61850_Connect success
```

---

### 8.2 状态回调
示例：

```text
[STATE] connected=1 reason=0
```

可能含义：

- `connected=1`：已连接
- `connected=0`：已断开

---

### 8.3 接口返回值
示例：

```text
[Connect] ret=0 errCode=0 errText=connect success
```

其中：

- `ret=0` 通常表示成功
- `errCode` 为 wrapper 内部记录的最后错误码
- `errText` 为文本描述

---

### 8.4 读取结果
示例：

```text
value(float)=123.456001
sourceTsMs=1718000000000
```

---

### 8.5 报告回调
示例：

```text
[REPORT] rcb=LD0/LLN0.RP.RP01 itemCount=2 ts=1718000000000
  - ref=someDataset[0] type=4 quality=0
  - ref=someDataset[1] type=1 quality=0
```

注意：
当前 wrapper 中报告项的 `ref` 可能是按数据集名和索引合成的显示值，主要用于测试和调试。

---

### 8.6 消息记录输出
启用 `--msg-log-file` 或 `--msg-log-stdout` 后，程序会输出类似 Modbus 工具中 TX/RX 风格的通信记录。

示例：

```text
[MSG][2025-08-08 12:00:00.123][TX][Connect] server=192.168.31.68 port=102 connectTimeoutMs=3000 requestTimeoutMs=3000
[MSG][2025-08-08 12:00:00.456][RX][Connect] ret=0 errCode=0 errText=connect success
[MSG][2025-08-08 12:00:00.500][TX][ReadValue] ref=simpleIOGenericIO/GGIO1.AnIn1.mag.f type=4
[MSG][2025-08-08 12:00:00.520][RX][ReadValue] ret=0 errCode=0 errText=read ok
[MSG][2025-08-08 12:00:00.521][RX][ReadValue] ref=simpleIOGenericIO/GGIO1.AnIn1.mag.f value=float:0.000000 sourceTsMs=1778812693000
```

字段含义：

- 第一个时间字段：本地记录时间
- `TX`：测试程序发起的操作请求
- `RX`：测试程序收到的返回、状态、日志或报告回调
- `Connect / ReadValue / WriteValue / Operate / EnableReport`：对应操作名称

注意：
这里记录的是**操作级消息日志**，不是底层原始 MMS 二进制抓包数据。

---

## 9. 使用建议

### 9.1 先从只读测试开始
建议第一次先执行：

```powershell
.\build\Release\iec61850_wrapper_test.exe --server <IED_IP>
```

先确认：

- 网络联通
- 端口正确
- IED 可响应 MMS 请求

---

### 9.2 写入和遥控前先确认点位
写入和遥控都可能影响现场设备状态。使用前请务必确认：

- 点位引用正确
- 功能约束正确
- 设备允许当前用户执行该操作
- 在安全测试环境中执行

---

### 9.3 报告引用必须准确
报告控制块引用应与设备实际配置一致，例如：

```text
LD0/LLN0.RP.RP01
```

如果引用错误，可能会返回：

- not found
- access denied
- dataset mismatch

---

### 9.4 建议联调时开启消息记录
如果你希望像 Modbus 调试工具那样查看 TX/RX 风格的通信过程，建议在测试时增加：

```powershell
--msg-log-file msg.log --msg-log-stdout
```

这样既能在终端实时看消息，也能保留一份文件用于后续分析。

---

## 10. 常见问题

### 10.1 Connect 失败
请检查：

- IP 地址是否正确
- 端口是否正确，通常是 `102`
- 目标 IED 是否在线
- 本机与设备网络是否互通
- 防火墙是否阻止连接

---

### 10.2 Read/Write/Operate 返回 not connected
表示连接未建立或已断开。请先确认：

- `Connect` 是否成功
- 设备是否主动断开
- 网络是否稳定

---

### 10.3 报告使能失败
可能原因包括：

- `report-ref` 填写错误
- 设备不支持该 RCB
- 数据集不匹配
- 权限不足
- 设备对客户端连接数有限制

---

### 10.4 类型不匹配
如果读写时类型和设备实际数据类型不一致，可能导致：

- 读取转换失败
- 写入失败
- 遥控失败

建议先通过设备文档或点表确认数据类型。

---

## 11. 相关文件

- 对外头文件：`inc/iec61850_wrapper.h`
- wrapper 实现：`src/iec61850_wrapper.cpp`
- 测试程序源码：`src/iec61850_wrapper_test.cpp`
- 项目说明：`README.md`
