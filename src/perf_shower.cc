#include "perf_shower.hh"
#include "unified_perf_format.pb.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <vector>

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

  UnifiedPerfData perf_data;
  std::ifstream input(file_name, std::ios::in | std::ios::binary);

  if (!input.is_open()) {
    std::cerr << "错误：无法打开文件 " << file_name << std::endl;
    return;
  }

  if (!perf_data.ParseFromIstream(&input)) {
    std::cerr << "错误：解析文件失败 " << file_name << std::endl;
    return;
  }

  // 检查数据类型
  if (perf_data.data_type() != UnifiedPerfData::INSTRUCTIONS) {
    std::cerr << "错误：文件数据类型不是 INSTRUCTIONS" << std::endl;
    return;
  }

  // 检查是否有 instructions payload
  if (!perf_data.has_instructions()) {
    std::cerr << "错误：UnifiedPerfData 中没有 instructions 数据" << std::endl;
    return;
  }

  auto &system_track = perfetto_wrapper_.getSystemTrack();
  auto &device_name = perf_data.device_name();
  auto &batch_instruction = perf_data.instructions();

  // 为设备创建一个轨道
  auto device_track = perfetto_wrapper_.createNamedTrack(
      "device_" + device_name, "Device: " + device_name, system_track, 0,
      false);

  // 处理该设备的所有 instructions
  for (const auto &inst : batch_instruction.instructions()) {
    processInstruction(inst, *device_track);
  }
  processPipMode(batch_instruction, *device_track);
}

void PerfShower::traceFunctions(const std::string &file_name) {
  if (!initialized_) {
    std::cerr << "错误：请先调用 init() 初始化" << std::endl;
    return;
  }

  UnifiedPerfData perf_data;
  std::ifstream input(file_name, std::ios::in | std::ios::binary);

  if (!input.is_open()) {
    std::cerr << "错误：无法打开文件 " << file_name << std::endl;
    return;
  }

  if (!perf_data.ParseFromIstream(&input)) {
    std::cerr << "错误：解析文件失败 " << file_name << std::endl;
    return;
  }

  // 检查数据类型
  if (perf_data.data_type() != UnifiedPerfData::FUNCTIONS) {
    std::cerr << "错误：文件数据类型不是 FUNCTIONS" << std::endl;
    return;
  }

  // 检查是否有 functions payload
  if (!perf_data.has_functions()) {
    std::cerr << "错误：UnifiedPerfData 中没有 functions 数据" << std::endl;
    return;
  }

  auto &system_track = perfetto_wrapper_.getSystemTrack();
  auto &device_name = perf_data.device_name();
  auto &batch_function = perf_data.functions();

  // 为设备创建一个轨道
  auto device_track = perfetto_wrapper_.createNamedTrack(
      "device_" + device_name, "Device: " + device_name, system_track, 0,
      false);

  // 处理该设备的所有 functions
  for (const auto &func : batch_function.functions()) {
    processFunction(func, *device_track);
  }
}

