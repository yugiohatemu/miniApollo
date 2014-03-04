//
//  synchronizer.h
//  Decision
//
//  Created by Yue Huang on 2014-03-04.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__synchronizer__
#define __Decision__synchronizer__

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <inttypes.h>
#include <vector>

class Synchronizer{
    boost::system::error_code error;
    boost::asio::io_service &io_service;
    boost::asio::io_service::strand &strand;
    
    boost::asio::deadline_timer t_sync;
    std::vector<uint64_t> list;
    
public:
    Synchronizer(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand);
    ~Synchronizer();
    
    //emmmmmmmm..................how to broadcast sync?
};

#endif /* defined(__Decision__synchronizer__) */
