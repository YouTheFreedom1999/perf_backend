#!/usr/bin/env python3
import os
import sys
import subprocess

def main():
    build_dir = "build"
    data_dir = "data"
    
    # Create data directory if it doesn't exist
    if not os.path.exists(data_dir):
        os.makedirs(data_dir)

    print("=== Configuring ===")
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    
    subprocess.check_call(["cmake", "-B", build_dir, "-S", "."])
    
    print("=== Building ===")
    subprocess.check_call(["make", "-C", build_dir])

    print("=== Running Generator ===")
    subprocess.check_call([os.path.join(build_dir, "unified_generator")])
    
    print("=== Running Processor ===")
    subprocess.check_call([os.path.join(build_dir, "unified_processor")])
    
    print("=== Running PerfShower Main ===")
    subprocess.check_call([os.path.join(build_dir, "perf_shower_main")])

if __name__ == "__main__":
    main()
