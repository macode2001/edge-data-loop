# Edge Data Loop for Intelligent Driving

一个面向智能驾驶车端场景的“数据闭环采集与回传系统”最小可运行项目。

项目模拟车端 Agent 持续采集传感器数据，根据触发策略生成事件包，先落本地缓存，再通过 HTTP 上传到后端。后端接收数据包、保存原始内容，并把元数据写入 SQLite，方便后续扩展为可视化平台、任务下发、数据检索和稳定性分析。

## 你会学到什么

- C++17 多线程、队列、RAII、文件 IO
- 车端数据缓存、失败重试、断点式回传思路
- 触发策略：急刹、障碍物、周期采样
- 数据脱敏：GPS 坐标网格化
- Linux/车端 Agent 的模块拆分方式
- HTTP 上报协议、后端接收、SQLite 元数据存储

## 项目结构

```text
.
├── CMakeLists.txt
├── README.md
├── backend
│   └── server.py
├── config
│   └── agent.conf
└── src
    ├── cache_store.cpp
    ├── cache_store.h
    ├── data_packet.h
    ├── http_client.cpp
    ├── http_client.h
    ├── main.cpp
    ├── sensor_simulator.cpp
    ├── sensor_simulator.h
    ├── uploader.cpp
    └── uploader.h
```

## 运行后端

```powershell
python backend/server.py
```

默认监听：

```text
http://127.0.0.1:8080
```

上传的数据会保存到：

```text
runtime/backend/storage/
runtime/backend/metadata.db
```

## 构建车端 Agent

需要先安装 CMake 和 C++ 编译器。

Windows 推荐：

- Visual Studio 2022 Build Tools
- 勾选 Desktop development with C++
- 安装 CMake

Ubuntu 推荐：

```bash
sudo apt update
sudo apt install -y build-essential cmake
```

然后执行：

```powershell
cmake -S . -B build
cmake --build build
```

## 运行车端 Agent

```powershell
.\build\Debug\edge_agent.exe
```

如果你的生成器输出的是 Release 或单配置目录，也可能是：

```powershell
.\build\edge_agent.exe
```

Linux/macOS 上通常是：

```bash
./build/edge_agent
```

## 当前功能

- 模拟车辆传感器帧：速度、加速度、障碍物距离、GPS
- 触发事件包：
  - hard_brake：急刹
  - obstacle_close：障碍物过近
  - periodic_sample：周期采样
- 本地缓存到 `data/cache`
- 上传成功后移动到 `data/sent`
- 上传失败后保留缓存，下一轮继续重试
- GPS 脱敏为近似网格坐标
- 后端接收 JSON 数据包并写入 SQLite

## 后续升级方向

1. 增加 protobuf 序列化，替代手写 JSON。
2. 增加上传限速、批量上传、压缩。
3. 增加任务下发接口，让后端控制采集策略。
4. 增加 `/proc` 系统监控，采集 CPU、内存、磁盘、进程状态。
5. 接入 ROS2 topic，把模拟传感器替换为真实中间件输入。
6. 加入 perf、valgrind、gdb 的问题复现样例。

## 简历描述参考

设计并实现智能驾驶车端数据闭环采集与回传系统，支持传感器数据模拟采集、触发式事件缓存、GPS 数据脱敏、失败重试、HTTP 回传、后端落盘与 SQLite 元数据管理。项目使用 C++17、Linux socket、Python HTTP Server、SQLite、CMake 实现，重点覆盖车端数据闭环、稳定性治理和边缘数据回传链路。
