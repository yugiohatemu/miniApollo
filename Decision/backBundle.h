//
//  backBundle.h
//  Decision
//
//  Created by Yue Huang on 2014-02-28.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__backBundle__
#define __Decision__backBundle__

#include <vector>
#include <inttypes.h>

class BackBundle{
    
public:
    struct Header{
        unsigned int size;
        uint64_t from, to;
        Header(std::vector<uint64_t> & sync):size((int)sync.size()),from(sync.front()),to(sync.back()){}
        Header(std::vector<uint64_t> & sync, unsigned int size):size(size), from(sync[0]), to(sync[size-1]){}
        Header(){}
        Header(const Header & h):size(h.size), from(h.from), to(h.to){}
        Header& operator=(const Header &h){
            size = h.size; from = h.from; to = h.to;
            return *this;
        }
        bool operator== (const Header &h){
            return size == h.size && from == h.from && to == h.to;
        }
        bool operator!= (const Header &h){return !(*this==h);}
    };
    
    BackBundle(std::vector<uint64_t> & sync, unsigned int size);
    BackBundle(Header &h);
    ~BackBundle();
    
    std::vector<uint64_t>& get_list();
    void set(std::vector<uint64_t> & sync, unsigned int size);
    void set(std::vector<uint64_t> & sync, unsigned int low, unsigned int high);
    bool is_ts_in_bb(uint64_t ts);
    void add_ts(uint64_t ts);
    bool operator<(const BackBundle & bb) const;
    Header get_header();
    bool is_bb_empty();
    bool is_bb_synced();
    void update_sync();
private:
    Header target;
    Header current;
    std::vector<uint64_t> ts_list;
    unsigned int size;
    bool bb_empty;
    bool bb_synced;
    
};

#endif /* defined(__Decision__backBundle__) */
