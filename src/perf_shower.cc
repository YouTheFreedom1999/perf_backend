#include "perf_shower.hh"
#include "unified_perf_format.pb.h"
#include "../lib/json.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <utility>
#include <vector>
#include <sstream>
#include <set>
#include <fcntl.h>
#include <unistd.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

using namespace unified_perf_format;
using google::protobuf::io::FileInputStream;
using google::protobuf::io::CodedInputStream;
using json = nlohmann::json;

// 注意：已不再使用带长度前缀的读取方式
// 现在使用 UnifiedPerfDataContainer 容器消息，可以直接用 ParseFromIstream 读取

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

void PerfShower::processLineMode(
    const unified_perf_format::BatchInstruction &batch_instruction,
    perfetto::Track &parent_track) {
  // Line 模式：按照 instruction 的顺序线性显示
  int track_rank_id = 0;
  for (const auto &inst : batch_instruction.instructions()) {
    std::string track_name = "inst_" + std::to_string(inst.thread_id()) + "_" +
                             std::to_string(inst.global_seq_num());
    auto track = perfetto_wrapper_.createNamedTrack(
        track_name, inst.name(), parent_track, track_rank_id++, true);
    
    // 按照 stage 的顺序线性添加
    for (const auto &stage : inst.stages()) {
      perfetto_wrapper_.addTraceEvent(stage.name(), *track, stage.start_time(),
                                      stage.end_time(), inst.metadata(),
                                      stage.metadata());
    }
  }
}

void PerfShower::processFuncMode(
    const unified_perf_format::BatchFunction &batch_function,
    perfetto::Track &parent_track) {
  std::map<std::string, std::vector<const unified_perf_format::Function *>>
      function_stack_map;
  std::map<std::string, std::shared_ptr<perfetto::NamedTrack>>
      function_track_map;

  for (const auto &func : batch_function.functions()) {
    auto function_track_name =
        "function_t" + std::to_string(func.thread_id());
    if (function_track_map.find(function_track_name) ==
        function_track_map.end()) {
      function_track_map[function_track_name] =
          perfetto_wrapper_.createNamedTrack(function_track_name,
                                             function_track_name, parent_track,
                                             func.thread_id());
    }
    auto function_track = function_track_map.at(function_track_name);
    if (func.inst_type() == unified_perf_format::InstType::CALL) {
      function_stack_map[function_track_name].push_back(&func);
    } else if (func.inst_type() == unified_perf_format::InstType::RET) {
      if (!function_stack_map[function_track_name].empty()) {
        perfetto_wrapper_.addTraceEvent(
            func.name(), *function_track,
            function_stack_map[function_track_name].back()->timestamp(),
            func.timestamp(), func.metadata(), MetadataMap());
        function_stack_map[function_track_name].pop_back();
      }
    } else if (func.inst_type() == unified_perf_format::InstType::POINT_SHOW) {
      perfetto_wrapper_.addTraceEvent(func.name(), *function_track,
                                      func.timestamp(), func.timestamp(),
                                      func.metadata(), MetadataMap());
    }
  }
}

void PerfShower::processCntMode(
    const unified_perf_format::BatchCounter &batch_counter,
    perfetto::Track &parent_track) {
  for (const auto &cnt : batch_counter.counters()) {
    auto track = perfetto_wrapper_.createCounterTrack(
        "counter_" + cnt.name(), cnt.unit(), parent_track);

    for (const auto &value : cnt.values()) {
      perfetto_wrapper_.addCounterEvent(*track, value.timestamp(),
                                        value.value());
    }
  }
}

