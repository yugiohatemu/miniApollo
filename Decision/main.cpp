#include <iostream>
#include "peer.h"
#include "dataPool.h"
#include "log.h"

int main( int argc, char * argv[] ){
    

    DataPool * data_pool = new DataPool();
    std::vector<Peer * > peer_list;
    unsigned int peer_size = 3;
    for (int i = 0; i < peer_size; i++) {
        peer_list.push_back(new Peer(i,peer_list,data_pool));
    }

    //ask others to get what?
    
    while(std::cin.get()!= 'q'){}
    Log::log().Print("Terminate Excution\n");
  
    for (int i = 0; i < peer_size; i++) {
        delete peer_list[i];
    }
    delete data_pool;
    
	return 0;
}
