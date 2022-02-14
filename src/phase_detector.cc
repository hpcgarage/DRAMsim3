// Copyright (c) 2021, Georgia Institute of Technology
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


// Ryan Thomas Lynch
// Georgia Institute of Technology
// ryan.lynch@gatech.edu
// CRNCH Lab
// based on pseudocode from https://doi.org/10.1145/3457388.3458670
// and Python code from Patrick Lavin available at:
// https://github.com/plavin/ModelSwapping/blob/master/PhaseDetector.py


// Phase detection code
// Version 0.9

#include "phase_detector.h"

using namespace std;

static std::map<uint64_t, uint64_t> address_to_first_seen_map;

static uint64_t clk = 0;

static uint64_t current_phase_latency = 0;
static uint64_t current_phase_num_transactions = 0;

static int old_phase = -2;
static uint64_t interval = 0;
static std::ofstream dram_phase_trace;

double PhaseDetector::difference_measure_of_signatures(bitvec sig1, bitvec sig2) {
    // auto xor_signatures = sig1 ^ sig2;
    // auto or_signatures = sig1 | sig2;
    return static_cast<double>((sig1 ^ sig2).count()) / (sig1 | sig2).count(); // this should work with any compiler
    // return ((double) xor_signatures.__builtin_count()) / or_signatures.__builtin_count(); // this might only work with GCC
}

//hashes the address of the current phase detector
uint64_t PhaseDetector::hash_address(uint64_t address) {
    // auto address_minus_bottom_drop_bits = address >> drop_bits;
    // uint32_t hashed_randomized_address = hash_bitvec(address_minus_bottom_drop_bits); // minstd_rand(address_minus_bottom_drop_bits)(); //hash_bitvec(address_minus_bottom_drop_bits);
    
    //drop the bottom {drop_bits} bits of the signature
    //hash it then return the top {log2_signature_len} bits of the hash (the number of bits determined by the length of the signature)
    //use this to then index into a bitvec that represents the current signature to set a specific bit to 1
    return ((uint32_t) std::hash<bitset<64>>{}(address >> phase_detector_constants::drop_bits)) >> (32 /*sizeof(uint32_t)*/ /* likely 32 if 32-bit MT or 64 if 64-bit MT or other hash */ - phase_detector_constants::log2_signature_len);
    // return hashed_randomized_address >> (32 /*sizeof(uint32_t)*/ /* likely 32 if 32-bit MT or 64 if 64-bit MT or other hash */ - log2_signature_len);
    
    //old: hash<bitset<1024>>()(address_minus_bottom_drop_bits) - time test on big XS: 18.539s
    //alt: hash<bitset<64>>()(address_minus_bottom_drop_bits) - time test on big XS: 7.962s
    //not really a hash: hash<uint64_t>()(address_minus_bottom_drop_bits) - time test on big XS: 6.178s
}

void PhaseDetector::detect(uint64_t instruction_pointer) {
    current_signature[hash_address(instruction_pointer)] = 1;
    

    if (instruction_count % phase_detector_constants::interval_len == 0) {
        // we are on a boundary! determine phase and notify listeners

        //first, check if the phase is stable since the difference measure is acceptably low
        if (difference_measure_of_signatures(current_signature, last_signature) < phase_detector_constants::threshold) {
            stable_count += 1;
            if (stable_count >= stable_min && phase == -1) {
                //add the current signature to the phase table and make the phase #/phase id to its index
                phase_table.push_back(current_signature);
                phase = phase_table.size() - 1; // or indexof curr_sig?
                //line 194 in the python
            }
        } else { //line 196 in python
            //if difference too high then it's not stable and we might now know the phase
            stable_count = 0;
            phase = -1;

            //see if we've entered a phase we have seen before
            if (!phase_table.empty()) { //line 201 python
                double best_diff = phase_detector_constants::threshold; 
                for (auto phase_table_iterator = phase_table.begin(); phase_table_iterator != phase_table.end(); phase_table_iterator++) {
                    const auto s = *phase_table_iterator;
                    const auto diff = difference_measure_of_signatures(current_signature, s);
                    // difference_scores_from_phase_table.push_back(diff);
                    if (diff < phase_detector_constants::threshold && diff < best_diff) {
                        phase = std::distance(phase_table.begin(), phase_table_iterator);
                        best_diff = diff;
                        //set current phase to the phase of the one with the lowest difference from current (which is the index in the phase table)
                    }
                }
            }
        }
        //whether or not the phase is stable, we need to update last phase and whatnot
        last_signature = current_signature;
        current_signature.reset();
        
        //add the current phase ID to the phase trace - from line 209 in python
        phase_trace.push_back(phase);

        //notify listeners of the current phase ID 
        //from line 212 in python
        for (auto f : listeners) {
            f(phase);
        }


        //TODO: add addr info to phase, dwarf map?

    }
    instruction_count += 1; // should this be before or after the if?
}

