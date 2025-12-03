#include "unified_perf_format.pb.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace unified_perf_format;

void GenerateInstructions() {
  std::cout << "Generating Instructions..." << std::endl;
  UnifiedPerfData perf_data;
  
  // 设置数据类型和设备名
  perf_data.set_data_type(UnifiedPerfData::INSTRUCTIONS);
  perf_data.set_device_name("GPU_0");
  
  // 创建 batch 并添加数据
  BatchInstruction *batch = perf_data.mutable_instructions();
  Instruction *inst = batch->add_instructions();
  inst->set_thread_id(1);
  inst->set_global_seq_num(1001);
  inst->set_device_name("GPU_0");
  inst->set_name("GEMM");
  inst->set_title("Matrix Multiplication");
  (*inst->mutable_metadata())["kernel_size"] = "1024x1024";
  // 添加父序列号示例
  inst->add_parent_seq_num(1000);

  Stage *stage = inst->add_stages();
  stage->set_name("Execution");
  stage->set_title("Execution");
  stage->set_order_id(0);
  stage->set_start_time(1000);
  stage->set_end_time(2000);

  stage = inst->add_stages();
  stage->set_name("Execution");
  stage->set_title("Execution");
  stage->set_order_id(1);
  stage->set_start_time(2000);
  stage->set_end_time(3000);

  stage = inst->add_stages();
  stage->set_name("Execution2");
  stage->set_title("Execution2");
  stage->set_order_id(2);
  stage->set_start_time(3000);
  stage->set_end_time(4000);

  // 序列化 UnifiedPerfData
  std::ofstream output("data/test_instructions.bin",
                       std::ios::out | std::ios::binary);
  if (!perf_data.SerializeToOstream(&output)) {
    std::cerr << "Failed to write data/test_instructions.bin" << std::endl;
  }
  std::cout << "Wrote data/test_instructions.bin" << std::endl;
}

void GenerateFunctions() {
  std::cout << "Generating Functions..." << std::endl;
  UnifiedPerfData perf_data;
  
  // 设置数据类型和设备名
  perf_data.set_data_type(UnifiedPerfData::FUNCTIONS);
  perf_data.set_device_name("CPU_0");

  BatchFunction *batch = perf_data.mutable_functions();
  Function *func = batch->add_functions();
  func->set_device_name("CPU_0");
  func->set_thread_id(2);
  func->set_global_seq_num(500);
  func->set_pc(0x80001000);
  func->set_inst_type(InstType::CALL);
  func->set_title("main_loop");

  std::ofstream output("data/test_functions.bin",
                       std::ios::out | std::ios::binary);
  if (!perf_data.SerializeToOstream(&output)) {
    std::cerr << "Failed to write data/test_functions.bin" << std::endl;
  }
  std::cout << "Wrote data/test_functions.bin" << std::endl;
}

void GenerateCounters() {
  std::cout << "Generating Counters..." << std::endl;
  UnifiedPerfData perf_data;
  
  // 设置数据类型和设备名
  perf_data.set_data_type(UnifiedPerfData::COUNTERS);
  perf_data.set_device_name("CPU_0");

  BatchCounter *batch = perf_data.mutable_counters();
  Counter *cnt = batch->add_counters();
  cnt->set_name("L1_CACHE_MISS");
  cnt->set_title("L1 Cache Misses");
  cnt->set_unit("Count");
  cnt->add_values(10);
  cnt->add_values(15);
  cnt->add_values(25);
  (*cnt->mutable_metadata())["source"] = "PMU";

  std::ofstream output("data/test_counters.bin",
                       std::ios::out | std::ios::binary);
  if (!perf_data.SerializeToOstream(&output)) {
    std::cerr << "Failed to write data/test_counters.bin" << std::endl;
  }
  std::cout << "Wrote data/test_counters.bin" << std::endl;
}

int main() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  GenerateInstructions();
  GenerateFunctions();
  GenerateCounters();

  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
