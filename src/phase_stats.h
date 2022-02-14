#ifndef __PHASE_STATS_
#define __PHASE_STATS_

#include <string>
#include <unordered_map>
#include <vector>

namespace dramsim3 {

class PhaseStats {
    public:
        void Increment(std::string name, int64_t phase_id);
        void AddValue(std::string name, uint64_t value, int64_t phase_id);

    private:
        class StatsData {
            public:
                std::unordered_map<std::string, std::vector<uint64_t>> value_stats_map_;
                std::unordered_map<std::string, uint64_t> counter_stats_map_;

                void Increment(std::string name);
                void AddValue(std::string name, uint64_t value);
        };

        std::unordered_map<int64_t, StatsData> stats_map_;

};
} // namespace : dramsim3
#endif