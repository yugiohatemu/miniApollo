#include <iostream>
#include "peer.h"
#include "priorityPeer.h"
#include "AROLog.h"

int main( int argc, char * argv[] ){
    AROLog::Log().OpenLog("/Users/yuehuang/Desktop/Decision/Log","linux_client.log");
    AROLog::Log().SetLogLevel(logDEBUG);

    PriorityPeer * priority_peer = new PriorityPeer();
    std::vector<Peer * > peer_list;
    unsigned int peer_size = 2;
    for (unsigned int i = 0; i < peer_size; i++) {
        peer_list.push_back(new Peer(i,peer_list,priority_peer));
    }

    //ask others to get what?
    
    
    while(std::cin.get()!= 'q'){}
   
    for (unsigned int i = 0; i < peer_size; i++) {
        delete peer_list[i];
    }
    delete priority_peer;
    AROLog::Log().CloseLog();
    return 0;
}
