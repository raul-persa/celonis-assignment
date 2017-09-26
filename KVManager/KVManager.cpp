//
// Created by raul on 24.09.17.
//
#include <unordered_map>
#include "KVManager.hpp"
#include <algorithm>

namespace {
    char *dataBegin(char *page) {
        PageHeader &header = *((PageHeader *) page);

        return (page + sizeof(PageHeader) + header.numEntries * sizeof(uint16_t));
    }

    char *dataEnd(char *page) {
        return page + PAGE_SIZE;
    }

    PageHeader& getHeader(char* page){
        return *((PageHeader *) page);
    }

    uint16_t * getEntrySizeArray(char* page) {
        return (uint16_t *) (page + sizeof(PageHeader));
    }

    bool hasEntry(char * page, const uint16_t entry)
    {
        PageHeader & header = getHeader(page);
        return header.numEntries > entry;
    }

}

const uint16_t MORE_PAGES_MASK = 1 << 15;
const uint16_t SIZE_MASK = MORE_PAGES_MASK - 1;

Entry_t KVManager::genNewEntry(const uint64_t hVal) {
    //use top 3 bits to find fileSegment
    std::lock_guard<std::mutex> lock(entryMapMutex);
    uint16_t segNo = (uint16_t) ((hVal >> 61) & 0b111);
    Entry_t newEntry;
    newEntry.fileSegment = segNo;
    newEntry.pageId = mCurrGenPageNo[segNo]++;
    entryMap.insert(std::make_pair(hVal, newEntry));
    return std::move(newEntry);
}

bool KVManager::entryLookup(const uint64_t hVal, Entry_t &e) {
    std::lock_guard<std::mutex> lock(entryMapMutex);
    auto it = entryMap.find(hVal);
    if (it != entryMap.end()) {
        //copy page info
        e = it->second;
        return true;
    } else {
        return false;
    }
}

// advances segment until they key is found.
// if the key is not found segment will point to the NULL_PAGE
uint16_t KVManager::crawlToKey(Segment *&segment, const std::string &key, const bool exclusive = true) {

    bool isKey = true;

    Entry_t &pageEntry = segment->entry;
    uint64_t currPid = pageEntry.pageId;
    uint16_t currentEntry = 0;

    const auto stringBegin = key.cbegin();
    const auto stringEnd = key.cend();
    auto stringCurr = stringBegin;

    bool skipStartOffset = true;

    do {
        char *page = (char *) segment->data;
        if (page == NULL_PAGE)
            return uint16_t(-1);

        //get header information
        PageHeader &header = getHeader(page);
        uint16_t *entrySizes = getEntrySizeArray(page);
        uint32_t currentOffset = header.startOffset;
        currentEntry = 0;

        if (!skipStartOffset) {
            //handle leftover from last page
            if (dataEnd(page) - currentOffset < dataBegin(page)) {
                //whole page is filled with part of the key
                if (stringEnd - stringCurr < currentOffset) {
                    //remaining string is shorter than the entry, keys definitely don't match;
                    skipStartOffset = true;
                } else {
                    //string size matches, compare strings
                    std::reverse_iterator<char *> rBegin(dataEnd(page));
                    std::reverse_iterator<char *> rEnd = rBegin + currentOffset;

                    if (std::equal(stringCurr, stringCurr + currentOffset, rBegin, rEnd)) {
                        skipStartOffset = false;
                        std::advance(stringCurr, currentOffset);
                    } else {
                        skipStartOffset = true;
                    }
                }
            } else {
                if (stringEnd - stringCurr == currentOffset) {
                    //string size matches, compare strings
                    std::reverse_iterator<char *> rBegin(dataEnd(page));
                    std::reverse_iterator<char *> rEnd = rBegin + currentOffset;

                    skipStartOffset = true;
                    if (std::equal(stringCurr, stringCurr + currentOffset, rBegin, rEnd)) {
                        return uint16_t(0);
                    }
                }
            }
        }

        //handle entries
        for (uint16_t entry = 0; entry < header.numEntries; ++entry) {
            size_t currEntrySize = (size_t) entrySizes[entry];
            currentEntry = entry;
            if (isKey) {
                //new key entry, reset key iterator
                stringCurr = stringBegin;

                if ((dataEnd(page) - currentOffset - currEntrySize) < dataBegin(page)) {
                    //key continues on next page

                    //entry size is actually smaller than reported
                    currEntrySize = (dataEnd(page) - currentOffset) - dataBegin(page);

                    if (stringEnd - stringCurr < currEntrySize) {
                        //remaining string is shorter than the entry, keys definitely don't match;
                        skipStartOffset = true;
                    } else {
                        //string size matches, compare strings
                        std::reverse_iterator<char *> rBegin(dataEnd(page) - currentOffset);
                        std::reverse_iterator<char *> rEnd = rBegin + currEntrySize;

                        if (std::equal(stringCurr, stringCurr + currEntrySize, rBegin, rEnd)) {
                            skipStartOffset = false;
                            std::advance(stringCurr, currEntrySize);
                        } else {
                            skipStartOffset = true;
                        }
                    }
                } else {
                    // key is finished on this page
                    if (stringEnd - stringCurr == currEntrySize) {
                        //string size matches, compare strings
                        std::reverse_iterator<char *> rBegin(dataEnd(page) - currentOffset);
                        std::reverse_iterator<char *> rEnd = rBegin + currEntrySize;

                        skipStartOffset = true;
                        if (std::equal(stringCurr, stringCurr + currEntrySize, rBegin, rEnd)) {
                            return entry;
                        }
                    }
                }
                //more offset by 1 entry
                currentOffset += currEntrySize;
            }
            //entries always alternate key, value
            isKey = !isKey;
        }

        //load new page
        if (currPid != header.nextPid) {
            Entry_t next;
            next.fileSegment = segment->entry.fileSegment;
            next.pageId = header.nextPid;
            Segment *nextSegment = &bm.pinPage(next, exclusive, false);
            bm.unpinPage(*segment, exclusive);
            segment = nextSegment;
        } else {
            return currentEntry;
        }
    } while (true);

}

