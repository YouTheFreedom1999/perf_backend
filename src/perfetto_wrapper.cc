#include "perfetto_wrapper.hh"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

PerfettoWrapper::PerfettoWrapper()
    : track_cnt_(0), flow_range_id_(0),
      system_track_(perfetto::Track::Global(1)) {}

PerfettoWrapper::~PerfettoWrapper() {
  if (tracing_session_) {
    perfetto::TrackEvent::Flush();
  }
}

void PerfettoWrapper::start(int buf_size_kb) {
  perfetto::TracingInitArgs args;
  args.backends = perfetto::kInProcessBackend;
  perfetto::Tracing::Initialize(args);
  perfetto::TrackEvent::Register();

  perfetto::TraceConfig cfg;
  auto buf_cfg = cfg.add_buffers();
  buf_cfg->set_size_kb(buf_size_kb);
  // buf_cfg->set_fill_policy(perfetto::protos::gen::TraceConfig_BufferConfig::RING_BUFFER);

  auto *ds_cfg = cfg.add_data_sources()->mutable_config();
  ds_cfg->set_name("track_event");

  tracing_session_ = perfetto::Tracing::NewTrace();
  tracing_session_->Setup(cfg);
  tracing_session_->StartBlocking();

  auto system_desc = system_track_.Serialize();
  system_desc.set_name("xpu_profiler");
  perfetto::TrackEvent::SetTrackDescriptor(system_track_, system_desc);
}

void PerfettoWrapper::end(const std::string &perf_path) {
  if (!tracing_session_) {
    std::cerr << "错误：追踪会话未初始化，请先调用 start()" << std::endl;
    return;
  }

  perfetto::TrackEvent::Flush();
  std::vector<char> trace_data(tracing_session_->ReadTraceBlocking());

  std::ofstream output(perf_path, std::ios::out | std::ios::binary);
  if (!output.is_open()) {
    std::cerr << "错误：无法打开文件 " << perf_path << std::endl;
    return;
  }

  output.write(&trace_data[0], std::streamsize(trace_data.size()));
  output.close();

  printf("Trace文件已保存为: %s\n", perf_path.c_str());
}

std::shared_ptr<perfetto::NamedTrack> PerfettoWrapper::createNamedTrack(
    const std::string &track_name, const std::string &track_show_name,
    perfetto::Track &parent_track, uint64_t rank_id, bool set_cnt) {

  auto track = std::make_shared<perfetto::NamedTrack>(
      perfetto::DynamicString(track_name), track_cnt_++, parent_track);

  auto desc = track->Serialize();
  if (set_cnt) {
    desc.set_name(
        ("[" + std::to_string(track_cnt_) + "]" + track_show_name).c_str());
  } else {
    desc.set_name(track_show_name.c_str());
  }
  desc.set_child_ordering(perfetto::protos::gen::TrackDescriptor::EXPLICIT);
  desc.set_sibling_order_rank(rank_id);

  perfetto::TrackEvent::SetTrackDescriptor(*track, desc);
  return track;
}

std::shared_ptr<perfetto::CounterTrack>
PerfettoWrapper::createCounterTrack(const std::string &name,
                                    const std::string &unit_name,
                                    perfetto::Track &parent_track) {

  auto track = std::make_shared<perfetto::CounterTrack>(
      perfetto::DynamicString(name), parent_track);
  // track->set_unit(perfetto::CounterTrack::Unit::UNIT_SIZE_BYTES);
  // track->set_unit_multiplier(1024);
  track->set_unit_name(unit_name.c_str());
  return track;
}

void PerfettoWrapper::addCounterEvent(perfetto::CounterTrack &track,
                                      uint64_t cycle, double value) {
  TRACE_COUNTER("cpu.common", track, cycle, value);
}

void PerfettoWrapper::addTraceEvent(const std::string &title_name,
                                    perfetto::NamedTrack &track,
                                    uint64_t start_cycle, uint64_t end_cycle,
                                    const std::string &msg) {

  TRACE_EVENT_BEGIN("cpu.common", perfetto::DynamicString(title_name), track,
                    start_cycle, "msg", perfetto::DynamicString(msg));
  TRACE_EVENT_END("cpu.common", track, end_cycle);
  std::cout << "addTraceEvent: " << title_name << " " << start_cycle << " "
            << end_cycle << " " << msg << std::endl;
}

void PerfettoWrapper::addTraceEventWithFlow(const std::string &title_name,
                                            perfetto::NamedTrack &track,
                                            uint64_t start_cycle,
                                            uint64_t end_cycle,
                                            uint64_t flow_id,
                                            const std::string &msg) {

  TRACE_EVENT_BEGIN("cpu.common", perfetto::DynamicString(title_name), track,
                    start_cycle,
                    perfetto::Flow::Global(flow_id + (flow_range_id_ << 32)),
                    "msg", perfetto::DynamicString(msg));
  TRACE_EVENT_END("cpu.common", track, end_cycle);
  flow_range_id_++;
}
