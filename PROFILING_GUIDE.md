# Android Native 性能分析指南

使用 simpleperf 录制 → 生成 binary_cache → 转换 pprof proto → 用 `go tool pprof` 分析。

---

## 1. 录制：simpleperf

### 基本命令

```bash
simpleperf record \
  -e cpu-cycles \          # 采样事件
  --duration 10 \          # 录制秒数
  --call-graph dwarf \     # 调用栈展开方式
  -o /data/local/tmp/perf.data \
  /data/local/tmp/target_exe
```

### 事件 `-e`

| 事件 | 含义 | 适用场景 |
|------|------|----------|
| `cpu-cycles` | CPU 硬件周期计数器，on-CPU 采样 | 默认选择，找 CPU 热点 |
| `cpu-clock` | 软件时钟，每毫秒一次 | 配合 `--trace-offcpu` 使用 |
| `task-clock` | 类似 cpu-clock，但只计此线程时间 | 单线程分析 |

> `cpu-cycles` 只在线程**正在执行**时采样，线程睡眠/阻塞时没有样本。

### 调用栈展开 `--call-graph`

| 方式 | 原理 | 优缺点 |
|------|------|--------|
| `dwarf` | 读取 DWARF 调试信息，软件解析栈帧 | 准确，开销略高，需要带调试符号的二进制 |
| `fp` | 依赖 frame pointer 寄存器链 | 开销低，但需要编译时加 `-fno-omit-frame-pointer`，且部分库不支持 |

Android NDK 推荐用 `dwarf`，本项目编译时已加 `-fno-omit-frame-pointer` 作为备用。

### off-CPU 录制 `--trace-offcpu`

```bash
simpleperf record \
  -e cpu-clock \           # 必须用 cpu-clock，不能用 cpu-cycles
  --trace-offcpu \         # 线程阻塞时也记录样本
  --duration 10 \
  --call-graph dwarf \
  -o /data/local/tmp/perf.data \
  /data/local/tmp/target_exe
```

`--trace-offcpu` 在线程**离开 CPU**（sleep、mutex 等待、I/O 阻塞）时插入样本，记录阻塞时长。结合 on-CPU 录制，能区分程序慢是因为**算得慢**还是**等得多**。

| 录制类型 | 能看到 | 看不到 |
|----------|--------|--------|
| on-CPU（cpu-cycles） | CPU 计算热点 | 线程睡眠/等锁时间 |
| off-CPU（--trace-offcpu） | 阻塞调用栈、等锁热点 | 纯计算时间 |

### 其他常用选项

| 选项 | 说明 |
|------|------|
| `--duration N` | 录制 N 秒后停止 |
| `-f 4000` | 采样频率，默认 4000 Hz |
| `--no-inherit` | 不录制子进程/子线程 |
| `-p <pid>` | attach 到已运行的进程 |
| `-t <tid>` | 只录制指定线程 |

---

## 2. 后处理：binary_cache 与 pprof proto

### 为什么需要 binary_cache

设备上的二进制没有完整调试符号（strip 过）。`binary_cache_builder.py` 把**带符号的本地二进制**按 build-id 组织成目录树，供 proto 生成器做符号解析。

```bash
# 在 host 上运行，需要连接设备
python3 $NDK/simpleperf/binary_cache_builder.py \
  -i perf.data \
  --ndk_path $NDK
```

### 生成 pprof proto

```bash
python3 $NDK/simpleperf/pprof_proto_generator.py \
  -i perf.data \
  -o perf.proto \
  --binary_cache_dir binary_cache
```

proto 文件可直接用 `go tool pprof` 打开，也可上传到 pprof web UI。

---

## 3. 分析：go tool pprof

```bash
go tool pprof -http=:8080 perf.proto
```

浏览器打开 `http://localhost:8080`，有四个主要视图。

---

### 3.1 Top 视图

```
      flat  flat%   sum%        cum   cum%  函数名
   3.21s   64.2%  64.2%      3.21s  64.2%  traverse
   1.05s   21.0%  85.2%      4.26s  85.2%  main
```

