#ifndef PERF_SHOWER_HH
#define PERF_SHOWER_HH

#include "perfetto_wrapper.hh"
#include "unified_perf_format.pb.h"
#include <string>
#include <vector>
#include <map>

/**
 * 过滤器规则：支持字符串匹配
 */
struct FilterRule {
  std::string value;
  FilterRule(const std::string &v) : value(v) {}
};

/**
 * 视图配置：对应 show.json 中的一个 view
 */
struct ViewConfig {
  std::string mode;  // "pipe", "line", "func", "cnt"
  std::vector<FilterRule> timeline_filter;  // 时间范围过滤，格式: "start-end"
  std::vector<FilterRule> event_filter;     // 事件名称过滤
  std::vector<FilterRule> track_filter;     // 轨道名称过滤
  std::vector<FilterRule> device_filter;    // 设备名称过滤
  std::vector<FilterRule> thread_filter;    // 线程ID过滤
};

/**
 * JSON 配置解析结果：包含视图配置和文件列表
 */
struct JsonConfig {
  std::map<std::string, ViewConfig> views;  // 视图配置映射
  std::vector<std::string> filelist;        // 输入文件列表
  std::string output;                       // 输出文件路径
};

/**
 * PerfShower 类：将统一性能格式数据转换为 Perfetto 追踪格式
 */
class PerfShower {
public:
  PerfShower();
  ~PerfShower();

  /**
   * 初始化 Perfetto 追踪系统
   * @param buf_size_kb 缓冲区大小（KB），默认 4096KB
   */
  void init(int buf_size_kb = 409600);

  /**
   * 结束追踪并保存到文件
   * @param output_path 输出文件路径
   */
  void finish(const std::string &output_path);
  
  /**
   * 根据 show.json 配置和多个 bin 文件显示性能数据
   * @param show_json_path show.json 配置文件路径
   * @param bin_file_paths 包含性能数据的 bin 文件路径列表（已废弃，从 JSON 中读取）
   * @return 输出文件路径（从 JSON 配置中读取）
   */
  std::string show(const std::string &show_json_path);
  
private:
  /**
   * 处理单个 Instruction 并添加到追踪
   * @param inst Instruction 对象引用
   * @param parent_track 父轨道
   */
  void processInstruction(const unified_perf_format::Instruction &inst,
                          perfetto::Track &parent_track);
  
  /**
   * 处理 Pipe 模式
   */
  void processPipMode(const unified_perf_format::BatchInstruction &batch_instruction,
                      perfetto::Track &parent_track);

  /**
   * 处理 Line 模式（线性模式）
   */
  void processLineMode(const unified_perf_format::BatchInstruction &batch_instruction,
                       perfetto::Track &parent_track);

  /**
   * 处理 Func 模式
   */
  void processFuncMode(const unified_perf_format::BatchFunction &batch_function,
                       perfetto::Track &parent_track);

  /**
   * 处理 Cnt 模式
   */
  void processCntMode(const unified_perf_format::BatchCounter &batch_counter,
                      perfetto::Track &parent_track);

  /**
   * 解析 show.json 文件
   * @param json_path JSON 文件路径
   * @return JSON 配置解析结果（包含视图配置和文件列表）
   */
  JsonConfig parseShowJson(const std::string &json_path);

  /**
   * 检查是否通过时间线过滤器
   */
  bool passTimelineFilter(const std::vector<FilterRule> &filters, uint64_t start_time, uint64_t end_time);

  /**
   * 检查是否通过事件过滤器
   */
  bool passEventFilter(const std::vector<FilterRule> &filters, const std::string &event_name);

  /**
   * 检查是否通过轨道过滤器
   */
  bool passTrackFilter(const std::vector<FilterRule> &filters, const std::string &track_name);

  /**
   * 检查是否通过设备过滤器
   */
  bool passDeviceFilter(const std::vector<FilterRule> &filters, const std::string &device_name);

  /**
   * 检查是否通过线程过滤器
   */
  bool passThreadFilter(const std::vector<FilterRule> &filters, uint32_t thread_id);

  /**
   * 根据视图配置处理数据
   * @param view_config 视图配置
   * @param perf_data_list 已经读取的性能数据列表
   * @param view_track 该视图对应的 track（父轨道）
   */
  void processDataWithView(const ViewConfig &view_config, 
                           const std::vector<unified_perf_format::UnifiedPerfData> &perf_data_list,
                           perfetto::Track &view_track);
  
  /**
   * 从文件读取性能数据（支持单个或多个消息格式）
   * @param bin_file_path 数据文件路径
   * @return 读取到的性能数据列表
   */
  std::vector<unified_perf_format::UnifiedPerfData> readPerfDataFromFile(const std::string &bin_file_path);
  
  /**
   * 从多个文件读取性能数据并合并
   * @param bin_file_paths 数据文件路径列表
   * @return 合并后的性能数据列表
   */
  std::vector<unified_perf_format::UnifiedPerfData> readPerfDataFromFiles(const std::vector<std::string> &bin_file_paths);

  PerfettoWrapper perfetto_wrapper_;
  bool initialized_;
};

#endif // PERF_SHOWER_HH