void PhaseDetector::init_phase_detector() {
    current_signature.reset();
    last_signature.reset();
    // hash_bitvec
    instruction_count = 0;
    stable_count = 0;
    phase = -1;
    phase_table.clear();
    phase_trace.clear();
    listeners.clear();
}

void PhaseDetector::print_log_file(string log_file_name) {
    
    ofstream log(log_file_name);
    // for (auto p : phase_trace) {
    for (size_t index = 0; index < phase_trace.size(); index++) {
        // cout << p << endl;
        auto p = phase_trace[index];
        if (log.is_open()) {
            log << index * phase_detector_constants::interval_len << "," << p << '\n';
        }
    }
    log.close();
    // } else {
    //     ofstream log("phase_trace_medium.txt");
    //     for (auto p : phase_trace) {
    //         // cout << p << endl;
    //         if (log.is_open()) {
    //             log << p << endl;
    //         }
    //     }
    //     log.close();
    // }
}

void PhaseDetector::cleanup_phase_detector(string log_file_name = "") {

    if (log_file_name.size() > 0) {
        print_log_file(log_file_name);
    }
    init_phase_detector();
}

void PhaseDetector::register_listeners(listener_function f) {
    listeners.push_back(f);
}

void dram_phase_trace_listener(int new_phase) {
    if (new_phase != old_phase) {
        if (dram_phase_trace.good()) {

            //TODO: need to get the stats for the current phase here to put in the output file all together

            int64_t adjustment = 0;  // (new_phase == -1) ? 0 : detector.stable_min * -1; //if we want to back label intervals

            double phase_aat = current_phase_latency / static_cast<double>(current_phase_num_transactions);

            dram_phase_trace << (interval + adjustment) << "," << new_phase << "," << phase_aat << '\n';
        } else {
            std::cerr << "dram phase trace file not good anymore! on phase " << new_phase << " and clock # " << clk << std::endl;
        }
        old_phase = new_phase;
        // memSystem->ResetStats();
        current_phase_latency = 0;
        current_phase_num_transactions = 0;
        
        //TODO: determine if transactions that cross boundaries are a big deal - just gonna throw them away for now lol
        address_to_first_seen_map.clear();
        
    }
    interval += 1;
}

void dramsim_transaction_callback_listener(uint64_t addr) {
    if (address_to_first_seen_map.count(addr)) {
        uint64_t latency = clk - address_to_first_seen_map[addr];
        address_to_first_seen_map.erase(addr);
        current_phase_num_transactions++;
        current_phase_latency += latency;
    }
}

void run_phase_detector(OnlineCPU *cpu, string input_files) {
    PhaseDetector detector;
    detector.init_phase_detector();

    ifstream input_file_list_stream(input_files);
    std::string line;
    clk = 0;
    current_phase_latency = 0;
    current_phase_num_transactions = 0;

    auto callback = std::bind(dramsim_transaction_callback_listener, std::placeholders::_1);
    detector.register_listeners(dram_phase_trace_listener);
    cpu->RegisterCallbacks(callback, callback);
    // memSystem->RegisterCallbacks(callback, callback);

    while (std::getline(input_file_list_stream, line)) {
        //line is the name of the current file to read from
        ifstream in_stream;
        in_stream.open(line);
        binary_output_struct_t current;
        bool readNext = true;
        while (in_stream.good()) {
            if (readNext) {
                in_stream.read((char*)&current, sizeof(binary_output_struct_t));
                detector.detect(current.instruction_pointer);
                address_to_first_seen_map[current.virtual_address] = clk;
                readNext = false;
            }
            readNext = cpu->canSendTransaction(current.virtual_address, current.is_write);
            cpu->ClockTick();
            clk++;
            if (readNext) {
                cout << "READ NEXT!" << endl;
            } else {
                cout << "did not read next" << endl;
            }
            if (clk % 10000 == 0) {
                cout << clk << endl;
            }
        }
        in_stream.close();
    }

    input_file_list_stream.close();

    // cpu->PrintStats();

    delete cpu;

    dram_phase_trace.close();

}
