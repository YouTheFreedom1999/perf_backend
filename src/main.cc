#include "perf_shower.hh"

int main() {
  PerfShower perf_shower;
  perf_shower.init();
  perf_shower.traceInstructions("data/test_instructions.bin");
  perf_shower.traceFunctions("data/test_functions.bin");
  perf_shower.traceCounters("data/test_counters.bin");
  perf_shower.finish("data/test.perfetto");
  return 0;
}