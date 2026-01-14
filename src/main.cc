#include "perf_shower.hh"
#include <iostream>
#include <cstring>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
  // 默认值
  const char* json_config = "data/show.json";
  
  // 解析命令行参数
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--json") == 0 || strcmp(argv[i], "-j") == 0) {
      if (i + 1 < argc) {
        json_config = argv[++i];
      } else {
        std::cerr << "错误: --json 需要指定文件路径" << std::endl;
        return 1;
      }
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      std::cout << "用法: " << argv[0] << " [选项]" << std::endl;
      std::cout << "选项:" << std::endl;
      std::cout << "  --json, -j <file>     JSON配置文件路径 (默认: data/show.json)" << std::endl;
      std::cout << "                        JSON 中必须包含 'filelist' 和 'output' 字段" << std::endl;
      std::cout << "  --help, -h             显示此帮助信息" << std::endl;
      std::cout << std::endl;
      std::cout << "示例:" << std::endl;
      std::cout << "  " << argv[0] << " --json config.json" << std::endl;
      std::cout << std::endl;
      std::cout << "JSON 配置文件格式示例:" << std::endl;
      std::cout << "  {" << std::endl;
      std::cout << "    \"filelist\": [\"file1.bin\", \"file2.bin\"]," << std::endl;
      std::cout << "    \"output\": \"output.perfetto\"," << std::endl;
      std::cout << "    \"view0\": { \"mode\": \"pipe\" }" << std::endl;
      std::cout << "  }" << std::endl;
      return 0;
    }
  }
  
  PerfShower perf_shower;
  perf_shower.init();
  
  std::string output_file = perf_shower.show(json_config);
  
  if (output_file.empty()) {
    std::cerr << "错误：未能从 JSON 配置中获取输出文件路径" << std::endl;
    return 1;
  }
  
  perf_shower.finish(output_file);
  
  return 0;
}