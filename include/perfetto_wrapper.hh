#ifndef PERFETTO_WRAPPER_HH
#define PERFETTO_WRAPPER_HH

#include <memory>
#include <string>
#include <vector>
#include "trace_categories.h"
#include "unified_perf_format.pb.h"

/**
 * PerfettoWrapper 类：封装 perfetto 追踪接口
 * 提供简洁的 API 供上层用户自定义添加 track 和 event
 */
class PerfettoWrapper {
public:
    PerfettoWrapper();
    ~PerfettoWrapper();

    // 禁用拷贝构造和赋值
    PerfettoWrapper(const PerfettoWrapper&) = delete;
    PerfettoWrapper& operator=(const PerfettoWrapper&) = delete;

    /**
     * 初始化 perfetto 追踪系统
     * @param buf_size_kb 缓冲区大小（KB）
     */
    void start(int buf_size_kb = 4096);

    /**
     * 结束追踪并保存到文件
     * @param perf_path 输出文件路径
     */
    void end(const std::string& perf_path);

    /**
     * 创建命名轨道（NamedTrack）
     * @param track_name 轨道内部名称
     * @param track_show_name 轨道显示名称
     * @param parent_track 父轨道引用
     * @param rank_id 排序ID
     * @param set_cnt 是否在名称前添加计数
     * @return 创建的 NamedTrack 智能指针
     */
    std::shared_ptr<perfetto::NamedTrack> createNamedTrack(
        const std::string& track_name,
        const std::string& track_show_name,
        perfetto::Track& parent_track,
        uint64_t rank_id = 0,
        bool set_cnt = false);

    /**
     * 创建计数器轨道（CounterTrack）
     * @param name 计数器名称
     * @param unit_name 单位名称
     * @param parent_track 父轨道引用
     * @return 创建的 CounterTrack 智能指针
     */
    std::shared_ptr<perfetto::CounterTrack> createCounterTrack(
        const std::string& name,
        const std::string& unit_name,
        perfetto::Track& parent_track);

    /**
     * 添加计数器事件
     * @param track 计数器轨道引用
     * @param cycle 时间戳（周期）
     * @param value 计数值
     */
    void addCounterEvent(perfetto::CounterTrack& track, uint64_t cycle, double value);

    /**
     * 添加追踪事件
     * @param title_name 事件标题
     * @param track 命名轨道引用
     * @param start_cycle 开始时间戳（周期）
     * @param end_cycle 结束时间戳（周期）
     * @param msg 事件消息
     */
    void addTraceEvent(
        const std::string& title_name,
        perfetto::NamedTrack& track,
        uint64_t start_cycle,
        uint64_t end_cycle,
        const google::protobuf::Map<std::string, std::string>& common_metadata,
        const google::protobuf::Map<std::string, std::string>& metadata);

    /**
     * 添加带流控制的追踪事件
     * @param title_name 事件标题
     * @param track 命名轨道引用
     * @param start_cycle 开始时间戳（周期）
     * @param end_cycle 结束时间戳（周期）
     * @param flow_id 流ID
     * @param msg 事件消息
     */
    void addTraceEventWithFlow(
        const std::string& title_name,
        perfetto::NamedTrack& track,
        uint64_t start_cycle,
        uint64_t end_cycle,
        uint64_t flow_id,
        const google::protobuf::Map<std::string, std::string>& common_metadata,
        const google::protobuf::Map<std::string, std::string>& metadata);

    /**
     * 获取系统轨道（根轨道）
     * @return 系统轨道引用
     */
    perfetto::Track& getSystemTrack() { return system_track_; }

private:
    uint64_t track_cnt_;                    // 轨道计数器
    uint64_t flow_range_id_;                // 流范围ID
    std::unique_ptr<perfetto::TracingSession> tracing_session_;  // 追踪会话
    perfetto::Track system_track_;          // 系统轨道（根轨道）
};

#endif // PERFETTO_WRAPPER_HH
