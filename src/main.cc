#include "perf_shower.hh"
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
  // 默认值
  const char* json_config = "data/show.json";
  std::string log_file_path;

  // 解析命令行参数
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--json") == 0 || strcmp(argv[i], "-j") == 0) {
      if (i + 1 < argc) {
        json_config = argv[++i];
      } else {
        std::cerr << "错误: --json 需要指定文件路径" << std::endl;
        return 1;
      }
    } else if (strcmp(argv[i], "--log") == 0) {
      if (i + 1 < argc) {
        log_file_path = argv[++i];
      } else {
        std::cerr << "错误: --log 需要指定文件路径" << std::endl;
        return 1;
      }
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      std::cout << "用法: " << argv[0] << " [选项]" << std::endl;
      std::cout << "选项:" << std::endl;
      std::cout << "  --json, -j <file>     JSON配置文件路径 (默认: data/show.json)" << std::endl;
      std::cout << "                        JSON 中必须包含 'filelist' 和 'output' 字段" << std::endl;
      std::cout << "  --log <file>          指定日志输出文件 (默认: 打印到控制台)" << std::endl;
      std::cout << "  --help, -h             显示此帮助信息" << std::endl;
      std::cout << std::endl;
      std::cout << "示例:" << std::endl;
      std::cout << "  " << argv[0] << " --json config.json --log run.log" << std::endl;
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
  
  // 如果指定了日志文件，重定向 std::cout 和 std::cerr
  std::ofstream log_stream;
  std::streambuf* cout_buf = nullptr;
  std::streambuf* cerr_buf = nullptr;

  if (!log_file_path.empty()) {
    log_stream.open(log_file_path);
    if (!log_stream.is_open()) {
      std::cerr << "错误: 无法打开日志文件 " << log_file_path << std::endl;
      return 1;
    }
    cout_buf = std::cout.rdbuf();
    cerr_buf = std::cerr.rdbuf();
    std::cout.rdbuf(log_stream.rdbuf());
     std::cerr.rdbuf(log_stream.rdbuf());
   }
 
   int ret = 0;
   {
     PerfShower perf_shower;
     perf_shower.init();
     
     std::string output_file = perf_shower.show(json_config);
     
     if (output_file.empty()) {
       std::cerr << "错误：未能从 JSON 配置中获取输出文件路径" << std::endl;
       ret = 1;
     } else {
       perf_shower.finish(output_file);
     }
   }
   
   // 恢复 buffer
   if (!log_file_path.empty()) {
     std::cout.rdbuf(cout_buf);
     std::cerr.rdbuf(cerr_buf);
   }
 
   return ret;
 }