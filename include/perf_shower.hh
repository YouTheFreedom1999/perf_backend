#ifndef PERF_SHOWER_HH
#define PERF_SHOWER_HH

#include "perfetto_wrapper.hh"
#include "unified_perf_format.pb.h"
#include <string>

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
  void init(int buf_size_kb = 4096);

  /**
   * 结束追踪并保存到文件
   * @param output_path 输出文件路径
   */
  void finish(const std::string &output_path);

  /**
   * 从文件加载并追踪 Instructions 数据
   * @param file_name 输入文件路径
   */
  void traceInstructions(const std::string &file_name);

  /**
   * 从文件加载并追踪 Functions 数据
   * @param file_name 输入文件路径
   */
  void traceFunctions(const std::string &file_name);

  /**
   * 从文件加载并追踪 Counters 数据
   * @param file_name 输入文件路径
   */
  void traceCounters(const std::string &file_name);
  
private:
  /**
   * 处理单个 Instruction 并添加到追踪
   * @param inst Instruction 对象引用
   * @param parent_track 父轨道
   */
  void processInstruction(const unified_perf_format::Instruction &inst,
                          perfetto::Track &parent_track);
  void processPipMode(const unified_perf_format::BatchInstruction &batch_instruction,
                      perfetto::Track &parent_track);

  PerfettoWrapper perfetto_wrapper_;
  bool initialized_;
};

#endif // PERF_SHOWER_HH
