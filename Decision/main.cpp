#include <iostream>
#include "peer.h"
#include "priorityPeer.h"
#include "log.h"

int main( int argc, char * argv[] ){
    

    PriorityPeer * priority_peer = new PriorityPeer();
    std::vector<Peer * > peer_list;
    unsigned int peer_size = 3;
    for (unsigned int i = 0; i < peer_size; i++) {
        peer_list.push_back(new Peer(i,peer_list,priority_peer));
    }

    //ask others to get what?
    
    while(std::cin.get()!= 'q'){}
    Log::log().Print("Terminate Excution\n");
  
    for (unsigned int i = 0; i < peer_size; i++) {
        delete peer_list[i];
    }
    delete priority_peer;
    
	return 0;
}
