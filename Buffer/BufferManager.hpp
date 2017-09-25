#pragma once

#include "../DataTypes/PageData.hpp"
#include "../KVManager/KVManager.hpp"

#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <atomic>

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

template <class EvictionPolicy, class IoPolicy>
class BufferManager {
public:

public:
    BufferManager(const uint64_t buffSize)
        : buffSize(buffSize)
        , evictor(buffSize)
    {
        segments = new Segment[buffSize];
        evictor.setBuffer(segments);
        dummySegment.data = (void *) NULL_PAGE;

        for(uint16_t i = 0; i < buffSize; ++i)
        {
            segments[i].data = malloc(PAGE_SIZE);
        }
    }

    Segment& pinPage(const Entry_t& entry, const bool exclusive = true, const bool createSegment = true) {
    retry:
        segmentLookupMutex.lock();
        auto it = segmentLookup.find(entry);
        if(it != segmentLookup.end() ) {
            uint64_t segmentId = it->second;
            //release the lock since we might need to wait for the shared lock
            segmentLookupMutex.unlock();

            if(!exclusive)
                segments[segmentId].mutex.lock_shared();
            else
                segments[segmentId].mutex.lock();

            //check that all is good
            if (segments[segmentId].entry != entry)
            {
                // segment changed pages in the meantime, have to retry
                if(!exclusive)
                    segments[segmentId].mutex.unlock_shared();
                else
                    segments[segmentId].mutex.unlock();
                goto retry;
            }

            return segments[segmentId];
        }
        else
        {
            auto loadingIt = loadingSegments.find(entry);
            if(loadingIt != loadingSegments.end())
            {
                //someone else is already loading this segment, wait and check again
                uint64_t segmentId = loadingIt->second;
                segmentLookupMutex.unlock();
                waitLoad(loadingIt->second);
                //check that all is good
                if (segments[segmentId].entry != entry)
                {
                    // segment changed pages in the meantime, have to retry
                    if(!exclusive)
                        segments[segmentId].mutex.unlock_shared();
                    else
                        segments[segmentId].mutex.unlock();
                    goto retry;
                }

                return segments[segmentId];
            }

            if(currPageCount == buffSize) {
                char* page;
                //get exclusive lock on page
                uint64_t victimNum = evictor.lockNextCandidate();
                Segment *seg = &segments[victimNum];
                //remove the page from reference
                segmentLookup.erase(seg->entry);
                //mark segment as in loading
                loadingSegments.insert(std::make_pair(seg->entry, victimNum));
                //release lock for I/O
                segmentLookupMutex.unlock();
                //write page to disk if dirty
                if(io.exists(entry) || createSegment) {
                    //page exists or we need to create it
                    if (seg->entry.flags & DIRTY)
                        io.flushPage(*seg);
                    // overwrite entry with the new entry
                    seg->entry = entry;
                    io.load(*seg);
                    //loading is done, we can free the segment lock
                    seg->mutex.unlock();
                }
                else {
                    //no need to create the page
                    seg = &dummySegment;
                }

                segmentLookupMutex.lock();
                loadingSegments.erase(seg->entry);
                segmentLookup.insert(std::make_pair(seg->entry, victimNum));
                segmentLookupMutex.unlock();

                return *seg;
            }
        }
    }

    void unpinPage(Segment& seg, bool exclusive){
        Segment segment = const_cast<Segment&>(seg);

        if(exclusive)
        {
            seg.mutex.unlock();
        }
        else
        {
            seg.mutex.unlock_shared();
        }
    }
private:

private:

    void waitLoad(uint64_t segNum) {
        //todo: implement
        //note: can either be a cond variable or just a backoff since we retry anyway
    }

    const uint64_t buffSize;
    std::atomic<uint64_t>  currPageCount;
    std::unordered_map<Entry_t, uint64_t, EntryHasher> segmentLookup;
    std::unordered_map<Entry_t, uint64_t, EntryHasher> loadingSegments;
    std::mutex segmentLookupMutex;

    Segment * segments;
    Segment dummySegment;
    EvictionPolicy evictor;
    IoPolicy io;
};
