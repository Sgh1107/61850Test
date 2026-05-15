# IEC61850 Wrapper

## 项目简介
本项目基于 **libiec61850 1.5.3** 静态库进行二次封装，最终生成一个可供上层调用的 Windows 动态库：

- `iec61850_wrapper.dll`
- `iec61850_wrapper.lib`

同时提供一个测试程序：

- `iec61850_wrapper_test.exe`

当前默认构建环境：

- Windows 10
- Visual Studio 2013
- x64
- CMake

---

## 目录结构

```text
3rd/                  第三方头文件（libiec61850 1.5.3）
build/                CMake 生成的 VS 工程与编译输出目录
CMakeLists.txt        CMake 构建脚本
docs/                 文档目录
inc/                  对外头文件
lib/                  已编译好的静态库（iec61850.lib / hal.lib）
src/                  wrapper 源码与测试程序源码
```

---

## 依赖说明
构建本项目前，请确认以下文件已经存在：

### 第三方头文件目录
`3rd/libiec61850/` 下应包含 libiec61850 1.5.3 相关头文件。

### 静态库目录
`lib/` 下应至少包含：

- `iec61850.lib`
- `hal.lib`

---

## 首次编译方法

### 1. 打开命令行
建议使用：

- `Developer Command Prompt for VS2013`
- 或已经正确配置 VS2013 编译环境的 PowerShell / CMD

### 2. 生成 Visual Studio 工程
在项目根目录执行：

```powershell
cmake -S . -B build -G "Visual Studio 12 2013 Win64"
```

说明：

- `-S .` 表示源码目录为当前目录
- `-B build` 表示构建目录为 `build`
- `Visual Studio 12 2013 Win64` 表示生成 VS2013 的 64 位工程

### 3. 编译 Release 版本

```powershell
cmake --build build --config Release
```

编译成功后，主要产物位于：

```text
build/Release/iec61850_wrapper.dll
build/Release/iec61850_wrapper.lib
build/Release/iec61850_wrapper_test.exe
```

---

## 如果需要重新生成工程
当你修改了以下内容时，建议重新运行 CMake：

- `CMakeLists.txt`
- 新增或删除 `.cpp/.h` 文件
- include 路径变化
- 静态库路径变化
- target 名称变化

可执行：

```powershell
cmake -S . -B build -G "Visual Studio 12 2013 Win64"
```

如果想彻底重新生成：

```powershell
rmdir /s /q build
cmake -S . -B build -G "Visual Studio 12 2013 Win64"
cmake --build build --config Release
```

---

## 使用生成的 .sln 进行后续开发
CMake 在 `build/` 目录下会生成 Visual Studio 解决方案文件（`.sln`）。

### 打开方式
直接使用 Visual Studio 2013 打开 `build` 目录下的 `.sln` 文件即可。

### 后续开发建议
你可以直接在 Visual Studio 中：

- 修改 `src/` 下源码
- 修改 `inc/` 下头文件
- 选择 `x64 + Release` 或 `x64 + Debug`
- 直接点击“生成”
- 运行 `iec61850_wrapper_test`
- 设置断点调试

### 推荐修改方式
#### 可以直接修改的内容
- `src/*.cpp`
- `inc/*.h`
- `docs/*`

#### 不建议直接手改的内容
- `build/*.vcxproj`
- `build/*.sln`

原因：
这些文件是由 CMake 自动生成的，重新执行 CMake 后可能会被覆盖。

### 如果要改工程配置怎么办
如果你想修改：

- 源文件列表
- include 路径
- 链接库
- 宏定义
- 输出文件名

请优先修改：

```text
CMakeLists.txt
```

修改后重新执行：

```powershell
cmake -S . -B build -G "Visual Studio 12 2013 Win64"
```

---

## 运行测试程序
测试程序路径：

```text
build/Release/iec61850_wrapper_test.exe
```

先查看帮助：

```powershell
.\build\Release\iec61850_wrapper_test.exe --help
```

更详细的测试说明请见：

```text
docs/test_program_usage.md
```

---

## 上层项目如何使用本封装库
通常上层项目只需要以下文件：

- `inc/iec61850_wrapper.h`
- `build/Release/iec61850_wrapper.lib`
- `build/Release/iec61850_wrapper.dll`

### 编译时
- 包含头文件：`inc/`
- 链接：`iec61850_wrapper.lib`

### 运行时
确保可执行程序所在目录，或系统可搜索路径下存在：

- `iec61850_wrapper.dll`

---

## 编码警告说明
当前 VS2013 可能会出现如下警告：

- `warning C4819`

这通常是因为源码文件中包含当前代码页无法完整表示的字符。该警告**不会阻止编译通过**。

如需消除该警告，可将相关源码文件保存为：

- UTF-8 with BOM

---

## 常用命令汇总

### 生成工程
```powershell
cmake -S . -B build -G "Visual Studio 12 2013 Win64"
```

### 编译 Release
```powershell
cmake --build build --config Release
```

### 重新生成并编译
```powershell
rmdir /s /q build
cmake -S . -B build -G "Visual Studio 12 2013 Win64"
cmake --build build --config Release
```

### 查看测试程序帮助
```powershell
.\build\Release\iec61850_wrapper_test.exe --help
```

---

## 文档索引
- 项目构建与开发说明：`README.md`
- 测试程序使用说明：`docs/test_program_usage.md`