JsonConfig PerfShower::parseShowJson(const std::string &json_path) {
  JsonConfig config;
  
  std::ifstream file(json_path);
  if (!file.is_open()) {
    std::cerr << "错误：无法打开 JSON 文件 " << json_path << std::endl;
    return config;
  }

  json j;
  try {
    file >> j;
  } catch (const json::parse_error &e) {
    std::cerr << "错误：JSON 解析失败: " << e.what() << std::endl;
    return config;
  }

  // 解析 filelist 字段（如果存在）
  if (j.contains("filelist") && j["filelist"].is_array()) {
    for (const auto &file_path : j["filelist"]) {
      if (file_path.is_string()) {
        config.filelist.push_back(file_path.get<std::string>());
      }
    }
    std::cout << "从 JSON 配置中读取到 " << config.filelist.size() << " 个输入文件" << std::endl;
  }

  // 解析 output 字段（如果存在）
  if (j.contains("output") && j["output"].is_string()) {
    config.output = j["output"].get<std::string>();
    std::cout << "从 JSON 配置中读取到输出文件路径: " << config.output << std::endl;
  }

  // 解析视图配置
  for (auto it = j.begin(); it != j.end(); ++it) {
    const std::string &view_name = it.key();
    // 跳过 "filelist" 和 "output" 字段，它们不是视图配置
    if (view_name == "filelist" || view_name == "output") {
      continue;
    }
    
    const json &view_obj = it.value();
    
    // 只处理对象类型的视图配置
    if (!view_obj.is_object()) {
      continue;
    }
    
    ViewConfig view_config;
    if (view_obj.contains("mode")) {
      view_config.mode = view_obj["mode"].get<std::string>();
    }
    
    // 解析过滤器数组：如果 JSON 中没有指定某个 filter 字段，则 filters 保持为空
    // 空的 filters 表示不进行筛选（所有数据都通过）
    auto parseFilterArray = [&view_obj](const std::string &key, std::vector<FilterRule> &filters) {
      if (view_obj.contains(key) && view_obj[key].is_array()) {
        for (const auto &rule : view_obj[key]) {
          if (rule.is_string()) {
            filters.push_back(FilterRule(rule.get<std::string>()));
          }
        }
      }
      // 如果 JSON 中没有这个字段，filters 保持为空，表示不过滤
    };

    parseFilterArray("timeline_filter", view_config.timeline_filter);
    parseFilterArray("event_filter", view_config.event_filter);
    parseFilterArray("track_filter", view_config.track_filter);
    parseFilterArray("device_filter", view_config.device_filter);
    parseFilterArray("thread_filter", view_config.thread_filter);

    config.views[view_name] = view_config;
  }

  return config;
}

bool PerfShower::passTimelineFilter(const std::vector<FilterRule> &filters, 
                                    uint64_t start_time, uint64_t end_time) {
  // 如果过滤器为空（JSON 中未指定），则通过所有数据
  if (filters.empty()) return true;
  
  for (const auto &rule : filters) {
    // 解析时间范围，格式: "start-end" 或单个时间戳
    std::string value = rule.value;
    size_t dash_pos = value.find('-');
    if (dash_pos != std::string::npos) {
      uint64_t filter_start = std::stoull(value.substr(0, dash_pos));
      uint64_t filter_end = std::stoull(value.substr(dash_pos + 1));
      // 检查时间范围是否有重叠：只要 stage 的时间范围与 filter 的时间范围有任何重叠，
      // 就返回 true（整个 stage 都会被绘制，而不仅仅是重叠的部分）
      // 重叠条件：stage.start <= filter.end && stage.end >= filter.start
      if (start_time <= filter_end && end_time >= filter_start) {
        return true;
      }
    } else {
      // 单个时间戳，检查该时间戳是否在 stage 的时间范围内
      uint64_t timestamp = std::stoull(value);
      if (timestamp >= start_time && timestamp <= end_time) {
        return true;
      }
    }
  }
  return false;
}

