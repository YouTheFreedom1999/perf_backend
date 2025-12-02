#include "unified_perf_format.pb.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace unified_perf_format;

void GenerateInstructions() {
  std::cout << "Generating Instructions..." << std::endl;
  AllInstructions all_instructions;
  
  // 先创建 batch 并添加数据
  BatchInstruction batch;
  Instruction *inst = batch.add_instructions();
  inst->set_title("Matrix Multiplication");
  inst->set_thread_id(1);
  inst->set_global_seq_num(1001);
  inst->set_device_name("GPU_0");
  inst->set_name("GEMM");
  inst->set_title("Matrix Multiplication");
  (*inst->mutable_metadata())["kernel_size"] = "1024x1024";

  Stage *stage = inst->add_stages();
  stage->set_title("Execution");
  stage->set_name("Execution");
  stage->set_start_time(1000);
  stage->set_end_time(2000);

  stage = inst->add_stages();
  stage->set_name("Execution");
  stage->set_title("Execution");
  stage->set_start_time(2000);
  stage->set_end_time(3000);

  stage = inst->add_stages();
  stage->set_name("Execution2");
  stage->set_title("Execution2");
  stage->set_start_time(3000);
  stage->set_end_time(4000);

  // 在添加完数据后，再将 batch 赋值给 map
  (*all_instructions.mutable_batch_instructions())["GPU_0"] = batch;

  // 序列化 all_instructions 而不是 batch
  std::ofstream output("data/test_instructions.bin",
                       std::ios::out | std::ios::binary);
  if (!all_instructions.SerializeToOstream(&output)) {
    std::cerr << "Failed to write data/test_instructions.bin" << std::endl;
  }
  std::cout << "Wrote data/test_instructions.bin" << std::endl;
}

void GenerateFunctions() {
  std::cout << "Generating Functions..." << std::endl;
  BatchFunction batch;

  Function *func = batch.add_functions();
  func->set_device_name("CPU_0");
  func->set_thread_id(2);
  func->set_global_seq_num(500);
  func->set_pc(0x80001000);
  func->set_insttype(InstType::CALL);
  func->set_title("main_loop");

  std::ofstream output("data/test_functions.bin",
                       std::ios::out | std::ios::binary);
  if (!batch.SerializeToOstream(&output)) {
    std::cerr << "Failed to write data/test_functions.bin" << std::endl;
  }
  std::cout << "Wrote data/test_functions.bin" << std::endl;
}

void GenerateCounters() {
  std::cout << "Generating Counters..." << std::endl;
  BatchCounter batch;

  Counter *cnt = batch.add_counters();
  cnt->set_name("L1_CACHE_MISS");
  cnt->set_title("L1 Cache Misses");
  cnt->set_unit("Count");
  cnt->add_values(10);
  cnt->add_values(15);
  cnt->add_values(25);
  (*cnt->mutable_metadata())["source"] = "PMU";

  std::ofstream output("data/test_counters.bin",
                       std::ios::out | std::ios::binary);
  if (!batch.SerializeToOstream(&output)) {
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
