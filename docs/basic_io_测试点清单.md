# basic_io 测试点清单

本文档根据 `server_example_basic_io` 对应的 CID 文件整理，给出当前示例服务端可优先使用的测试点与推荐测试命令。

---

## 1. 重要说明

### 1.1 逻辑设备名
该 CID 中真实的逻辑设备名是：

```text
GenericIO
```

因此测试引用应优先写成：

```text
GenericIO/...
```

而不是：

```text
LD0/...
```

---

### 1.2 服务端 IP
根据你的实际测试环境，当前服务端 IP 可使用：

```text
192.168.31.68
```

如果后续宿主机 IP 变化，请同步替换命令中的 `--server` 参数。

---

### 1.3 当前 wrapper 能力边界
当前 wrapper / test 程序适合优先测试：

- connect
- keepalive
- read
- write
- operate
- report enable / disable

其中：

- `SPCSO1 ~ SPCSO4` 更推荐用于 **遥控测试**
- 普通 `WriteValue` 是否成功，取决于服务端是否允许直接写该属性

---

## 2. 推荐读取测试点

### 2.1 模拟量 float 点
CID 中明确存在以下测量点：

- `GenericIO/GGIO1.AnIn1.mag.f`
- `GenericIO/GGIO1.AnIn2.mag.f`
- `GenericIO/GGIO1.AnIn3.mag.f`
- `GenericIO/GGIO1.AnIn4.mag.f`

这些点类型为：

```text
FLOAT32
```

#### 推荐命令

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --read-ref "GenericIO/GGIO1.AnIn1.mag.f" --read-type float
```

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --read-ref "GenericIO/GGIO1.AnIn2.mag.f" --read-type float
```

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --read-ref "GenericIO/GGIO1.AnIn3.mag.f" --read-type float
```

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --read-ref "GenericIO/GGIO1.AnIn4.mag.f" --read-type float
```

---

### 2.2 开关状态 bool 点
CID 中明确存在以下布尔状态点：

- `GenericIO/GGIO1.SPCSO1.stVal`
- `GenericIO/GGIO1.SPCSO2.stVal`
- `GenericIO/GGIO1.SPCSO3.stVal`
- `GenericIO/GGIO1.SPCSO4.stVal`

这些点类型为：

```text
BOOLEAN
```

#### 推荐命令

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --read-ref "GenericIO/GGIO1.SPCSO1.stVal" --read-type bool
```

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --read-ref "GenericIO/GGIO1.SPCSO2.stVal" --read-type bool
```

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --read-ref "GenericIO/GGIO1.SPCSO3.stVal" --read-type bool
```

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --read-ref "GenericIO/GGIO1.SPCSO4.stVal" --read-type bool
```

---

### 2.3 其他可读点
还可以尝试读取以下点：

- `GenericIO/LLN0.Mod.stVal`
- `GenericIO/LLN0.Beh.stVal`
- `GenericIO/LLN0.Health.stVal`
- `GenericIO/LPHD1.PhyHealth.stVal`

这些属于枚举型状态量。

#### 示例命令

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --read-ref "GenericIO/LLN0.Mod.stVal" --read-type enum
```

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --read-ref "GenericIO/LLN0.Health.stVal" --read-type enum
```

---

## 3. 推荐写入测试点

> 注意：写入成功与否，不仅取决于点是否存在，还取决于服务端是否允许直接写入该数据属性。

### 3.1 可尝试写入的 bool 点
可尝试以下点：

- `GenericIO/GGIO1.SPCSO1.stVal`
- `GenericIO/GGIO1.SPCSO2.stVal`
- `GenericIO/GGIO1.SPCSO3.stVal`
- `GenericIO/GGIO1.SPCSO4.stVal`

#### 推荐命令

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --write-ref "GenericIO/GGIO1.SPCSO1.stVal" --write-type bool --write-value 1 --skip-read
```

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --write-ref "GenericIO/GGIO1.SPCSO1.stVal" --write-type bool --write-value 0 --skip-read
```

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --write-ref "GenericIO/GGIO1.SPCSO2.stVal" --write-type bool --write-value 1 --skip-read
```

---

### 3.2 写入测试结论建议
如果上述写入失败，请先不要直接判断 wrapper 有问题，可能原因包括：

- 服务端不允许直接写 `stVal`
- 该点设计上更适合通过 `Operate` 控制
- 服务端实现限制

因此对 `basic_io` 来说：

## 更推荐将 `SPCSO1 ~ SPCSO4` 用于遥控测试，而不是优先作为普通写测试点

---

## 4. 推荐遥控测试点

CID 中以下点明确具备控制模型：

- `GenericIO/GGIO1.SPCSO1`
- `GenericIO/GGIO1.SPCSO2`
- `GenericIO/GGIO1.SPCSO3`
- `GenericIO/GGIO1.SPCSO4`

并且 `ctlModel` 为：

```text
direct-with-normal-security
```

这表示：

- 推荐使用 `direct` 模式
- 当前不建议优先测试 `sbo`

---

### 4.1 遥控 SPCSO1

#### 置 1
```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --control-ref "GenericIO/GGIO1.SPCSO1" --control-mode direct --control-type bool --control-value 1 --skip-read
```

#### 置 0
```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --control-ref "GenericIO/GGIO1.SPCSO1" --control-mode direct --control-type bool --control-value 0 --skip-read
```

---

### 4.2 遥控 SPCSO2

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --control-ref "GenericIO/GGIO1.SPCSO2" --control-mode direct --control-type bool --control-value 1 --skip-read
```

