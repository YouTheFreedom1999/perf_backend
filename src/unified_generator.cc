#include "unified_perf_format.pb.h"
#include <cmath>
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
  perf_data.set_device_name("GPU");
  
  // 创建 batch 并添加数据
  BatchInstruction *batch = perf_data.mutable_instructions();
  Instruction *inst = batch->add_instructions();
  inst->set_thread_id(1);
  inst->set_global_seq_num(1001);
  inst->set_name("GEMM");
  (*inst->mutable_metadata())["kernel_size"] = "1024x1024";
  (*inst->mutable_metadata())["kernel_type"] = "GEMM";
  (*inst->mutable_metadata())["kernel_name"] = "GEMM";

  Stage *stage = inst->add_stages();
  stage->set_name("Execution");
  stage->set_order_id(0);
  stage->set_start_time(1000);
  stage->set_end_time(2000);

  stage = inst->add_stages();
  stage->set_name("Execution");
  stage->set_order_id(1);
  stage->set_start_time(2000);
  stage->set_end_time(3000);

  stage = inst->add_stages();
  stage->set_name("Execution2");
  stage->set_order_id(2);
  stage->set_start_time(3000);
  stage->set_end_time(4000);

  // 序列化 UnifiedPerfData（保留单独文件用于兼容性）
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
  func->set_name("main");
  func->set_thread_id(0);
  func->set_pc(0x80001000);
  func->set_timestamp(1000);
  func->set_inst_type(InstType::CALL);

  func = batch->add_functions();
  func->set_name("func1");
  func->set_thread_id(0);
  func->set_pc(0x80002000);
  func->set_timestamp(2000);
  func->set_inst_type(InstType::CALL);

  func = batch->add_functions();
  func->set_name("func1");
  func->set_thread_id(0);
  func->set_pc(0x8000000);
  func->set_timestamp(4000);
  func->set_inst_type(InstType::RET);

  func = batch->add_functions();
  func->set_name("main");
  func->set_thread_id(0);
  func->set_pc(0x80008000);
  func->set_timestamp(5000);
  func->set_inst_type(InstType::RET);


  func = batch->add_functions();
  func->set_name("sync");
  func->set_thread_id(0);
  func->set_pc(0x800032000);
  func->set_timestamp(3000);
  func->set_inst_type(InstType::POINT_SHOW);

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
  perf_data.set_device_name("CPU");

  BatchCounter *batch = perf_data.mutable_counters();
  Counter *cnt = batch->add_counters();
  cnt->set_name("Freq");
  cnt->set_unit("GHz");
  CntValue *value;

  for(int i = 0 ;  i < 5000 ; i ++) {
    // 绘制sin函数曲线 (生成sin曲线形式的Counter Value)
    value = cnt->add_values();
    double t = i * 0.01;
    value->set_timestamp(1000 + i);
    value->set_value(std::cos(t));
  }

  (*cnt->mutable_metadata())["source"] = "PMU";

  std::ofstream output("data/test_counters.bin",
                       std::ios::out | std::ios::binary);
  if (!perf_data.SerializeToOstream(&output)) {
    std::cerr << "Failed to write data/test_counters.bin" << std::endl;
  }
  std::cout << "Wrote data/test_counters.bin" << std::endl;
}

void GenerateUnifiedFile() {
  std::cout << "Generating unified file with all data types..." << std::endl;
  
  // 使用容器消息来包含所有数据
  UnifiedPerfDataContainer container;
  
  // 创建 Instructions 数据
  UnifiedPerfData perf_data_inst;
  perf_data_inst.set_data_type(UnifiedPerfData::INSTRUCTIONS);
  perf_data_inst.set_device_name("GPU");
  BatchInstruction *batch_inst = perf_data_inst.mutable_instructions();
  Instruction *inst = batch_inst->add_instructions();
  inst->set_thread_id(1);
  inst->set_global_seq_num(1001);
  inst->set_name("GEMM");
  (*inst->mutable_metadata())["kernel_size"] = "1024x1024";
  Stage *stage = inst->add_stages();
  stage->set_name("Execution");
  stage->set_order_id(0);
  stage->set_start_time(1000);
  stage->set_end_time(2000);
  stage = inst->add_stages();
  stage->set_name("Execution");
  stage->set_order_id(1);
  stage->set_start_time(2000);
  stage->set_end_time(3000);
  stage = inst->add_stages();
  stage->set_name("Execution2");
  stage->set_order_id(2);
  stage->set_start_time(3000);
  stage->set_end_time(4000);
  container.add_data_list()->CopyFrom(perf_data_inst);
  
  // 创建 Functions 数据
  UnifiedPerfData perf_data_func;
  perf_data_func.set_data_type(UnifiedPerfData::FUNCTIONS);
  perf_data_func.set_device_name("CPU_0");
  BatchFunction *batch_func = perf_data_func.mutable_functions();
  Function *func = batch_func->add_functions();
  func->set_name("main");
  func->set_thread_id(0);
  func->set_pc(0x80001000);
  func->set_timestamp(1000);
  func->set_inst_type(InstType::CALL);
  func = batch_func->add_functions();
  func->set_name("func1");
  func->set_thread_id(0);
  func->set_pc(0x80002000);
  func->set_timestamp(2000);
  func->set_inst_type(InstType::CALL);
  func = batch_func->add_functions();
  func->set_name("func1");
  func->set_thread_id(0);
  func->set_pc(0x8000000);
  func->set_timestamp(4000);
  func->set_inst_type(InstType::RET);
  func = batch_func->add_functions();
  func->set_name("main");
  func->set_thread_id(0);
  func->set_pc(0x80008000);
  func->set_timestamp(5000);
  func->set_inst_type(InstType::RET);
  func = batch_func->add_functions();
  func->set_name("sync");
  func->set_thread_id(0);
  func->set_pc(0x800032000);
  func->set_timestamp(3000);
  func->set_inst_type(InstType::POINT_SHOW);
  container.add_data_list()->CopyFrom(perf_data_func);
  
  // 创建 Counters 数据
  UnifiedPerfData perf_data_cnt;
  perf_data_cnt.set_data_type(UnifiedPerfData::COUNTERS);
  perf_data_cnt.set_device_name("CPU");
  BatchCounter *batch_cnt = perf_data_cnt.mutable_counters();
  Counter *cnt = batch_cnt->add_counters();
  cnt->set_name("Freq");
  cnt->set_unit("GHz");
  for(int i = 0; i < 5000; i++) {
    CntValue *value = cnt->add_values();
    double t = i * 0.01;
    value->set_timestamp(1000 + i);
    value->set_value(std::cos(t));
  }
  (*cnt->mutable_metadata())["source"] = "PMU";
  container.add_data_list()->CopyFrom(perf_data_cnt);
  
  // 序列化容器消息到文件
  std::ofstream output("data/test_unified.bin", std::ios::out | std::ios::binary);
  if (!container.SerializeToOstream(&output)) {
    std::cerr << "Failed to write data/test_unified.bin" << std::endl;
  }
  std::cout << "Wrote data/test_unified.bin" << std::endl;
}

int main() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  GenerateInstructions();
  GenerateFunctions();
  GenerateCounters();
  
  // 生成统一文件
  GenerateUnifiedFile();

  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