std::string KVManager::get(const std::string &key) {
    using namespace std::string_literals;

    std::hash<std::string> hash;
    uint64_t hVal = hash(key);

    Entry_t entry;
    if (entryLookup(hVal, entry))
        return ""s;

    //Page p = bufferManager.readPin(entry.fileSegment, entry.pageId)
    //Page: [ Header: [NextPid, NumEntries, startOFfset] ksize1, vsize1 ksize ... etc]
    //

    Segment *segment = &bm.pinPage(entry, false, false);

    uint16_t pageRecordId = crawlToKey(segment, key, false);

    char *page = (char *) segment->data;
    if (page == NULL_PAGE) {
        return ""s;
    }

    PageHeader &header = getHeader(page);
    uint16_t *entrySizes = getEntrySizeArray(page);

    if(header.nextPid == segment->entry.pageId && header.numEntries == pageRecordId )
    {
        //reached the end and did not find any matches
        return ""s;
    }
    else {
        std::stringstream ss;
        readString(segment, pageRecordId, ss);
        return ss.str();
    }
}

bool KVManager::put(const std::string &key, const std::string &value) {

    std::hash<std::string> hash;
    uint64_t hVal = hash(key);

    Entry_t entry;
    if (!entryLookup(hVal, entry))
        entry = genNewEntry(hVal);

    //Page p = bufferManager.writePin(pi.fileSegment, pi.pageId)
    //Page: [ Header: [NextPid, NumEntries, startOFfset] ksize1, vsize1 ksize ... etc]


    return false;
}

bool KVManager::remove(const std::string &key) {

    std::hash<std::string> hash;
    uint64_t hVal = hash(key);

    Entry_t entry;
    if (!entryLookup(hVal, entry))
        return false;


    //Page p = bufferManager.writePin(pi.fileSegment, pi.pageId)
    //Page: [ Header: [NextPid, NumEntries, startOFfset] ksize1, vsize1 ksize ... etc]
    return false;
}

void KVManager::readString(Segment *segment, uint16_t recordId, std::stringstream &ss) {
    char* page = (char*) segment->data;
    if (page == NULL_PAGE)
    {
        return;
    }

    PageHeader * header = &getHeader(page);

    if(header->numEntries < recordId)
    {
        //nnothing to read
        return ;
    }

    uint64_t currentSize = 0;

    if(!hasEntry(page, recordId))
    {
        //value starts at next page
        assert(header->nextPid != segment->entry.pageId);
        Entry_t entry;
        entry.pageId = header->nextPid;
        entry.fileSegment = segment->entry.fileSegment;
        Segment * newSegment = &bm.pinPage(entry, false);
        bm.unpinPage(*segment, false);
        segment = newSegment;
        page = (char*) segment->data;
        header = &getHeader(page);
        recordId = 0;
    }

    bool done = false;

//    do
//    {
//        ss.put()
//        if ()
//    } while (header.nextPid != segment->entry.pageId || done);


}
