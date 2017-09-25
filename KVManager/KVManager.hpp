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
    Entry_t entryLookup(uint64_t hVal);
    Entry_t genNewEntry(uint64_t seed);

    std::unordered_map<uint64_t, Entry_t> entryMap;
    std::mutex entryMapMutex;

    std::array<std::atomic<uint64_t>, 8> mCurrGenPageNo;
    BufferManager<FIFOEvictionPolicy, SimpleLinuxIoPolicy> bm;
};

