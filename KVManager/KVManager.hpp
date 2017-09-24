#pragma once
#include <string>
#include <unordered_map>
#include <atomic>
#include <mutex>

enum EntryFlags : uint16_t {
    DEFAULT = 0,
    NEW = 1 << 1,
    DIRTY = 1 << 2,
    INVALID = 1 << 15
};

struct Entry_t {
    std::uint16_t segment;
    std::uint64_t startPage;
    std::uint16_t flags;
};

struct PageHeader {
    uint64_t nextPid;
    int16_t numEntries;
    uint16_t startOffset;
};

const uint64_t PAGE_SIZE = 4 * 1024;


class KVManager {
public:
    KVManager() {
        for (std::atomic<uint64_t>& at : mCurrGenPageNo) {
            at = 0 ;
        }
    }
    std::string get(std::string key);

    bool put(std::string key, std::string value);

    bool remove(std::string key);
private:

    Entry_t entryLookup(uint64_t hVal);

    Entry_t genNewEntry(uint64_t seed);

    std::unordered_map<uint64_t, Entry_t> entryMap;
    std::mutex entryMapMutex;

    std::array<std::atomic<uint64_t>, 8> mCurrGenPageNo;
};

