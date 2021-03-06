#pragma once

#include "../DataTypes/PageData.hpp"

#include <unistd.h>
#include <stdio.h>

#include <iostream>
#include <unordered_map>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <fcntl.h>
#include <sys/stat.h>

class SimpleLinuxIoPolicy {
public:

    bool exists(const Entry_t& entry)
    {
        auto it = segments.find(entry.fileSegment);
        if ( it == segments.end() ) {
            int file = open(std::to_string(entry.fileSegment).c_str(), O_CREAT | O_RDWR,
                            S_IRUSR | S_IWUSR);
            if(file == -1) {
                std::cerr << "cannot open file: " << strerror(errno)
                << std::endl;
            }
            segments.insert(std::make_pair(entry.fileSegment, file));
            return false;
        }
        else {
            size_t size = getFileSize(it->second);
            if((entry.pageId * PAGE_SIZE ) > size)
            {
                return false;
            }
        }
    }

    void load(Segment& seg)
    {
        auto it = segments.find(seg.entry.fileSegment);

        size_t fileSize = getFileSize(it->second);

        bool isInRange = fileSize
                         >= (seg.entry.pageId + 1) * (PAGE_SIZE ) - 1;

        if(isInRange) {
            if (pread(it->second, seg.data, PAGE_SIZE,
                      seg.entry.pageId * PAGE_SIZE) == -1) {
                std::cerr << "cannot read page data: " << strerror(errno)
                << std::endl;
            }
        }
        else {
            //initialize a fresh new page
            memset(seg.data, 0, PAGE_SIZE);
            PageHeader& header = *((PageHeader *) seg.data);
            header.nextPid = seg.entry.pageId;
            header.numEntries = 0;
            header.startOffset = 0;
            seg.flags |= NEW;
        }
    }

    void flushPage(Segment& seg) {
        auto it = segments.find(seg.entry.fileSegment);

        if (it == segments.end()) {
            //dummy file got in, PANIC!
            exit(-1);
        }

        //write the new data
        if (pwrite(it->second, seg.data, PAGE_SIZE,
                   seg.entry.pageId * (PAGE_SIZE)) == -1) {
            std::cerr << "cannot write page data: " << strerror(errno)
                      << std::endl;
        }

    }

private:

    size_t getFileSize(int fd)
    {
        struct stat st;

        if (fstat(fd, &st) == -1)
            std::cerr << "cannot open file: " << strerror(errno);

        return (size_t) st.st_size;
    }

    std::unordered_map<uint16_t, int > segments;
};