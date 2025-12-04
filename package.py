#!/usr/bin/env python3
"""
打包脚本：将整个项目打包成一个可执行文件
支持定义 json 配置、bin 输入文件、输出 ptrace 文件
"""

import os
import sys
import shutil
import subprocess
import argparse
import tempfile
from pathlib import Path


class ProjectPackager:
    def __init__(self, output_dir="package"):
        self.project_root = Path(__file__).parent.absolute()
        self.output_dir = Path(output_dir)
        self.build_dir = self.project_root / "build"
        
    def build_project(self):
        """构建C++项目"""
        print("=== 配置CMake ===")
        if not self.build_dir.exists():
            self.build_dir.mkdir()
        
        subprocess.check_call([
            "cmake", "-B", str(self.build_dir), "-S", str(self.project_root)
        ])
        
        print("=== 编译项目 ===")
        subprocess.check_call(["make", "-C", str(self.build_dir)])
        
        print("✓ 项目构建完成")
    
    def create_package(self):
        """创建打包目录"""
        if self.output_dir.exists():
            print(f"清理现有打包目录: {self.output_dir}")
            shutil.rmtree(self.output_dir)
        
        self.output_dir.mkdir()
        print(f"✓ 创建打包目录: {self.output_dir}")
    
    def copy_executables(self):
        """复制可执行文件"""
        exe_dir = self.output_dir / "bin"
        exe_dir.mkdir()
        
        executables = [
            "perf_shower_main",
            "unified_generator",
            "unified_processor"
        ]
        
        for exe in executables:
            src = self.build_dir / exe
            if src.exists():
                dst = exe_dir / exe
                shutil.copy2(src, dst)
                # 确保可执行权限
                os.chmod(dst, 0o755)
                print(f"✓ 复制可执行文件: {exe}")
            else:
                print(f"⚠ 警告: 未找到可执行文件 {exe}")
    
    def copy_libraries(self):
        """复制共享库"""
        lib_dir = self.output_dir / "lib"
        lib_dir.mkdir()
        
        libraries = [
            "libperfetto.so"
        ]
        
        for lib in libraries:
            src = self.build_dir / lib
            if src.exists():
                dst = lib_dir / lib
                shutil.copy2(src, dst)
                print(f"✓ 复制库文件: {lib}")
            else:
                print(f"⚠ 警告: 未找到库文件 {lib}")
    
    def copy_data_files(self):
        """复制数据文件（示例）"""
        data_dir = self.output_dir / "data"
        data_dir.mkdir()
        
        src_data_dir = self.project_root / "data"
        if src_data_dir.exists():
            for item in src_data_dir.iterdir():
                if item.is_file():
                    shutil.copy2(item, data_dir / item.name)
                    print(f"✓ 复制数据文件: {item.name}")
    
    def create_launcher_script(self):
        """创建启动脚本"""
        launcher_content = '''#!/usr/bin/env python3
"""
性能追踪工具启动脚本
用法:
    perf_tool.py --json <config.json> --bin <input.bin> --output <output.perfetto>
    perf_tool.py --json data/show.json --bin data/test_unified.bin --output output.perfetto
"""

import os
import sys
import argparse
import subprocess
from pathlib import Path

# 获取脚本所在目录
SCRIPT_DIR = Path(__file__).parent.absolute()
BIN_DIR = SCRIPT_DIR / "bin"
LIB_DIR = SCRIPT_DIR / "lib"
DATA_DIR = SCRIPT_DIR / "data"

def setup_environment():
    """设置运行环境"""
    # 设置库路径
    lib_path = str(LIB_DIR)
    if "LD_LIBRARY_PATH" in os.environ:
        os.environ["LD_LIBRARY_PATH"] = lib_path + ":" + os.environ["LD_LIBRARY_PATH"]
    else:
        os.environ["LD_LIBRARY_PATH"] = lib_path

def main():
    parser = argparse.ArgumentParser(
        description="性能追踪工具 - 将统一性能格式转换为Perfetto追踪格式",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s --json data/show.json --bin data/test_unified.bin --output output.perfetto
  %(prog)s -j config.json -b input.bin -o trace.perfetto
        """
    )
    
    parser.add_argument(
        "--json", "-j",
        required=True,
        help="JSON配置文件路径（show.json）"
    )
    
    parser.add_argument(
        "--bin", "-b",
        required=True,
        help="输入二进制文件路径（性能数据）"
    )
    
    parser.add_argument(
        "--output", "-o",
        required=True,
        help="输出Perfetto追踪文件路径（.perfetto）"
    )
    
    parser.add_argument(
        "--generator",
        action="store_true",
        help="运行unified_generator生成测试数据"
    )
    
    parser.add_argument(
        "--processor",
        action="store_true",
        help="运行unified_processor处理数据"
    )
    
    args = parser.parse_args()
    
    # 设置环境
    setup_environment()
    
    # 检查可执行文件
    perf_shower_exe = BIN_DIR / "perf_shower_main"
    if not perf_shower_exe.exists():
        print(f"错误: 未找到可执行文件 {perf_shower_exe}")
        sys.exit(1)
    
    # 检查输入文件
    json_path = Path(args.json)
    bin_path = Path(args.bin)
    
    if not json_path.exists():
        # 尝试在data目录中查找
        data_json = DATA_DIR / json_path.name
        if data_json.exists():
            json_path = data_json
        else:
            print(f"错误: JSON配置文件不存在: {args.json}")
            sys.exit(1)
    
    if not bin_path.exists():
        # 尝试在data目录中查找
        data_bin = DATA_DIR / bin_path.name
        if data_bin.exists():
            bin_path = data_bin
        else:
            print(f"错误: 二进制文件不存在: {args.bin}")
            sys.exit(1)
    
    # 运行generator（如果需要）
    if args.generator:
        generator_exe = BIN_DIR / "unified_generator"
        if generator_exe.exists():
            print("=== 运行Generator ===")
            subprocess.check_call([str(generator_exe)])
        else:
            print("⚠ 警告: unified_generator 不存在")
    
    # 运行processor（如果需要）
    if args.processor:
        processor_exe = BIN_DIR / "unified_processor"
        if processor_exe.exists():
            print("=== 运行Processor ===")
            subprocess.check_call([str(processor_exe)])
        else:
            print("⚠ 警告: unified_processor 不存在")
    
    # 运行perf_shower_main
    print(f"=== 运行PerfShower ===")
    print(f"JSON配置: {json_path}")
    print(f"输入文件: {bin_path}")
    print(f"输出文件: {args.output}")
    
    # 设置环境变量（用于库路径）
    env = os.environ.copy()
    
    try:
        # 直接传递命令行参数给可执行文件
        subprocess.check_call([
            str(perf_shower_exe),
            "--json", str(json_path),
            "--bin", str(bin_path),
            "--output", str(args.output)
        ], env=env)
        print(f"✓ 成功生成Perfetto追踪文件: {args.output}")
    except subprocess.CalledProcessError as e:
        print(f"错误: 执行失败，退出码 {e.returncode}")
        sys.exit(e.returncode)

if __name__ == "__main__":
    main()
'''
        
        launcher_path = self.output_dir / "perf_tool.py"
        with open(launcher_path, "w", encoding="utf-8") as f:
            f.write(launcher_content)
        
        # 设置可执行权限
        os.chmod(launcher_path, 0o755)
        print(f"✓ 创建启动脚本: perf_tool.py")
    
    def create_readme(self):
        """创建README文件"""
        readme_content = '''# 性能追踪工具打包版本

## 文件结构

```
package/
├── bin/              # 可执行文件
│   ├── perf_shower_main
│   ├── unified_generator
│   └── unified_processor
├── lib/              # 共享库
│   └── libperfetto.so
├── data/             # 示例数据文件
│   ├── show.json
│   └── *.bin
├── perf_tool.py      # 启动脚本
└── README.md         # 本文件
```

## 使用方法

### 基本用法

```bash
./perf_tool.py --json <config.json> --bin <input.bin> --output <output.perfetto>
```

### 参数说明

- `--json, -j`: JSON配置文件路径（show.json格式）
- `--bin, -b`: 输入二进制文件路径（性能数据）
- `--output, -o`: 输出Perfetto追踪文件路径（.perfetto格式）

### 示例

```bash
# 使用示例数据
./perf_tool.py --json data/show.json --bin data/test_unified.bin --output trace.perfetto

# 使用自定义文件
./perf_tool.py -j my_config.json -b my_data.bin -o my_trace.perfetto

# 先运行generator生成数据，再处理
./perf_tool.py --generator --json data/show.json --bin data/test_unified.bin --output trace.perfetto
```

## 依赖说明

- 需要系统安装protobuf库
- 需要libperfetto.so在LD_LIBRARY_PATH中（脚本会自动设置）

## 注意事项

1. 确保输入文件路径正确
2. 输出目录需要有写权限
3. 如果使用相对路径，脚本会在data目录中查找文件
'''
        
        readme_path = self.output_dir / "README.md"
        with open(readme_path, "w", encoding="utf-8") as f:
            f.write(readme_content)
        print(f"✓ 创建README文件")
    
    def package(self):
        """执行完整的打包流程"""
        print("=" * 60)
        print("开始打包项目")
        print("=" * 60)
        
        try:
            # 1. 构建项目
            self.build_project()
            
            # 2. 创建打包目录
            self.create_package()
            
            # 3. 复制文件
            self.copy_executables()
            self.copy_libraries()
            self.copy_data_files()
            
            # 4. 创建启动脚本和文档
            self.create_launcher_script()
            self.create_readme()
            
            print("=" * 60)
            print(f"✓ 打包完成！输出目录: {self.output_dir.absolute()}")
            print("=" * 60)
            print("\n使用方法:")
            print(f"  cd {self.output_dir}")
            print("  ./perf_tool.py --json data/show.json --bin data/test_unified.bin --output trace.perfetto")
            print("=" * 60)
            
        except subprocess.CalledProcessError as e:
            print(f"错误: 构建失败，退出码 {e.returncode}")
            sys.exit(1)
        except Exception as e:
            print(f"错误: {e}")
            sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description="打包性能追踪工具项目",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument(
        "--output", "-o",
        default="package",
        help="输出打包目录（默认: package）"
    )
    
    args = parser.parse_args()
    
    packager = ProjectPackager(output_dir=args.output)
    packager.package()


if __name__ == "__main__":
    main()

