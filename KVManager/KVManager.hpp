#pragma once
#include <string>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include "../DataTypes/PageData.hpp"
#include "../Buffer/BufferManager.hpp"
#include "../Policy/FIFOEvictionPolicy.hpp"
#include "../Policy/SimpleLinuxIoPolicy.hpp"


class KVManager {
public:
    KVManager() : bm (100) {
        for (std::atomic<uint64_t>& at : mCurrGenPageNo) {
            at = 0 ;
        }
    }
    std::string get(const std::string& key);

    bool put(const std::string& key, const std::string& value);

    bool remove(const std::string &key);
private:
    bool entryLookup(const uint64_t hVal, Entry_t& e);
    Entry_t genNewEntry(const uint64_t seed);

    std::unordered_map<uint64_t, Entry_t> entryMap;
    std::mutex entryMapMutex;

    std::array<std::atomic<uint64_t>, 8> mCurrGenPageNo;
    BufferManager<FIFOEvictionPolicy, SimpleLinuxIoPolicy> bm;

    uint16_t crawlToKey(Segment *&segment, const std::string &key, const bool exclusive);

    void readString(Segment *segment, uint16_t recordId, std::stringstream &ss);
};

