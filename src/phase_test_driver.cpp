// # Copyright (c) 2021, Georgia Institute of Technology
// #
// # SPDX-License-Identifier: Apache-2.0
// #
// # Licensed under the Apache License, Version 2.0 (the "License");
// # you may not use this file except in compliance with the License.
// # You may obtain a copy of the License at
// #
// #     http://www.apache.org/licenses/LICENSE-2.0
// #
// # Unless required by applicable law or agreed to in writing, software
// # distributed under the License is distributed on an "AS IS" BASIS,
// # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// # See the License for the specific language governing permissions and
// # limitations under the License.

// Ryan Thomas Lynch
// Georgia Institute of Technology
// ryan.lynch@gatech.edu
// CRNCH Lab

#include <iostream>
#include "./../ext/headers/args.hxx"
#include "cpu.h"


using namespace dramsim3;

int main(int argc, const char **argv) {
    args::ArgumentParser parser(
        "DRAM Simulator.",
        "Examples: \n."
        "./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini -c 100 -t "
        "sample_trace.txt\n"
        "./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini -s random -c 100");
    args::HelpFlag help(parser, "help", "Display the help menu", {'h', "help"});
    args::ValueFlag<uint64_t> num_cycles_arg(parser, "num_cycles",
                                             "Number of cycles to simulate",
                                             {'c', "cycles"}, 100000);
    args::ValueFlag<std::string> output_dir_arg(
        parser, "output_dir", "Output directory for stats files",
        {'o', "output-dir"}, ".");
    args::ValueFlag<std::string> stream_arg(
        parser, "stream_type", "address stream generator - (random), stream",
        {'s', "stream"}, "");
    args::ValueFlag<std::string> trace_file_arg(
        parser, "trace",
        "Trace file, setting this option will ignore -s option",
        {'t', "trace"});
    args::Positional<std::string> config_arg(
        parser, "config", "The config file name (mandatory)");

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    std::string config_file = args::get(config_arg);
    if (config_file.empty()) {
        std::cerr << parser;
        return 1;
    }

    uint64_t cycles = args::get(num_cycles_arg);
    std::string output_dir = args::get(output_dir_arg);
    std::string trace_file = args::get(trace_file_arg);
    std::string stream_type = args::get(stream_arg);

    CPU *cpu;
    if (!trace_file.empty()) {
        cpu = new TraceBasedCPU(config_file, output_dir, trace_file);
    } else {
        if (stream_type == "stream" || stream_type == "s") {
            cpu = new StreamCPU(config_file, output_dir);
        } else {
            cpu = new RandomCPU(config_file, output_dir);
        }
    }

    for (uint64_t clk = 0; clk < cycles; clk++) {
        cpu->ClockTick();
    }
    cpu->PrintStats();

    delete cpu;

    return 0;
}
