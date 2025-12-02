# perf_backend

1. 自定义的统一性能输出格式，可以对pythonModel、Gem5、systemC、真实芯片的trace流，进行可视化显示和数据分析
2. 可视化工具有perfetto或者本人开发的统一可视化工具（对表Nvdia的Nsight性能分析工具）
3. 底层使用perfetto和 protobuf 实现
4. 支持多设备、多线程、多阶段、多计数器、多函数、多指令的性能分析
5. 支持定义配置指令和执行指令的隶属关系，通过可视化连线方式显示
6. 支持可以配置的显示任意线程数据，独立或共同显示不同线程数据
7. 包含对 CPU VPU TPU TMA DM VHM 的各种可视化流水和性能指标显示


# 可视化分析视图
1. line mode 
2. pipe mode
4. func mode
5. scheduler mode
6. util mode 
7. 性能分析 直方图 显示 ，支持嵌入python脚本的实时显示 （jupyter notebook mode）


