//
// Created by raul on 24.09.17.
//
#include <unordered_map>
#include "KVManager.hpp"
#include <algorithm>

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
    uint16_t segNo = (uint16_t) (hVal >> 61 & 0b111);
    Entry_t newEntry{ segNo, mCurrGenPageNo[segNo]++, DEFAULT };
    entryMap.insert(std::make_pair(hVal, newEntry));
}

Entry_t KVManager::entryLookup(uint64_t hVal) {
    std::lock_guard<std::mutex>lock(entryMapMutex);
    auto it = entryMap.find(hVal);
    if (it != entryMap.end()){
        //copy page info
        return it->second;
    }
    else {
        return Entry_t { 0, 0, INVALID};
    }
}

std::string KVManager::get(std::string key) {
    using namespace std::string_literals;

    std::hash<std::string> hash;
    uint64_t hVal = hash(key);

    Entry_t pi = entryLookup (hVal);
    if(pi.flags & INVALID)
        return ""s;

    //Page p = bufferManager.readPin(pi.segment, pi.startPage)
    //Page: [ Header: [NextPid, NumEntries, startOFfset] ksize1, vsize1 ksize ... etc]
    //
    uint64_t currPid = pi.startPage;
    uint16_t currKeySize = 0;
    uint16_t currValueSize = 0;
    uint16_t sizeLeftInPage = PAGE_SIZE - sizeof(PageHeader);
    auto stringBegin = key.cbegin();
    auto stringEnd = key.cend();
    auto stringCurr = stringBegin;
    uint16_t currEntry = -1;

    char* page;
    PageHeader ph;

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

    return std::__cxx11::string();
}

bool KVManager::put(std::string key, std::string value) {

    std::hash<std::string> hash;
    uint64_t hVal = hash(key);

    Entry_t pi = entryLookup (hVal);
    if(pi.flags & INVALID)
        pi = genNewEntry(hVal);


    //Page p = bufferManager.writePin(pi.segment, pi.startPage)
    //Page: [ Header: [NextPid, NumEntries, startOFfset] ksize1, vsize1 ksize ... etc]


    return false;
}

bool KVManager::remove(std::string key) {

    std::hash<std::string> hash;
    uint64_t hVal = hash(key);

    Entry_t pi = entryLookup (hVal);
    if(pi.flags & INVALID)
        return false;


    //Page p = bufferManager.writePin(pi.segment, pi.startPage)
    //Page: [ Header: [NextPid, NumEntries, startOFfset] ksize1, vsize1 ksize ... etc]
    return false;
}
