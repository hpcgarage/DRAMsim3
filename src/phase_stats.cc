#include <iostream>

#include "phase_stats.h"

namespace dramsim3 {
    void PhaseStats::AddValue(std::string name, std::uint64_t value, int64_t phase_id) {
        if (stats_map_.find(phase_id) != stats_map_.end() ) {
            auto stats_phase = stats_map_.find(phase_id);
            stats_phase->second.AddValue(name, value);
        } else {
            auto stats_phase = StatsData();
            stats_phase.AddValue(name, value);
            stats_map_.insert({phase_id, stats_phase});
        }
    }

    void PhaseStats::Increment(std::string name, int64_t phase_id) {
        if (stats_map_.find(phase_id) != stats_map_.end() ) {
            auto stats_phase = stats_map_.find(phase_id);
            stats_phase->second.Increment(name);
        } else {
            auto stats_phase = StatsData();
            stats_phase.Increment(name);
            stats_map_.insert({phase_id, stats_phase});
        }
    }

    void PhaseStats::StatsData::Increment(std::string name) {
        if (counter_stats_map_.find(name) != counter_stats_map_.end() ) {
            auto counter_pair = counter_stats_map_.find(name);
            counter_pair->second += 1;
        } else {
            auto counter_pair = 1;
            counter_stats_map_.insert({name, counter_pair});
        }
    }

    void PhaseStats::StatsData::AddValue(std::string name, uint64_t value) {
        if (value_stats_map_.find(name) != value_stats_map_.end() ) {
            auto values_vector = value_stats_map_.find(name);
            values_vector->second.push_back(value);
        } else {
            std::vector<uint64_t> values_vector;
            values_vector.push_back(value);
            value_stats_map_.insert({name, values_vector});
        }
    }

} // namespace : dramsim3