void PerfShower::traceCounters(const std::string &file_name) {
  if (!initialized_) {
    std::cerr << "错误：请先调用 init() 初始化" << std::endl;
    return;
  }

  UnifiedPerfData perf_data;
  std::ifstream input(file_name, std::ios::in | std::ios::binary);

  if (!input.is_open()) {
    std::cerr << "错误：无法打开文件 " << file_name << std::endl;
    return;
  }

  if (!perf_data.ParseFromIstream(&input)) {
    std::cerr << "错误：解析文件失败 " << file_name << std::endl;
    return;
  }

  // 检查数据类型
  if (perf_data.data_type() != UnifiedPerfData::COUNTERS) {
    std::cerr << "错误：文件数据类型不是 COUNTERS" << std::endl;
    return;
  }

  // 检查是否有 counters payload
  if (!perf_data.has_counters()) {
    std::cerr << "错误：UnifiedPerfData 中没有 counters 数据" << std::endl;
    return;
  }

  auto &system_track = perfetto_wrapper_.getSystemTrack();
  auto &device_name = perf_data.device_name();
  auto &batch_counter = perf_data.counters();

  // 为设备创建一个轨道
  auto device_track = perfetto_wrapper_.createNamedTrack(
      "device_" + device_name, "Device: " + device_name, system_track, 0,
      false);

  // 处理该设备的所有 counters
  for (const auto &cnt : batch_counter.counters()) {
    processCounter(cnt, *device_track);
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

// 辅助函数：检查两个 stage 是否时间重叠
static bool stagesOverlap(const unified_perf_format::Stage &a,
                          const unified_perf_format::Stage &b) {
  // 两个阶段重叠当且仅当：a.start < b.end && b.start < a.end
  return a.start_time() < b.end_time() && b.start_time() < a.end_time();
}

void PerfShower::processPipMode(
    const unified_perf_format::BatchInstruction &batch_instruction,
    perfetto::Track &parent_track) {
  // 定义 track 分配策略
  enum TrackPolicy { SMALL_FIRST, LAST_STEP_FIRST };
  TrackPolicy track_policy = SMALL_FIRST; // 默认使用 SMALL_FIRST 策略

  std::map<std::string, std::vector<const unified_perf_format::Stage *>>
      stage_map;

  // 将所有 stage 按照 name 分组
  for (const auto &inst : batch_instruction.instructions()) {
    for (const auto &st : inst.stages()) {
      assert(st.start_time() <= st.end_time());
      stage_map[st.name()].push_back(&st);
    }
  }

  // 将每个 name 的 stage 按照 start_time 排序
  for (auto &[name, stages] : stage_map) {
    std::sort(stages.begin(), stages.end(),
              [](const unified_perf_format::Stage *a,
                 const unified_perf_format::Stage *b) {
                return a->start_time() < b->start_time();
              });
  }

  // stage_map = {
  //   "Life" : [stage1, stage2, stage3],
  //   "Twice" : [stage4, stage5, stage6],
  // }
  int track_rank_id = 0;

  for (auto &stage_it : stage_map) {
    std::vector<std::queue<const unified_perf_format::Stage *>>
        track_stage_queue_vec;
    int last_step_idx = 0;

    for (auto stage_ptr : stage_it.second) {
      bool found_track = false;
      // 找到一个不重叠的track
      if (track_policy == SMALL_FIRST) {
        for (auto &track : track_stage_queue_vec) {
          if (!track.empty() && !stagesOverlap(*track.back(), *stage_ptr)) {
            track.push(stage_ptr);
            found_track = true;
            break;
          }
        }
      } else if (track_policy == LAST_STEP_FIRST) {
        int cnt = 0;
        for (int idx = last_step_idx; cnt < track_stage_queue_vec.size();
             idx = (idx + 1) % track_stage_queue_vec.size()) {
          if (!track_stage_queue_vec[idx].empty() &&
              !stagesOverlap(*track_stage_queue_vec[idx].back(), *stage_ptr)) {
            track_stage_queue_vec[idx].push(stage_ptr);
            found_track = true;
            last_step_idx = idx;
            break;
          }
          cnt++;
        }
      }
      // 如果没找到，创建一个新track
      if (!found_track) {
        track_stage_queue_vec.push_back(
            std::queue<const unified_perf_format::Stage *>());
        track_stage_queue_vec.back().push(stage_ptr);
        last_step_idx = track_stage_queue_vec.size() - 1;
      }
    }

    int cnt = 0;
    // 遍历每个track 会按照字典序排列
    for (auto &track : track_stage_queue_vec) {
      auto stage_name = stage_it.first + "_" + std::to_string(cnt++);
      auto track_name = stage_name;
      auto track_ptr = perfetto_wrapper_.createNamedTrack(
          track_name, stage_name, parent_track, track_rank_id++, false);

      // 遍历track中的每个stage
      while (!track.empty()) {
        const auto *stage = track.front();
        std::string msg = stage->name();
        // 添加 metadata 信息
        if (!stage->metadata().empty()) {
          msg += " [";
          bool first = true;
          for (const auto &[key, value] : stage->metadata()) {
            if (!first)
              msg += ", ";
            msg += key + "=" + value;
            first = false;
          }
          msg += "]";
        }

        // 使用 instruction 的 global_seq_num 作为 flow_id
        perfetto_wrapper_.addTraceEvent(stage->title(), *track_ptr,
                                        stage->start_time(), stage->end_time(),
                                        msg);
        track.pop();
      }
    }
    track_stage_queue_vec.clear();
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
  msg += " Type: " + std::to_string(static_cast<int>(func.inst_type()));
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
