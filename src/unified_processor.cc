#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "unified_perf_format.pb.h"
#include "perfetto.h" // Include Perfetto header directly

using namespace unified_perf_format;

void ReadInstructions() {
    std::cout << "=== Reading Instructions (test_instructions.bin) ===" << std::endl;
    UnifiedPerfData perf_data;
    std::ifstream input("data/test_instructions.bin", std::ios::in | std::ios::binary);
    if (!input) {
        std::cerr << "data/test_instructions.bin not found." << std::endl;
        return;
    }
    if (!perf_data.ParseFromIstream(&input)) {
        std::cerr << "Failed to parse instructions." << std::endl;
        return;
    }

    if (perf_data.data_type() != UnifiedPerfData::INSTRUCTIONS || !perf_data.has_instructions()) {
        std::cerr << "Error: File does not contain INSTRUCTIONS data." << std::endl;
        return;
    }

    const BatchInstruction& batch = perf_data.instructions();
    std::cout << "Device: " << perf_data.device_name() << std::endl;
    for (int i = 0; i < batch.instructions_size(); ++i) {
        const Instruction& inst = batch.instructions(i);
        std::cout << "Instruction #" << i << ":" << std::endl;
        std::cout << "  Name: " << inst.name() << std::endl;
        std::cout << "  Title: " << inst.title() << std::endl;
        std::cout << "  GlobalSeq: " << inst.global_seq_num() << std::endl;
        std::cout << "  Device: " << inst.device_name() << std::endl;
        std::cout << "  ThreadID: " << inst.thread_id() << std::endl;
        if (inst.parent_seq_num_size() > 0) {
            std::cout << "  ParentSeqNums: ";
            for (auto seq : inst.parent_seq_num()) {
                std::cout << seq << " ";
            }
            std::cout << std::endl;
        }
        if (inst.metadata().count("kernel_size")) {
            std::cout << "  Metadata[kernel_size]: " << inst.metadata().at("kernel_size") << std::endl;
        }
        for (int j = 0; j < inst.stages_size(); ++j) {
             const Stage& stage = inst.stages(j);
             std::cout << "    Stage[" << stage.order_id() << "]: " << stage.name() 
                       << " [" << stage.start_time() << " - " << stage.end_time() << "]" << std::endl;
        }
    }
}

void ReadFunctions() {
    std::cout << "\n=== Reading Functions (test_functions.bin) ===" << std::endl;
    UnifiedPerfData perf_data;
    std::ifstream input("data/test_functions.bin", std::ios::in | std::ios::binary);
    if (!input) {
        std::cerr << "data/test_functions.bin not found." << std::endl;
        return;
    }
    if (!perf_data.ParseFromIstream(&input)) {
        std::cerr << "Failed to parse functions." << std::endl;
        return;
    }

    if (perf_data.data_type() != UnifiedPerfData::FUNCTIONS || !perf_data.has_functions()) {
        std::cerr << "Error: File does not contain FUNCTIONS data." << std::endl;
        return;
    }

    const BatchFunction& batch = perf_data.functions();
    std::cout << "Device: " << perf_data.device_name() << std::endl;
    for (int i = 0; i < batch.functions_size(); ++i) {
        const Function& func = batch.functions(i);
        std::cout << "Function #" << i << ":" << std::endl;
        std::cout << "  Title: " << func.title() << std::endl;
        std::cout << "  Device: " << func.device_name() << std::endl;
        std::cout << "  ThreadID: " << func.thread_id() << std::endl;
        std::cout << "  GlobalSeq: " << func.global_seq_num() << std::endl;
        std::cout << "  PC: " << std::hex << func.pc() << std::dec << std::endl;
        std::cout << "  Type: " << static_cast<int>(func.inst_type()) << std::endl;
    }
}

void ReadCounters() {
    std::cout << "\n=== Reading Counters (test_counters.bin) ===" << std::endl;
    UnifiedPerfData perf_data;
    std::ifstream input("data/test_counters.bin", std::ios::in | std::ios::binary);
    if (!input) {
        std::cerr << "data/test_counters.bin not found." << std::endl;
        return;
    }
    if (!perf_data.ParseFromIstream(&input)) {
        std::cerr << "Failed to parse counters." << std::endl;
        return;
    }

    if (perf_data.data_type() != UnifiedPerfData::COUNTERS || !perf_data.has_counters()) {
        std::cerr << "Error: File does not contain COUNTERS data." << std::endl;
        return;
    }

    const BatchCounter& batch = perf_data.counters();
    std::cout << "Device: " << perf_data.device_name() << std::endl;
    for (int i = 0; i < batch.counters_size(); ++i) {
        const Counter& cnt = batch.counters(i);
        std::cout << "Counter #" << i << ":" << std::endl;
        std::cout << "  Name: " << cnt.name() << std::endl;
        std::cout << "  Title: " << cnt.title() << std::endl;
        std::cout << "  Unit: " << cnt.unit() << std::endl;
        std::cout << "  Values: ";
        for (auto val : cnt.values()) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
}

void InitializePerfetto() {
    std::cout << "\n=== Initializing Perfetto (Shared Library Link) ===" << std::endl;
    perfetto::TracingInitArgs args;
    args.backends = perfetto::kSystemBackend;
    perfetto::Tracing::Initialize(args);
    std::cout << "Perfetto Initialized via C++ API." << std::endl;
}

int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    ReadInstructions();
    ReadFunctions();
    ReadCounters();

    InitializePerfetto();

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
