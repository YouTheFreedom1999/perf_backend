#include "perf_shower.hh"
#include <cassert>
#include <fstream>
#include <iostream>

using namespace unified_perf_format;

PerfShower::PerfShower() : initialized_(false) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
}

PerfShower::~PerfShower() {}

void PerfShower::init(int buf_size_kb) {
  if (!initialized_) {
    perfetto_wrapper_.start(buf_size_kb);
    initialized_ = true;
  }
}

void PerfShower::finish(const std::string &output_path) {
  if (initialized_) {
    perfetto_wrapper_.end(output_path);
    initialized_ = false;
    google::protobuf::ShutdownProtobufLibrary();
  }
}

void PerfShower::traceInstructions(const std::string &file_name) {
  if (!initialized_) {
    std::cerr << "错误：请先调用 init() 初始化" << std::endl;
    return;
  }

  AllInstructions all_instructions;
  std::ifstream input(file_name, std::ios::in | std::ios::binary);

  if (!input.is_open()) {
    std::cerr << "错误：无法打开文件 " << file_name << std::endl;
    return;
  }

  if (!all_instructions.ParseFromIstream(&input)) {
    std::cerr << "错误：解析文件失败 " << file_name << std::endl;
    return;
  }

  auto &system_track = perfetto_wrapper_.getSystemTrack();

  // 遍历所有设备的 batch_instructions
  for (auto & it : all_instructions.batch_instructions()) {
    auto & device_name = it.first;
    auto & batch_instruction = it.second;
    // 为每个设备创建一个轨道
    auto device_track = perfetto_wrapper_.createNamedTrack(
        "device_" + device_name, "Device: " + device_name, system_track, 0,
        false);

    // 处理该设备的所有 instructions
    for (const auto &inst : batch_instruction.instructions()) {
      processInstruction(inst, *device_track);
    }
  }
}

void PerfShower::traceFunctions(const std::string &file_name) {
  if (!initialized_) {
    std::cerr << "错误：请先调用 init() 初始化" << std::endl;
    return;
  }

  AllFunctions all_functions;
  std::ifstream input(file_name, std::ios::in | std::ios::binary);

  if (!input.is_open()) {
    std::cerr << "错误：无法打开文件 " << file_name << std::endl;
    return;
  }

  if (!all_functions.ParseFromIstream(&input)) {
    std::cerr << "错误：解析文件失败 " << file_name << std::endl;
    return;
  }

  auto &system_track = perfetto_wrapper_.getSystemTrack();

  // 遍历所有设备的 batch_functions
  for (const auto &[device_name, batch_function] :
       all_functions.batch_functions()) {
    // 为每个设备创建一个轨道
    auto device_track = perfetto_wrapper_.createNamedTrack(
        "device_" + device_name, "Device: " + device_name, system_track, 0,
        false);

    // 处理该设备的所有 functions
    for (const auto &func : batch_function.functions()) {
      processFunction(func, *device_track);
    }
  }
}

void PerfShower::traceCounters(const std::string &file_name) {
  if (!initialized_) {
    std::cerr << "错误：请先调用 init() 初始化" << std::endl;
    return;
  }

  AllCounters all_counters;
  std::ifstream input(file_name, std::ios::in | std::ios::binary);

  if (!input.is_open()) {
    std::cerr << "错误：无法打开文件 " << file_name << std::endl;
    return;
  }

  if (!all_counters.ParseFromIstream(&input)) {
    std::cerr << "错误：解析文件失败 " << file_name << std::endl;
    return;
  }

  auto &system_track = perfetto_wrapper_.getSystemTrack();

  // 遍历所有设备的 batch_counters
  for (const auto &[device_name, batch_counter] :
       all_counters.batch_counters()) {
    // 为每个设备创建一个轨道
    auto device_track = perfetto_wrapper_.createNamedTrack(
        "device_" + device_name, "Device: " + device_name, system_track, 0,
        false);

    // 处理该设备的所有 counters
    for (const auto &cnt : batch_counter.counters()) {
      processCounter(cnt, *device_track);
    }
  }
}

void PerfShower::traceAll(const std::string &instructions_file,
                          const std::string &functions_file,
                          const std::string &counters_file) {
  if (!instructions_file.empty()) {
    traceInstructions(instructions_file);
  }
  if (!functions_file.empty()) {
    traceFunctions(functions_file);
  }
  if (!counters_file.empty()) {
    traceCounters(counters_file);
  }
}

void PerfShower::processInstruction(
    const unified_perf_format::Instruction &inst,
    perfetto::Track &parent_track) {
  // 创建 instruction 轨道
  std::string track_name = "inst_" + std::to_string(inst.thread_id()) + "_" +
                           std::to_string(inst.global_seq_num());
  std::string track_show_name = inst.name() + " (" + inst.title() + ")";

  auto track = perfetto_wrapper_.createNamedTrack(
      track_name, track_show_name, parent_track, inst.global_seq_num(), true);

  // 处理所有 stages
  for (const auto &stage : inst.stages()) {
    assert(stage.start_time() <= stage.end_time());

    std::string msg = stage.name();
    // 如果有 metadata，添加到消息中
    if (!stage.metadata().empty()) {
      msg += " [";
      bool first = true;
      for (const auto &[key, value] : stage.metadata()) {
        if (!first)
          msg += ", ";
        msg += key + "=" + value;
        first = false;
      }
      msg += "]";
    }

    perfetto_wrapper_.addTraceEvent(stage.title(), *track, stage.start_time(),
                                    stage.end_time(), msg);
  }
}

void PerfShower::processFunction(const unified_perf_format::Function &func,
                                 perfetto::Track &parent_track) {
  // 创建 function 轨道
  std::string track_name = "func_" + std::to_string(func.thread_id()) + "_" +
                           std::to_string(func.global_seq_num());
  std::string track_show_name = func.title();

  auto track = perfetto_wrapper_.createNamedTrack(
      track_name, track_show_name, parent_track, func.global_seq_num(), true);

  // Functions 通常是点事件，使用一个时间戳
  // 这里假设使用 global_seq_num 作为时间戳（实际应该根据具体需求调整）
  uint64_t timestamp = func.global_seq_num();

  std::string msg = "PC: 0x" + std::to_string(func.pc());
  if (!func.metadata().empty()) {
    msg += " [";
    bool first = true;
    for (const auto &[key, value] : func.metadata()) {
      if (!first)
        msg += ", ";
      msg += key + "=" + value;
      first = false;
    }
    msg += "]";
  }

  // 对于点事件，使用相同的开始和结束时间
  perfetto_wrapper_.addTraceEvent(func.title(), *track, timestamp, timestamp,
                                  msg);
}

void PerfShower::processCounter(const unified_perf_format::Counter &cnt,
                                perfetto::Track &parent_track) {
  // 创建 counter 轨道
  auto counter_track = perfetto_wrapper_.createCounterTrack(
      cnt.name(), cnt.unit(), parent_track);

  // 添加计数器值
  uint64_t cycle = 0;
  for (uint64_t value : cnt.values()) {
    perfetto_wrapper_.addCounterEvent(*counter_track, cycle,
                                      static_cast<double>(value));
    cycle += 1; // 假设每个值间隔一个周期，实际应该根据具体需求调整
  }
}
