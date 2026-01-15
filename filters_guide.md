# Filters 写法指南

本文档详细说明 `perf_shower.cc` 中 filters（过滤器）的使用方法和配置格式。

## 目录

- [概述](#概述)
- [过滤器类型](#过滤器类型)
- [过滤逻辑](#过滤逻辑)
- [JSON 配置格式](#json-配置格式)
- [使用示例](#使用示例)
- [注意事项](#注意事项)

## 概述

Filters 用于在性能数据可视化时筛选和过滤数据，支持按时间范围、事件名称、轨道名称、设备名称和线程ID进行过滤。所有过滤器都配置在 `show.json` 配置文件中，每个视图（view）可以独立配置自己的过滤器。

### 基本规则

1. **可选字段**：所有过滤器字段都是可选的，如果 JSON 中未指定某个过滤器，则不会进行过滤（所有数据都通过）
2. **OR 逻辑**：每个过滤器类型内部使用 OR 逻辑，只要匹配到任何一个规则就通过
3. **AND 逻辑**：不同过滤器类型之间使用 AND 逻辑，必须同时满足所有指定的过滤器才能通过

## 过滤器类型

### 1. timeline_filter（时间线过滤器）

用于按时间范围过滤事件。

**格式**：
- 时间范围：`"start-end"`（使用连字符分隔）
- 单个时间戳：`"timestamp"`（纯数字字符串）

**匹配规则**：
- 时间范围格式：检查事件的时间范围是否与过滤器的时间范围有重叠
  - 重叠条件：`event.start_time <= filter_end && event.end_time >= filter_start`
  - 只要有任何重叠，整个事件都会被显示
- 单个时间戳：检查该时间戳是否在事件的时间范围内
  - 条件：`timestamp >= event.start_time && timestamp <= event.end_time`

**示例**：
```json
"timeline_filter": [
  "1000-2000",      // 显示时间范围 1000-2000 内的事件
  "5000-6000",      // 显示时间范围 5000-6000 内的事件
  "1500"            // 显示包含时间戳 1500 的事件
]
```

### 2. event_filter（事件过滤器）

用于按事件名称过滤。

**格式**：字符串数组，每个字符串是一个子串匹配模式

**匹配规则**：
- 使用子串匹配（`std::string::find()`）
- 如果事件名称包含过滤器中的任何一个子串，则通过
- 匹配的是事件的 `show_title`（如果为空则使用 `name`）

**示例**：
```json
"event_filter": [
  "Load",           // 匹配所有包含 "Load" 的事件
  "Store",          // 匹配所有包含 "Store" 的事件
  "Execute"         // 匹配所有包含 "Execute" 的事件
]
```

### 3. track_filter（轨道过滤器）

用于按轨道名称过滤。

**格式**：字符串数组，每个字符串是一个子串匹配模式

**匹配规则**：
- 使用子串匹配（`std::string::find()`）
- 如果轨道名称包含过滤器中的任何一个子串，则通过
- 主要用于 `cnt` 模式，过滤计数器名称

**示例**：
```json
"track_filter": [
  "cache",          // 匹配所有包含 "cache" 的轨道
  "memory",         // 匹配所有包含 "memory" 的轨道
  "bandwidth"       // 匹配所有包含 "bandwidth" 的轨道
]
```

### 4. device_filter（设备过滤器）

用于按设备名称过滤。

**格式**：字符串数组，每个字符串是一个子串匹配模式

**匹配规则**：
- 使用子串匹配（`std::string::find()`）
- 如果设备名称包含过滤器中的任何一个子串，则通过
- 在数据块级别进行过滤，如果设备不匹配，整个数据块都会被跳过

**示例**：
```json
"device_filter": [
  "cpu",            // 只显示 CPU 设备的数据
  "vpu",            // 只显示 VPU 设备的数据
  "tpu"             // 只显示 TPU 设备的数据
]
```

### 5. thread_filter（线程过滤器）

用于按线程ID过滤。

**格式**：字符串数组，每个字符串是一个线程ID（数字字符串）

**匹配规则**：
- 精确匹配线程ID
- 如果线程ID等于过滤器中的任何一个值，则通过
- 使用 `std::stoul()` 解析，必须是有效的无符号整数

**示例**：
```json
"thread_filter": [
  "0",              // 只显示线程 0 的数据
  "1",              // 只显示线程 1 的数据
  "2"               // 只显示线程 2 的数据
]
```

## 过滤逻辑

### 过滤器组合逻辑

不同过滤器类型之间使用 **AND** 逻辑：

```
通过条件 = device_filter 通过 AND
           thread_filter 通过 AND
           timeline_filter 通过 AND
           event_filter 通过 AND
           track_filter 通过（如果适用）
```

### 单个过滤器内部逻辑

每个过滤器类型内部使用 **OR** 逻辑：

```
timeline_filter 通过 = rule1 匹配 OR rule2 匹配 OR ... OR ruleN 匹配
event_filter 通过 = rule1 匹配 OR rule2 匹配 OR ... OR ruleN 匹配
...
```

### 空过滤器

如果某个过滤器字段在 JSON 中未指定或为空数组，则：
- 该过滤器不进行过滤
- 所有数据都通过该过滤器

## JSON 配置格式

### 基本结构

```json
{
  "filelist": ["file1.bin", "file2.bin"],
  "output": "output.perfetto",
  "view_name": {
    "mode": "pipe|line|func|cnt",
    "timeline_filter": ["规则1", "规则2", ...],
    "event_filter": ["规则1", "规则2", ...],
    "track_filter": ["规则1", "规则2", ...],
    "device_filter": ["规则1", "规则2", ...],
    "thread_filter": ["规则1", "规则2", ...]
  }
}
```

### 字段说明

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `filelist` | 字符串数组 | 是 | 输入文件列表 |
| `output` | 字符串 | 是 | 输出文件路径 |
| `view_name` | 对象 | 是 | 视图配置（可以有多个视图） |
| `mode` | 字符串 | 是 | 视图模式：`pipe`、`line`、`func`、`cnt` |
| `timeline_filter` | 字符串数组 | 否 | 时间线过滤器 |
| `event_filter` | 字符串数组 | 否 | 事件过滤器 |
| `track_filter` | 字符串数组 | 否 | 轨道过滤器 |
| `device_filter` | 字符串数组 | 否 | 设备过滤器 |
| `thread_filter` | 字符串数组 | 否 | 线程过滤器 |

## 使用示例

### 示例 1：基本过滤

只显示 CPU 设备在时间范围 1000-5000 内的 Load 和 Store 事件：

```json
{
  "filelist": ["perf_data.bin"],
  "output": "filtered.perfetto",
  "cpu_view": {
    "mode": "pipe",
    "device_filter": ["cpu"],
    "timeline_filter": ["1000-5000"],
    "event_filter": ["Load", "Store"]
  }
}
```

### 示例 2：多线程过滤

显示线程 0 和线程 1 的所有事件：

```json
{
  "filelist": ["perf_data.bin"],
  "output": "threads.perfetto",
  "thread_view": {
    "mode": "line",
    "thread_filter": ["0", "1"]
  }
}
```

### 示例 3：多时间范围过滤

显示多个时间范围内的事件：

```json
{
  "filelist": ["perf_data.bin"],
  "output": "multi_time.perfetto",
  "time_view": {
    "mode": "pipe",
    "timeline_filter": [
      "1000-2000",
      "5000-6000",
      "10000-11000"
    ]
  }
}
```

### 示例 4：计数器过滤（cnt 模式）

只显示包含 "cache" 的计数器：

```json
{
  "filelist": ["perf_counters.bin"],
  "output": "counters.perfetto",
  "counter_view": {
    "mode": "cnt",
    "track_filter": ["cache"]
  }
}
```

### 示例 5：函数模式过滤

显示特定函数的调用：

```json
{
  "filelist": ["perf_functions.bin"],
  "output": "functions.perfetto",
  "func_view": {
    "mode": "func",
    "event_filter": ["processData", "compute"],
    "thread_filter": ["0"]
  }
}
```

### 示例 6：组合过滤

复杂的组合过滤示例：

```json
{
  "filelist": ["perf_data.bin"],
  "output": "complex.perfetto",
  "complex_view": {
    "mode": "pipe",
    "device_filter": ["vpu", "tpu"],
    "thread_filter": ["0", "1"],
    "timeline_filter": ["1000-5000"],
    "event_filter": ["Execute", "Load"]
  }
}
```

这个配置会显示：
- 设备名称包含 "vpu" 或 "tpu" 的数据
- 线程ID为 0 或 1 的数据
- 时间范围在 1000-5000 内的数据
- 事件名称包含 "Execute" 或 "Load" 的数据

**所有条件必须同时满足**（AND 逻辑）。

### 示例 7：无过滤器（显示所有数据）

不指定任何过滤器，显示所有数据：

```json
{
  "filelist": ["perf_data.bin"],
  "output": "all_data.perfetto",
  "all_view": {
    "mode": "pipe"
  }
}
```

## 注意事项

### 1. 时间戳格式

- 时间戳必须是有效的无符号整数（uint64_t）
- 时间范围格式：`"start-end"`，start 和 end 都必须是数字
- 不支持负数时间戳

### 2. 字符串匹配

- `event_filter`、`track_filter`、`device_filter` 使用子串匹配
- 匹配是大小写敏感的
- 如果事件名称为空，会使用默认名称

### 3. 线程ID格式

- 线程ID必须是有效的无符号整数（uint32_t）
- 使用字符串格式（JSON 中必须是字符串，如 `"0"` 而不是 `0`）
- 不支持负数线程ID

### 4. 性能考虑

- 过滤器在数据处理时应用，不会修改原始数据
- 使用 `CopyFrom()` 复制数据，确保原始数据保持不变
- 如果过滤器为空，不会进行任何过滤操作（性能最优）

### 5. 模式相关

不同模式支持的过滤器：

| 模式 | timeline_filter | event_filter | track_filter | device_filter | thread_filter |
|------|----------------|--------------|--------------|---------------|---------------|
| `pipe` | ✅ | ✅ | ❌ | ✅ | ✅ |
| `line` | ✅ | ✅ | ❌ | ✅ | ✅ |
| `func` | ✅ | ✅ | ❌ | ✅ | ✅ |
| `cnt` | ✅ | ❌ | ✅ | ✅ | ❌ |

### 6. 错误处理

- 如果时间戳格式错误，`std::stoull()` 会抛出异常
- 如果线程ID格式错误，`std::stoul()` 会抛出异常
- 建议在配置文件中使用正确的格式，避免运行时错误

### 7. 调试建议

- 如果过滤结果不符合预期，可以：
  1. 先不设置过滤器，查看所有数据
  2. 逐步添加过滤器，观察过滤效果
  3. 检查事件名称、设备名称等实际值
  4. 使用子串匹配时，确保子串拼写正确

## 实现细节

### 代码位置

过滤器的实现在以下文件中：
- `components/perf_backend/src/perf_shower.cc`：过滤器逻辑实现
- `components/perf_backend/include/perf_shower.hh`：过滤器结构定义

### 关键函数

- `passTimelineFilter()`：时间线过滤器判断（第 339-366 行）
- `passEventFilter()`：事件过滤器判断（第 368-379 行）
- `passTrackFilter()`：轨道过滤器判断（第 381-392 行）
- `passDeviceFilter()`：设备过滤器判断（第 394-405 行）
- `passThreadFilter()`：线程过滤器判断（第 407-419 行）
- `parseShowJson()`：JSON 配置解析（第 261-337 行）
- `processDataWithView()`：应用过滤器处理数据（第 478-663 行）

## 总结

Filters 提供了灵活的数据过滤机制，可以按时间、事件、设备、线程等多个维度进行过滤。合理使用过滤器可以：

1. **提高可视化效率**：只显示关注的数据
2. **减少文件大小**：过滤后的数据文件更小
3. **便于分析**：聚焦特定时间段或特定类型的事件
4. **多视图对比**：为不同视图配置不同过滤器，进行对比分析

建议根据实际需求组合使用不同的过滤器，以达到最佳的可视化效果。
