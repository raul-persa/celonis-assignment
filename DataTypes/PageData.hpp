#pragma once

#include <cstdint>
#include <boost/functional/hash.hpp>
#include <boost/thread/pthread/shared_mutex.hpp>

const uint64_t PAGE_SIZE = 4 * 1024;
static constexpr char  const * NULL_PAGE = nullptr;

enum EntryFlags : uint16_t {
    DEFAULT = 0,
    NEW = 1 << 1,
    DIRTY = 1 << 2,
    INVALID = 1 << 15
};

struct Entry_t {
    Entry_t()
            : segment(0), startPage(0), flags(INVALID) {

    }

    Entry_t(const Entry_t &other)
            : segment(other.segment), startPage(other.startPage), flags(other.flags) {
    }

    Entry_t &operator=(const Entry_t &other) {

        segment = other.segment;
        startPage = other.startPage;
        flags = other.flags;
    }

    Entry_t(Entry_t &&other)
            : segment(other.segment), startPage(other.startPage), flags(other.flags) {
        other.invalidate();
    }


    Entry_t &operator==(Entry_t &&other) {
        segment = other.segment;
        startPage = other.startPage;
        flags = other.flags;
        other.invalidate();
    }

    bool operator==(const Entry_t &other) const {
        return segment == other.segment && startPage == other.startPage && flags == other.flags;
    }

    bool operator!=(const Entry_t &other) const {
        return segment != other.segment && startPage != other.startPage && flags != other.flags;
    }


    uint16_t segment;
    uint64_t startPage;
    uint16_t flags;

private:
    void invalidate() {
        segment = 0;
        startPage = 0;
        flags = INVALID;
    }
};

struct EntryHasher {
    std::size_t operator()(const Entry_t &e) const {
        std::size_t seed = 0;
        boost::hash_combine(seed, e.segment);
        boost::hash_combine(seed, e.startPage);
        boost::hash_combine(seed, e.flags);
        return seed;
    }
};

namespace std {
    template<>
    struct hash<Entry_t> {
        std::size_t operator()(const Entry_t &e) const {
            EntryHasher h;
            return h(e);
        }
    };
}

struct Segment {

    Segment()
            : entry(), mutex() {

    }

    Entry_t entry;
    boost::shared_mutex mutex;
    void *data;
};

struct PageHeader {
    uint64_t nextPid;
    int16_t numEntries;
    uint16_t startOffset;
};
