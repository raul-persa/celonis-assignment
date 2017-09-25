//
// Created by raul on 24.09.17.
//
#include <unordered_map>
#include "KVManager.hpp"
#include <algorithm>

const uint16_t MORE_PAGES_MASK = 1 << 15;
const uint16_t SIZE_MASK = MORE_PAGES_MASK - 1;

struct Page {
    PageHeader header;

    char* getEnd(){
        return (char*) this + PAGE_SIZE;
    }


    uint16_t getKeySize(uint16_t id) {

    }

    uint16_t getValSize(uint16_t id) {

    }
};

Entry_t KVManager::genNewEntry(uint64_t hVal) {
    //use top 3 bits to find segment
    std::lock_guard<std::mutex>lock(entryMapMutex);
    uint16_t segNo = (uint16_t) ( (hVal >> 61) & 0b111);
    Entry_t newEntry;
    newEntry.segment = segNo;
    newEntry.startPage = mCurrGenPageNo[segNo]++;
    newEntry.flags = DEFAULT;
    entryMap.insert(std::make_pair(hVal, newEntry));
    return std::move(newEntry);
}

Entry_t KVManager::entryLookup(uint64_t hVal) {
    std::lock_guard<std::mutex>lock(entryMapMutex);
    auto it = entryMap.find(hVal);
    if (it != entryMap.end()){
        //copy page info
        return it->second;
    }
    else {
        return std::move(Entry_t());
    }
}

std::string KVManager::get(const std::string &key) {
    using namespace std::string_literals;

    std::hash<std::string> hash;
    uint64_t hVal = hash(key);

    Entry_t pi = entryLookup (hVal);
    if(pi.flags & INVALID)
        return ""s;

    //Page p = bufferManager.readPin(pi.segment, pi.startPage)
    //Page: [ Header: [NextPid, NumEntries, startOFfset] ksize1, vsize1 ksize ... etc]
    //

    const Segment& segment = bm.pinPage(pi, false, false);
    char * page = (char *) segment.data;
    if(page == NULL_PAGE)
        return ""s;

    uint64_t currPid = pi.startPage;
    uint16_t currKeySize = 0;
    uint16_t currValueSize = 0;
    uint16_t sizeLeftInPage = PAGE_SIZE - sizeof(PageHeader);
    auto stringBegin = key.cbegin();
    auto stringEnd = key.cend();
    auto stringCurr = stringBegin;
    uint16_t currEntry = 0;

    PageHeader ph = *((PageHeader * ) page);
    //no entries here
    if(ph.numEntries == 0)
        return ""s;

    bool match = true;
    uint16_t * sizeTable = (uint16_t *)(page + sizeof(PageHeader));

    currKeySize = sizeTable[2*currEntry];
    bool moreKeyPages = (bool) (currKeySize & MORE_PAGES_MASK);
    currKeySize &= SIZE_MASK;

    currValueSize = sizeTable[2*currEntry + 1];
    bool moreValuePages = (bool) (currValueSize & MORE_PAGES_MASK);
    currValueSize &= SIZE_MASK;

    if( (stringEnd - stringCurr) < currKeySize)
    {
        //currKey is longer then the key we are looking for, skip it
    }
    else
    {

    std::reverse_iterator<char *> rBegin(page + PAGE_SIZE);
    std::reverse_iterator<char *> rEnd = rBegin + currKeySize;

    match &= std::equal(stringCurr,stringCurr + currKeySize, rBegin, rEnd);

    if(moreKeyPages)
    {
        stringCurr = stringCurr + currKeySize;
    }
    else {
        if (match)
        {
            //return getVal(page, currValueSize, currKeySize);
        }
    }

    }



    do {
        std::reverse_iterator<char *> rBegin(page + PAGE_SIZE);
        std::reverse_iterator<char *> rEnd(page + PAGE_SIZE - ph.startOffset);
        bool match = std::equal(stringCurr, stringEnd, rBegin, rEnd);

        if(match)
            break;

        if(currEntry <= ph.numEntries) {
            ++currEntry;
            currKeySize = ((uint16_t *)(page + sizeof(PageHeader)))[2*currEntry];
            currValueSize = ((uint16_t *)(page + sizeof(PageHeader)))[2*currEntry + 1];
            sizeLeftInPage -= 2 * sizeof(uint16_t);

            stringCurr = stringBegin;

            if(currKeySize > sizeLeftInPage) {
                std::reverse_iterator<char *> rBegin(page + PAGE_SIZE);
                std::reverse_iterator<char *> rEnd(page + PAGE_SIZE - ph.startOffset);
                bool match = std::equal(stringCurr, stringEnd, rBegin, rEnd);
            }

        }

    } while (true);

    //handler.getValue;

    return std::string();
}

bool KVManager::put(const std::string &key, const std::string &value) {

    std::hash<std::string> hash;
    uint64_t hVal = hash(key);

    Entry_t pi = entryLookup (hVal);
    if(pi.flags & INVALID)
        pi = genNewEntry(hVal);


    //Page p = bufferManager.writePin(pi.segment, pi.startPage)
    //Page: [ Header: [NextPid, NumEntries, startOFfset] ksize1, vsize1 ksize ... etc]


    return false;
}

bool KVManager::remove(const std::string &key) {

    std::hash<std::string> hash;
    uint64_t hVal = hash(key);

    Entry_t pi = entryLookup (hVal);
    if(pi.flags & INVALID)
        return false;


    //Page p = bufferManager.writePin(pi.segment, pi.startPage)
    //Page: [ Header: [NextPid, NumEntries, startOFfset] ksize1, vsize1 ksize ... etc]
    return false;
}
