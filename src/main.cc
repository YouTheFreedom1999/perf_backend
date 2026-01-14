#include "perf_shower.hh"
#include <iostream>
#include <cstring>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
  // 默认值
  const char* json_config = "data/show.json";
  std::vector<std::string> bin_inputs;
  bin_inputs.push_back("data/test_unified.bin");  // 默认文件
  const char* output_file = "data/test.perfetto";
  
  // 解析命令行参数
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--json") == 0 || strcmp(argv[i], "-j") == 0) {
      if (i + 1 < argc) {
        json_config = argv[++i];
      } else {
        std::cerr << "错误: --json 需要指定文件路径" << std::endl;
        return 1;
      }
    } else if (strcmp(argv[i], "--bin") == 0 || strcmp(argv[i], "-b") == 0) {
      if (i + 1 < argc) {
        // 如果这是第一次指定 --bin，清空默认值
        if (bin_inputs.size() == 1 && bin_inputs[0] == "data/test_unified.bin") {
          bin_inputs.clear();
        }
        bin_inputs.push_back(argv[++i]);
      } else {
        std::cerr << "错误: --bin 需要指定文件路径" << std::endl;
        return 1;
      }
    } else if (strcmp(argv[i], "--output") == 0 || strcmp(argv[i], "-o") == 0) {
      if (i + 1 < argc) {
        output_file = argv[++i];
      } else {
        std::cerr << "错误: --output 需要指定文件路径" << std::endl;
        return 1;
      }
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      std::cout << "用法: " << argv[0] << " [选项]" << std::endl;
      std::cout << "选项:" << std::endl;
      std::cout << "  --json, -j <file>     JSON配置文件路径 (默认: data/show.json)" << std::endl;
      std::cout << "  --bin, -b <file>      输入二进制文件路径，可多次指定以读取多个文件" << std::endl;
      std::cout << "                        (默认: data/test_unified.bin)" << std::endl;
      std::cout << "  --output, -o <file>    输出Perfetto文件路径 (默认: data/test.perfetto)" << std::endl;
      std::cout << "  --help, -h             显示此帮助信息" << std::endl;
      std::cout << std::endl;
      std::cout << "示例:" << std::endl;
      std::cout << "  " << argv[0] << " --bin file1.bin --bin file2.bin --bin file3.bin" << std::endl;
      return 0;
    }
  }
  
  PerfShower perf_shower;
  perf_shower.init();
  
  // 使用命令行参数指定的文件路径（支持多个文件）
  perf_shower.show(json_config, bin_inputs);
  perf_shower.finish(output_file);
  
  return 0;
}