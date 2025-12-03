#include "perf_shower.hh"

int main() {
  PerfShower perf_shower;
  perf_shower.init();
  
  // 使用新的 show() 方法，根据 show.json 配置显示数据
  perf_shower.show("data/show.json", "data/test_unified.bin");
  
  // 也可以继续使用旧的方法（用于兼容性）
  // perf_shower.traceInstructions("data/test_instructions.bin");
  // perf_shower.traceFunctions("data/test_functions.bin");
  // perf_shower.traceCounters("data/test_counters.bin");
  
  perf_shower.finish("data/test.perfetto");
  return 0;
}