bool PerfShower::passEventFilter(const std::vector<FilterRule> &filters, 
                                 const std::string &event_name) {
  // 如果过滤器为空（JSON 中未指定），则通过所有数据
  if (filters.empty()) return true;
  
  for (const auto &rule : filters) {
    if (event_name.find(rule.value) != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool PerfShower::passTrackFilter(const std::vector<FilterRule> &filters, 
                                 const std::string &track_name) {
  // 如果过滤器为空（JSON 中未指定），则通过所有数据
  if (filters.empty()) return true;
  
  for (const auto &rule : filters) {
    if (track_name.find(rule.value) != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool PerfShower::passDeviceFilter(const std::vector<FilterRule> &filters, 
                                  const std::string &device_name) {
  // 如果过滤器为空（JSON 中未指定），则通过所有数据
  if (filters.empty()) return true;
  
  for (const auto &rule : filters) {
    if (device_name.find(rule.value) != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool PerfShower::passThreadFilter(const std::vector<FilterRule> &filters, 
                                  uint32_t thread_id) {
  // 如果过滤器为空（JSON 中未指定），则通过所有数据
  if (filters.empty()) return true;
  
  for (const auto &rule : filters) {
    uint32_t filter_thread = std::stoul(rule.value);
    if (thread_id == filter_thread) {
      return true;
    }
  }
  return false;
}

std::vector<UnifiedPerfData> PerfShower::readPerfDataFromFile(const std::string &bin_file_path) {
  std::vector<UnifiedPerfData> perf_data_list;
  
  std::ifstream file_stream(bin_file_path, std::ios::in | std::ios::binary);
  if (!file_stream.is_open()) {
    std::cerr << "错误：无法打开文件 " << bin_file_path << std::endl;
    return perf_data_list;
  }

  // 先尝试读取容器消息格式（推荐方式）
  unified_perf_format::UnifiedPerfDataContainer container;
  if (container.ParseFromIstream(&file_stream)) {
    // 成功读取容器消息，提取所有数据
    for (int i = 0; i < container.data_list_size(); i++) {
      perf_data_list.push_back(container.data_list(i));
    }
    std::cout << "使用容器消息格式读取，共 " << perf_data_list.size() << " 个数据块" << std::endl;
    return perf_data_list;
  }
  
  // 如果容器消息格式失败，尝试单个 UnifiedPerfData 格式（向后兼容）
  file_stream.clear();
  file_stream.seekg(0, std::ios::beg);
  UnifiedPerfData perf_data;
  if (perf_data.ParseFromIstream(&file_stream)) {
    perf_data_list.push_back(perf_data);
    std::cout << "使用单个消息格式读取" << std::endl;
    return perf_data_list;
  }
  
  std::cerr << "错误：无法解析文件 " << bin_file_path 
            << "（既不是容器消息格式，也不是单个消息格式）" << std::endl;
  return perf_data_list;
}

std::vector<UnifiedPerfData> PerfShower::readPerfDataFromFiles(const std::vector<std::string> &bin_file_paths) {
  std::vector<UnifiedPerfData> merged_perf_data_list;
  
  for (const auto &bin_file_path : bin_file_paths) {
    std::cout << "读取性能数据文件: " << bin_file_path << std::endl;
    auto perf_data_list = readPerfDataFromFile(bin_file_path);
    if (!perf_data_list.empty()) {
      // 将读取到的数据合并到总列表中
      merged_perf_data_list.insert(merged_perf_data_list.end(), 
                                    perf_data_list.begin(), 
                                    perf_data_list.end());
      std::cout << "成功从文件 " << bin_file_path << " 读取 " 
                << perf_data_list.size() << " 个数据块" << std::endl;
    } else {
      std::cerr << "警告：文件 " << bin_file_path << " 未读取到任何数据" << std::endl;
    }
  }
  
  std::cout << "总共读取 " << merged_perf_data_list.size() << " 个数据块" << std::endl;
  return merged_perf_data_list;
}

void PerfShower::processDataWithView(const ViewConfig &view_config, 
                                     const std::vector<UnifiedPerfData> &perf_data_list,
                                     perfetto::Track &view_track) {
  // 注意：此方法不会修改 perf_data_list 中的原始数据
  // 所有过滤操作都在新创建的对象上进行（filtered_batch, filtered_inst 等）
  // 使用 CopyFrom() 复制数据，确保原始数据保持不变
  
  std::cout << "processDataWithView: 处理 " << perf_data_list.size() << " 个数据块，模式: " << view_config.mode << std::endl;
  
  // 缓存已创建的 device track，避免重复创建
  std::map<std::string, std::shared_ptr<perfetto::NamedTrack>> device_track_map;
  
  // 处理每个消息
  for (const auto &perf_data : perf_data_list) {
    std::cout << "  处理数据块: device_name=" << perf_data.device_name() 
              << ", data_type=" << perf_data.data_type() 
              << ", has_instructions=" << perf_data.has_instructions()
              << ", has_functions=" << perf_data.has_functions()
              << ", has_counters=" << perf_data.has_counters() << std::endl;
    
    // 检查设备过滤器
    if (!passDeviceFilter(view_config.device_filter, perf_data.device_name())) {
      std::cout << "    设备过滤器未通过，跳过" << std::endl;
      continue;
    }

    // 检查是否已经为该设备创建了 track，如果没有则创建
    std::shared_ptr<perfetto::NamedTrack> device_track;
    const std::string &device_name = perf_data.device_name();
    auto it = device_track_map.find(device_name);
    if (it != device_track_map.end()) {
      // 复用已创建的 track
      device_track = it->second;
      std::cout << "    复用已存在的 device track: " << device_name << std::endl;
    } else {
      // 创建新的 device track
      device_track = perfetto_wrapper_.createNamedTrack(
          "device_" + device_name, "Device: " + device_name, 
          view_track, 0, false);
      device_track_map[device_name] = device_track;
      std::cout << "    创建新的 device track: " << device_name << std::endl;
    }

  // 根据 mode 处理数据
  if (view_config.mode == "pipe" && perf_data.has_instructions()) {
    // 应用过滤器处理 instructions
    auto &batch_instruction = perf_data.instructions();
    std::cout << "    处理 pipe 模式，共有 " << batch_instruction.instructions_size() << " 个指令" << std::endl;
    unified_perf_format::BatchInstruction filtered_batch;
    
    for (const auto &inst : batch_instruction.instructions()) {
      if (!passThreadFilter(view_config.thread_filter, inst.thread_id())) {
        continue;
      }

      bool has_valid_stage = false;
      unified_perf_format::Instruction filtered_inst;
      filtered_inst.CopyFrom(inst);
      filtered_inst.clear_stages();
      
      for (const auto &stage : inst.stages()) {
        if (passTimelineFilter(view_config.timeline_filter, 
                               stage.start_time(), stage.end_time()) &&
            passEventFilter(view_config.event_filter, stage.name())) {
          auto *new_stage = filtered_inst.add_stages();
          new_stage->CopyFrom(stage);
          has_valid_stage = true;
        }
      }
      
      if (has_valid_stage) {
        auto *new_inst = filtered_batch.add_instructions();
        new_inst->CopyFrom(filtered_inst);
      }
    }
    
    std::cout << "    过滤后剩余 " << filtered_batch.instructions_size() << " 个有效指令" << std::endl;
    if (filtered_batch.instructions_size() > 0) {
      processPipMode(filtered_batch, *device_track);
      std::cout << "    已调用 processPipMode" << std::endl;
    } else {
      std::cout << "    警告：没有有效指令，跳过 processPipMode" << std::endl;
    }
    
  } else if (view_config.mode == "line" && perf_data.has_instructions()) {
    std::cout << "    处理 line 模式，共有 " << perf_data.instructions().instructions_size() << " 个指令" << std::endl;
    auto &batch_instruction = perf_data.instructions();
    unified_perf_format::BatchInstruction filtered_batch;
    
    for (const auto &inst : batch_instruction.instructions()) {
      if (!passThreadFilter(view_config.thread_filter, inst.thread_id())) {
        continue;
      }
      
      bool has_valid_stage = false;
      unified_perf_format::Instruction filtered_inst;
      filtered_inst.CopyFrom(inst);
      filtered_inst.clear_stages();
      
      for (const auto &stage : inst.stages()) {
        if (passTimelineFilter(view_config.timeline_filter, 
                               stage.start_time(), stage.end_time()) &&
            passEventFilter(view_config.event_filter, stage.name())) {
          auto *new_stage = filtered_inst.add_stages();
          new_stage->CopyFrom(stage);
          has_valid_stage = true;
        }
      }
      
      if (has_valid_stage) {
        auto *new_inst = filtered_batch.add_instructions();
        new_inst->CopyFrom(filtered_inst);
      }
    }
    
    if (filtered_batch.instructions_size() > 0) {
      processLineMode(filtered_batch, *device_track);
    }
    
  } else if (view_config.mode == "func" && perf_data.has_functions()) {
    std::cout << "    处理 func 模式，共有 " << perf_data.functions().functions_size() << " 个函数" << std::endl;
    auto &batch_function = perf_data.functions();
    unified_perf_format::BatchFunction filtered_batch;
    
    for (const auto &func : batch_function.functions()) {
      if (!passThreadFilter(view_config.thread_filter, func.thread_id())) {
        continue;
      }
      
      if (passTimelineFilter(view_config.timeline_filter, 
                             func.timestamp(), func.timestamp()) &&
          passEventFilter(view_config.event_filter, func.name())) {
        auto *new_func = filtered_batch.add_functions();
        new_func->CopyFrom(func);
      }
    }
    
    if (filtered_batch.functions_size() > 0) {
      processFuncMode(filtered_batch, *device_track);
    }
    
  } else if (view_config.mode == "cnt" && perf_data.has_counters()) {
    std::cout << "    处理 cnt 模式，共有 " << perf_data.counters().counters_size() << " 个计数器" << std::endl;
    auto &batch_counter = perf_data.counters();
    unified_perf_format::BatchCounter filtered_batch;
    
    for (const auto &cnt : batch_counter.counters()) {
      if (!passTrackFilter(view_config.track_filter, cnt.name())) {
        continue;
      }
      
      unified_perf_format::Counter filtered_cnt;
      filtered_cnt.CopyFrom(cnt);
      filtered_cnt.clear_values();
      
      for (const auto &value : cnt.values()) {
        if (passTimelineFilter(view_config.timeline_filter, 
                               value.timestamp(), value.timestamp())) {
          auto *new_value = filtered_cnt.add_values();
          new_value->CopyFrom(value);
        }
      }
      
      if (filtered_cnt.values_size() > 0) {
        auto *new_cnt = filtered_batch.add_counters();
        new_cnt->CopyFrom(filtered_cnt);
      }
    }
    
    if (filtered_batch.counters_size() > 0) {
      processCntMode(filtered_batch, *device_track);
    }
  } else {
    // 模式不匹配或数据类型不匹配
    std::cout << "    警告：模式 " << view_config.mode << " 与数据类型不匹配" << std::endl;
    std::cout << "      mode=" << view_config.mode 
              << ", has_instructions=" << perf_data.has_instructions()
              << ", has_functions=" << perf_data.has_functions()
              << ", has_counters=" << perf_data.has_counters() << std::endl;
  }
  }
}
std::string PerfShower::show(const std::string &show_json_path) {
  std::string output_path;
  
  if (!initialized_) {
    std::cerr << "错误：请先调用 init() 初始化" << std::endl;
    return output_path;
  }

  auto json_config = parseShowJson(show_json_path);
  if (json_config.views.empty()) {
    std::cerr << "错误：没有找到有效的视图配置" << std::endl;
    return output_path;
  }

  // 从 JSON 配置中读取 filelist
  std::vector<std::string> final_file_paths;
  if (!json_config.filelist.empty()) {
    final_file_paths = json_config.filelist;
    std::cout << "使用 JSON 配置中的 filelist，共 " << final_file_paths.size() << " 个文件" << std::endl;
  } else {
    std::cerr << "错误：JSON 配置文件中未指定 'filelist' 字段" << std::endl;
    std::cerr << "请在 JSON 配置文件中添加 'filelist' 字段，例如：" << std::endl;
    std::cerr << "  \"filelist\": [\"file1.bin\", \"file2.bin\"]" << std::endl;
    return output_path;
  }

  if (final_file_paths.empty()) {
    std::cerr << "错误：JSON 配置中的 'filelist' 字段为空" << std::endl;
    return output_path;
  }

  // 从 JSON 配置中读取 output 路径
  if (!json_config.output.empty()) {
    output_path = json_config.output;
  } else {
    std::cerr << "错误：JSON 配置文件中未指定 'output' 字段" << std::endl;
    std::cerr << "请在 JSON 配置文件中添加 'output' 字段，例如：" << std::endl;
    std::cerr << "  \"output\": \"data/test.perfetto\"" << std::endl;
    return output_path;
  }

  // 从多个文件读取性能数据并合并
  auto perf_data_list = readPerfDataFromFiles(final_file_paths);
  if (perf_data_list.empty()) {
    std::cerr << "错误：未能从文件读取到任何数据" << std::endl;
    return output_path;
  }

  auto &system_track = perfetto_wrapper_.getSystemTrack();
  int view_rank = 0;

  // 处理每个视图，为每个 view 创建一个独立的 track
  for (auto it = json_config.views.begin(); it != json_config.views.end(); ++it) {
    const std::string &view_name = it->first;
    const ViewConfig &view_config = it->second;
    
    std::cout << "处理视图: " << view_name << ", 模式: " << view_config.mode << std::endl;
    
    // 为每个 view 创建一个 track，view_name 作为 track 名称
    auto view_track = perfetto_wrapper_.createNamedTrack(
        "view_" + view_name, view_name, system_track, view_rank++, false);
    
    // 在该 view 的 track 下处理数据（使用已读取的数据）
    processDataWithView(view_config, perf_data_list, *view_track);
    std::cout << "视图 " << view_name << " 处理完成" << std::endl;
  }

  std::cout << "所有视图处理完成，准备返回输出路径: " << output_path << std::endl;
  return output_path;
}