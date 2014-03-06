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
        Header(std::vector<uint64_t> & sync):size((unsigned int)sync.size()),from(sync.front()),to(sync.back()){}
        Header(std::vector<uint64_t> & sync, unsigned int size):size(size), from(sync.front()), to(sync.back()){}
        Header():size(0), from(0), to(0){}
        Header(const Header & h):size(h.size), from(h.from), to(h.to){}
        Header& operator=(const Header &h){
            size = h.size; from = h.from; to = h.to;
            return *this;
        }
        bool operator== (const Header &h){return size == h.size && from == h.from && to == h.to;}
        bool operator!= (const Header &h){return !(*this==h);}
    };
    
    BackBundle(std::vector<uint64_t> & sync, unsigned int size);
    BackBundle(Header &h);
    BackBundle(const BackBundle & b);
    ~BackBundle();
    
    std::vector<uint64_t> get_list();

    bool is_ts_in_bb(uint64_t ts);
    void add_ts(uint64_t ts);
    bool operator<(const BackBundle & bb) const;
    
    Header target;
    Header current;
    
    bool is_bb_synced();
    void update_sync();
    unsigned int size();
    void sync(BackBundle * bb);
private:
    
    std::vector<uint64_t> ts_list;
    bool bb_synced;
    
};

#endif /* defined(__Decision__backBundle__) */
