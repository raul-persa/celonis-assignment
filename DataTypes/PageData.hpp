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
            : fileSegment(0), pageId(0) {

    }

    Entry_t(const Entry_t &other)
            : fileSegment(other.fileSegment), pageId(other.pageId) {
    }

    Entry_t &operator=(const Entry_t &other) {

        fileSegment = other.fileSegment;
        pageId = other.pageId;
    }

    Entry_t(Entry_t &&other)
            : fileSegment(other.fileSegment), pageId(other.pageId) {
        other.invalidate();
    }


    Entry_t &operator=(Entry_t &&other) {
        fileSegment = other.fileSegment;
        pageId = other.pageId;
        other.invalidate();
    }

    bool operator==(const Entry_t &other) const {
        return fileSegment == other.fileSegment && pageId == other.pageId;
    }

    bool operator!=(const Entry_t &other) const {
        return fileSegment != other.fileSegment && pageId != other.pageId;
    }


    uint16_t fileSegment;
    uint64_t pageId;

private:
    void invalidate() {
        fileSegment = 0;
        pageId = 0;
    }
};

struct EntryHasher {
    std::size_t operator()(const Entry_t &e) const {
        std::size_t seed = 0;
        boost::hash_combine(seed, e.fileSegment);
        boost::hash_combine(seed, e.pageId);
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
    uint16_t flags;
    void *data;
};

struct PageHeader {
    uint64_t nextPid;
    uint16_t numEntries;
    uint16_t startOffset;
};