---

### 4.3 遥控 SPCSO3

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --control-ref "GenericIO/GGIO1.SPCSO3" --control-mode direct --control-type bool --control-value 1 --skip-read
```

---

### 4.4 遥控 SPCSO4

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --control-ref "GenericIO/GGIO1.SPCSO4" --control-mode direct --control-type bool --control-value 1 --skip-read
```

---

## 5. 报告测试点

CID 中已配置多个报告控制块。

### 5.1 数据集
#### Events
包含：
- `SPCSO1.stVal`
- `SPCSO2.stVal`
- `SPCSO3.stVal`
- `SPCSO4.stVal`

#### Measurements
包含：
- `AnIn1.mag.f`
- `AnIn1.q`
- `AnIn2.mag.f`
- `AnIn2.q`
- `AnIn3.mag.f`
- `AnIn3.q`
- `AnIn4.mag.f`
- `AnIn4.q`

---

### 5.2 ReportControl 名称
CID 中存在：

- `EventsRCB`
- `EventsRCBPreConf`
- `EventsBRCB`
- `EventsBRCBPreConf`
- `EventsIndexed`
- `Measurements`

---

### 5.3 报告引用说明
在不同实现中，RCB 的实际对象引用命名可能与逻辑上的：

- `RP`
- `BR`
- indexed 编号

有关，因此建议优先从非 indexed 的名称开始试。

可优先尝试：

```text
GenericIO/LLN0.RP.EventsRCB
GenericIO/LLN0.BR.EventsBRCB
```

#### 推荐命令

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --report-ref "GenericIO/LLN0.RP.EventsRCB" --skip-read
```

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --report-ref "GenericIO/LLN0.BR.EventsBRCB" --skip-read
```

如果失败，再尝试预配置版本：

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --report-ref "GenericIO/LLN0.RP.EventsRCBPreConf" --skip-read
```

```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --report-ref "GenericIO/LLN0.BR.EventsBRCBPreConf" --skip-read
```

---

## 6. 最推荐的实际测试顺序

### 第一步：基础连接 + float 读取
```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --read-ref "GenericIO/GGIO1.AnIn1.mag.f" --read-type float
```

### 第二步：bool 读取
```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --read-ref "GenericIO/GGIO1.SPCSO1.stVal" --read-type bool
```

### 第三步：普通写 bool（可选）
```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --write-ref "GenericIO/GGIO1.SPCSO1.stVal" --write-type bool --write-value 1 --skip-read
```

### 第四步：direct 遥控
```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --control-ref "GenericIO/GGIO1.SPCSO1" --control-mode direct --control-type bool --control-value 1 --skip-read
```

### 第五步：报告测试
```powershell
iec61850_wrapper_test.exe --server 192.168.31.68 --report-ref "GenericIO/LLN0.RP.EventsRCB" --skip-read
```

---

## 7. 不建议继续使用的测试点写法
以下写法不建议继续使用：

```text
LD0/...
SomeUintPoint.stVal
SomeIntPoint.stVal
```

原因：

- 与当前 `basic_io` 的 CID 模型不匹配
- 容易导致 `object does not exist`

---

## 8. 测试建议总结

### 推荐优先测试
- `GenericIO/GGIO1.AnIn1.mag.f`
- `GenericIO/GGIO1.SPCSO1.stVal`
- `GenericIO/GGIO1.SPCSO1`
- `GenericIO/LLN0.RP.EventsRCB`

### 最适合做遥控的点
- `GenericIO/GGIO1.SPCSO1`
- `GenericIO/GGIO1.SPCSO2`
- `GenericIO/GGIO1.SPCSO3`
- `GenericIO/GGIO1.SPCSO4`

### 最适合做普通读取的点
- `GenericIO/GGIO1.AnIn1.mag.f`
- `GenericIO/GGIO1.AnIn2.mag.f`
- `GenericIO/GGIO1.AnIn3.mag.f`
- `GenericIO/GGIO1.AnIn4.mag.f`
- `GenericIO/GGIO1.SPCSO1.stVal`

---

## 9. 相关文档
- 项目说明：`README.md`
- 测试程序说明：`docs/test_program_usage.md`
- 测试记录清单：`docs/测试记录清单.md`
