#include "perf_shower.hh"
#include "unified_perf_format.pb.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

using namespace unified_perf_format;

typedef const google::protobuf::Map<std::string, std::string> MetadataMap;

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

  std::map<std::string, std::vector<const unified_perf_format::Function *>>
      function_stack_map;
  std::map<std::string, std::shared_ptr<perfetto::NamedTrack>>
      function_track_map;

  for (auto &func : batch_function.functions()) {
    auto function_track_name =
        device_name + "_function_t" + std::to_string(func.thread_id());
    if (function_track_map.find(function_track_name) ==
        function_track_map.end()) {
      function_track_map[function_track_name] =
          perfetto_wrapper_.createNamedTrack(function_track_name,
                                             function_track_name, system_track,
                                             func.thread_id());
    }
    auto function_track = function_track_map.at(function_track_name);
    if (func.inst_type() == unified_perf_format::InstType::CALL) {
      function_stack_map[function_track_name].push_back(&func);
    } else if (func.inst_type() == unified_perf_format::InstType::RET) {
      perfetto_wrapper_.addTraceEvent(
          func.name(), *function_track,
          function_stack_map[function_track_name].back()->timestamp(),
          func.timestamp(), func.metadata(), MetadataMap());
      function_stack_map[function_track_name].pop_back();
    } else if (func.inst_type() == unified_perf_format::InstType::POINT_SHOW) {
      perfetto_wrapper_.addTraceEvent(func.name(), *function_track,
                                      func.timestamp(), func.timestamp(),
                                      func.metadata(), MetadataMap());
    }
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
    auto track = perfetto_wrapper_.createCounterTrack(
        "counter_" + cnt.name(), cnt.unit(), system_track);

    uint64_t last_timestamp = 0;
    for (auto &value : cnt.values()) {
      perfetto_wrapper_.addCounterEvent(*track, value.timestamp(),
                                        value.value());
      last_timestamp = value.timestamp();
    }
  }
}

void PerfShower::processInstruction(
    const unified_perf_format::Instruction &inst,
    perfetto::Track &parent_track) {
  // 创建 instruction 轨道
  std::string track_name = "inst_" + std::to_string(inst.thread_id()) + "_" +
                           std::to_string(inst.global_seq_num());

  auto track = perfetto_wrapper_.createNamedTrack(
      track_name, inst.name(), parent_track, inst.global_seq_num(), true);

  // 处理所有 stages
  for (const auto &stage : inst.stages()) {
    assert(stage.start_time() <= stage.end_time());
    perfetto_wrapper_.addTraceEvent(stage.name(), *track, stage.start_time(),
                                    stage.end_time(), inst.metadata(),
                                    stage.metadata());
  }
}


void PerfShower::showCounter(const std::string &perf_path , ) {
  if (!initialized_) {
    std::cerr << "错误：请先调用 init() 初始化" << std::endl;
    return;
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

  std::map<
      std::string,
      std::vector<std::pair<const unified_perf_format::Stage *, MetadataMap *>>>
      stage_map;

  // 将所有 stage 按照 name 分组
  for (const auto &inst : batch_instruction.instructions()) {
    for (const auto &st : inst.stages()) {
      assert(st.start_time() <= st.end_time());
      stage_map[st.name()].push_back(std::make_pair(&st, &inst.metadata()));
    }
  }

  // 将每个 name 的 stage 按照 start_time 排序
  for (auto &[name, stages] : stage_map) {
    std::sort(
        stages.begin(), stages.end(),
        [](const std::pair<const unified_perf_format::Stage *, MetadataMap *>
               &a,
           const std::pair<const unified_perf_format::Stage *, MetadataMap *>
               &b) { return a.first->start_time() < b.first->start_time(); });
  }

  // stage_map = {
  //   "Life" : [stage1, stage2, stage3],
  //   "Twice" : [stage4, stage5, stage6],
  // }
  int track_rank_id = 0;

  for (auto &stage_it : stage_map) {
    std::vector<std::queue<
        std::pair<const unified_perf_format::Stage *, MetadataMap *>>>
        track_stage_queue_vec;
    int last_step_idx = 0;

    for (auto stage_ptr : stage_it.second) {
      bool found_track = false;
      // 找到一个不重叠的track
      if (track_policy == SMALL_FIRST) {
        for (auto &track : track_stage_queue_vec) {
          if (!track.empty() &&
              !stagesOverlap(*track.back().first, *stage_ptr.first)) {
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
              !stagesOverlap(*track_stage_queue_vec[idx].back().first,
                             *stage_ptr.first)) {
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
            std::queue<std::pair<const unified_perf_format::Stage *,
                                 MetadataMap *>>());
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
        const auto *stage = track.front().first;
        // 使用 instruction 的 global_seq_num 作为 flow_id
        perfetto_wrapper_.addTraceEvent(
            stage->name(), *track_ptr, stage->start_time(), stage->end_time(),
            (*(track.front().second)), stage->metadata());
        track.pop();
      }
    }
    track_stage_queue_vec.clear();
  }
}