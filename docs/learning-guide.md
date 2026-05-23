# 项目学习指南

这份文档专门对应当前项目，不是泛泛讲 C++。你可以按这里的顺序读代码，每读完一节就运行一次项目，看终端输出和文件变化。

## 1. 这个项目在模拟什么

项目模拟的是智能驾驶车端数据闭环：

```text
传感器数据
  -> 车端 Agent 采集
  -> 触发策略判断
  -> 构建事件数据包
  -> 本地缓存
  -> HTTP 上传
  -> 后端接收
  -> 保存原始数据和元数据
```

真实车企里，这条链路通常用于把车端遇到的特殊场景回传到云端，例如急刹、障碍物过近、模型识别异常、人工接管等。云端拿到数据后，可以用于问题定位、模型训练、回归测试和算法迭代。

## 2. 先读 main.cpp

入口文件：

```text
src/main.cpp
```

重点读 `run_agent()`。

它做了 5 件事：

1. 读取配置：`load_config()`
2. 初始化模块：`CacheStore`、`SensorSimulator`、`Uploader`
3. 循环生成传感器帧：`simulator.next_frame()`
4. 判断是否触发事件：`detect_trigger()`
5. 触发后保存缓存，并周期性上传：`store.save_packet()`、`uploader.upload_once()`

你可以把 `run_agent()` 理解成车端 Agent 的主循环。

## 3. 配置文件怎么生效

配置文件：

```text
config/agent.conf
```

关键字段：

```text
server_host=127.0.0.1
server_port=8080
sample_interval_ms=200
periodic_trigger_every=15
max_packets=60
```

含义：

- `server_host` / `server_port`：后端地址
- `sample_interval_ms`：每隔多少毫秒采集一帧
- `periodic_trigger_every`：每多少帧强制触发一次周期采样
- `max_packets`：程序最多生成多少帧后退出

学习实验：

把 `max_packets=60` 改成 `max_packets=10`，重新运行 Agent，观察程序是不是更快结束。

## 4. 传感器数据从哪里来

相关文件：

```text
src/sensor_simulator.h
src/sensor_simulator.cpp
```

核心函数：

```cpp
SensorFrame SensorSimulator::next_frame()
```

它会模拟：

- 车速：`speed_mps`
- 加速度：`acceleration_mps2`
- 障碍物距离：`obstacle_distance_m`
- GPS 坐标：`latitude`、`longitude`
- 时间戳：`timestamp`

它还会随机制造两个事件：

- 急刹：让加速度突然变成很大的负数
- 障碍物过近：让障碍物距离变得很小

## 5. 什么情况下触发上传

相关函数：

```cpp
detect_trigger()
```

当前规则：

```text
加速度 < -5.0      -> hard_brake
障碍物距离 < 6.0   -> obstacle_close
帧号能被 N 整除    -> periodic_sample
```

注意：触发后不是立即直接上传，而是先写入本地缓存。这样做是为了防止网络异常时数据丢失。

## 6. 为什么有 ring_buffer

相关代码：

```cpp
std::deque<SensorFrame> ring_buffer;
constexpr size_t kRingBufferSize = 12;
```

`ring_buffer` 保存最近 12 帧。

原因是：当急刹发生时，只上传急刹那一帧意义不大。后端更想知道急刹前后几秒发生了什么，所以项目会把最近的一段帧一起打包。

这就是事件上下文。

## 7. 数据包是什么

相关文件：

```text
src/data_packet.h
```

两个核心结构：

```cpp
struct SensorFrame
struct DataPacket
```

`SensorFrame` 是一帧数据。

`DataPacket` 是一次事件包，里面包含：

- `packet_id`：数据包唯一 ID
- `vehicle_id`：车辆 ID
- `trigger_reason`：触发原因
- `frames`：事件上下文数据
- `created_at`：数据包创建时间

## 8. 本地缓存怎么工作

相关文件：

```text
src/cache_store.cpp
```

核心函数：

```cpp
save_packet()
list_cached_packets()
mark_sent()
```

流程：

```text
触发事件
  -> 写 data/cache/xxx.tmp
  -> rename 成 data/cache/xxx.json
  -> 上传成功后移动到 data/sent/xxx.json
```

为什么先写 `.tmp`？

如果程序写文件写到一半崩溃，`.tmp` 不会被上传模块扫描到。只有完整写完并改名成 `.json` 后，才会进入上传流程。

## 9. 上传怎么工作

相关文件：

```text
src/uploader.cpp
src/http_client.cpp
```

`Uploader` 负责业务逻辑：

```text
扫描 data/cache
  -> 读取 JSON
  -> 调用 HttpClient 上传
  -> 成功则移动到 data/sent
  -> 失败则保留在 data/cache
```

`HttpClient` 负责底层网络：

```text
建立 socket
  -> 拼 HTTP POST 请求
  -> 发送 JSON
  -> 读取 HTTP 响应
```

学习时你可以先理解 `Uploader`，暂时不用深挖 socket 细节。

## 10. 后端怎么接收

相关文件：

```text
backend/server.py
```

核心函数：

```python
do_POST()
list_packets()
init_db()
```

后端收到 JSON 后会做两件事：

1. 把原始 JSON 保存到：

```text
runtime/backend/storage/
```

2. 把元数据写入 SQLite：

```text
runtime/backend/metadata.db
```

浏览器访问：

```text
http://127.0.0.1:8080/api/v1/packets
```

看到 `packets` 列表，就说明数据闭环跑通。

## 11. 你应该掌握的 5 个问题

读完第一轮后，试着自己回答：

1. 为什么车端数据要先写本地缓存？
2. 为什么上传成功后要从 `data/cache` 移动到 `data/sent`？
3. `trigger_reason` 有哪几种？
4. `ring_buffer` 为什么保存的是最近 12 帧？
5. 后端为什么既保存原始 JSON，又保存 SQLite 元数据？

能回答这 5 个问题，你就已经理解了这个项目的第一层。

## 12. 推荐学习顺序

第一轮只看主链路：

```text
main.cpp -> sensor_simulator.cpp -> data_packet.h -> cache_store.cpp -> uploader.cpp -> server.py
```

第二轮再看细节：

```text
http_client.cpp -> C++ socket -> HTTP 请求格式 -> Windows 文件句柄问题
```

第三轮开始改功能：

```text
把急刹阈值放到配置文件
增加上传批次大小
增加后端任务下发接口
增加系统 CPU/内存监控
```
