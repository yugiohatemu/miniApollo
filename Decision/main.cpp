#include <iostream>
#include "peer.h"
#include "priorityPeer.h"
#include "AROLog.h"

int main( int argc, char * argv[] ){
    

    PriorityPeer * priority_peer = new PriorityPeer();
    std::vector<Peer * > peer_list;
    unsigned int peer_size = 2;
    for (unsigned int i = 0; i < peer_size; i++) {
        peer_list.push_back(new Peer(i,peer_list,priority_peer));
    }

    //ask others to get what?
    AROLog::Log().SetLogLevel(logDEBUG);
    
    while(std::cin.get()!= 'q'){}
   
    for (unsigned int i = 0; i < peer_size; i++) {
        delete peer_list[i];
    }
    delete priority_peer;
    
	return 0;
}