| 列 | 含义 |
|----|------|
| `flat` | 采样点**直接落在**该函数的时间（函数本身的开销） |
| `flat%` | flat 占总时间的比例 |
| `cum` | 该函数及其**所有子调用**的累计时间 |
| `cum%` | cum 占总时间的比例 |
| `sum%` | 前几行 flat% 的累加，快速定位前 N% 热点 |

**快速定位规则：**

- `flat%` 高 → 这个函数**本身**是瓶颈，优化它内部的逻辑
- `flat%` 低但 `cum%` 高 → 这个函数是入口，瓶颈在它的某个子调用里，往下追
- `flat% ≈ cum%` → 这个函数几乎不调用其他函数，是叶子节点热点

---

### 3.2 Flame Graph（火焰图）

```
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ main (100%)
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ traverse (99%)
▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  std::list::iterator::operator++ (87%)
```

- **X 轴宽度** = 该函数占用的 CPU 时间比例，越宽越热
- **Y 轴** = 调用深度，下层调用上层（ pprof 默认是上层调用下层）
- **点击某个框** = 以该函数为根重新展示，方便聚焦子树
- **搜索框** = 高亮包含关键词的所有帧，快速找某个模块

**阅读步骤：**

1. 找最宽的框 → 最热的函数
2. 看它下面（被它调用的）→ 热点是在哪个子调用
3. 找"平顶"（宽框上面没有更宽的子框）→ 这里是真正消耗时间的地方

---

### 3.3 Graph 视图

节点 = 函数，边 = 调用关系，节点越大/越红 = 越热。

- 节点内显示 `flat / cum`
- 边上的数字 = 通过这条调用路径的时间
- 虚线边 = 被 pprof 因阈值裁剪过的边

适合快速看**调用关系全局结构**，火焰图适合看**时间分布**。

---

### 3.4 Refine 过滤（Web UI 右上角）

| 操作 | 效果 |
|------|------|
| Focus | 只显示调用栈中**包含**该函数的路径 |
| Ignore | 排除包含该函数的路径 |
| Hide | 从图中隐藏该节点（不影响时间统计） |

例：想只看 `mutex` 相关的调用栈 → Focus 输入 `mutex`。

---

## 4. 常见热点符号识别

| 符号 | 说明 | 对应问题 |
|------|------|----------|
| `__schedule` | Linux 调度器，线程让出 CPU | **off-CPU 热点**，说明线程大量阻塞（等锁、I/O、sleep） |
| `__aarch64_ldadd8_relax` | ARM64 原子 fetch_add | 原子操作竞争 / **false sharing**（多线程写同一 cache line） |
| `pthread_mutex_lock` / `futex` | 互斥锁等待 | **锁竞争**，临界区太大或持锁太久 |
| `scudo::*` / `malloc` / `free` | Scudo 分配器（Android 默认） | **内存分配压力**，频繁 alloc/free |
| `@plt` 后缀 | PLT 跳转（动态链接桩） | 动态库调用开销，通常可忽略 |
| `std::_List_node` 相关 | `std::list` 节点操作 | **指针追踪**，cache miss 严重 |
| `std::map` / `_Rb_tree` | 红黑树节点操作 | O(log N) 树遍历，考虑换 `unordered_map` |
| `[kernel.kallsyms]` | 内核符号 | 系统调用、中断、调度开销 |

---

## 5. 快速排查流程

```
打开 Top 视图
    │
    ├─ flat% 高的函数 → 直接优化该函数
    │
    └─ cum% 高但 flat% 低 → 进火焰图
           │
           └─ 找最宽的"平顶"块
                  │
                  ├─ 是系统/库符号（__schedule, mutex, scudo）
                  │       → 对照上表判断根因
                  │
                  └─ 是业务代码
                          → 看源码，优化算法/数据结构
```

**on-CPU 还是 off-CPU？**

- 程序 CPU 占用高但吞吐低 → 先看 on-CPU（cpu-cycles）
- 程序 CPU 占用低但响应慢 → 先看 off-CPU（--trace-offcpu），找 `__schedule` / `futex`
- 多线程程序不确定 → 两个都录，对比火焰图宽度
