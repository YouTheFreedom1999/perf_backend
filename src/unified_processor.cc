#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "unified_perf_format.pb.h"
#include "perfetto.h" // Include Perfetto header directly

using namespace unified_perf_format;

void ReadInstructions() {
    std::cout << "=== Reading Instructions (test_instructions.bin) ===" << std::endl;
    BatchInstruction batch;
    std::ifstream input("data/test_instructions.bin", std::ios::in | std::ios::binary);
    if (!input) {
        std::cerr << "data/test_instructions.bin not found." << std::endl;
        return;
    }
    if (!batch.ParseFromIstream(&input)) {
        std::cerr << "Failed to parse instructions." << std::endl;
        return;
    }

    for (int i = 0; i < batch.instructions_size(); ++i) {
        const Instruction& inst = batch.instructions(i);
        std::cout << "Instruction #" << i << ":" << std::endl;
        std::cout << "  Name: " << inst.name() << std::endl;
        std::cout << "  GlobalSeq: " << inst.global_seq_num() << std::endl;
        std::cout << "  Device: " << inst.device_name() << std::endl;
        if (inst.metadata().count("kernel_size")) {
            std::cout << "  Metadata[kernel_size]: " << inst.metadata().at("kernel_size") << std::endl;
        }
        for (int j = 0; j < inst.stages_size(); ++j) {
             const Stage& stage = inst.stages(j);
             std::cout << "    Stage: " << stage.name() 
                       << " [" << stage.start_time() << " - " << stage.end_time() << "]" << std::endl;
        }
    }
}

void ReadFunctions() {
    std::cout << "\n=== Reading Functions (test_functions.bin) ===" << std::endl;
    BatchFunction batch;
    std::ifstream input("data/test_functions.bin", std::ios::in | std::ios::binary);
    if (!input) {
        std::cerr << "data/test_functions.bin not found." << std::endl;
        return;
    }
    if (!batch.ParseFromIstream(&input)) {
        std::cerr << "Failed to parse functions." << std::endl;
        return;
    }

    for (int i = 0; i < batch.functions_size(); ++i) {
        const Function& func = batch.functions(i);
        std::cout << "Function #" << i << ":" << std::endl;
        std::cout << "  Title: " << func.title() << std::endl;
        std::cout << "  PC: " << std::hex << func.pc() << std::dec << std::endl;
        std::cout << "  Type: " << func.insttype() << std::endl;
    }
}

void ReadCounters() {
    std::cout << "\n=== Reading Counters (test_counters.bin) ===" << std::endl;
    BatchCounter batch;
    std::ifstream input("data/test_counters.bin", std::ios::in | std::ios::binary);
    if (!input) {
        std::cerr << "data/test_counters.bin not found." << std::endl;
        return;
    }
    if (!batch.ParseFromIstream(&input)) {
        std::cerr << "Failed to parse counters." << std::endl;
        return;
    }

    for (int i = 0; i < batch.counters_size(); ++i) {
        const Counter& cnt = batch.counters(i);
        std::cout << "Counter #" << i << ":" << std::endl;
        std::cout << "  Name: " << cnt.name() << std::endl;
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
