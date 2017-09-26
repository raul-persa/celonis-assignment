#pragma once
#include <cstdint>
#include <atomic>

#include "../DataTypes/PageData.hpp"

class NoValidCandidateException : public std::exception {
public:
    NoValidCandidateException(const char * what)
        : exText(what)
    {

    }

    const char * what() {
        return exText.c_str();
    }
private:
    std::string exText;
};

class RoundRobinEvictionPolicy {
public:
    RoundRobinEvictionPolicy(uint64_t buffSize);

    void setBuffer(Segment* buffer) {
        this->buffer = buffer;
    }

    uint64_t lockNextCandidate() {
        uint64_t retries = 0;
        while (retries < 4 * buffSize)
        {
            //note: currPtr will overflow, when that happens the policy will not be purely fifo but this
            //avoids having locks. Ideally the size could be limited to a power of 2 and the % replace with a mask
            uint64_t currBufferId = (currPtr++) % buffSize;
            if (buffer[currPtr].mutex.try_lock())
            {
                return currBufferId;
            }
            ++retries;
        }
        //we retried 4 times, it's time to panic
        throw new NoValidCandidateException("Finding a candidate faild after 4 retries");
    }

private:
    const uint64_t buffSize;
    std::atomic<uint64_t> currPtr;
    Segment* buffer;

};