#include "peer.h"
#include "AROLog.h"

int main( int argc, char * argv[] ){
    AROLog::Log().OpenLog("/Users/yuehuang/Desktop/Decision/Log","linux_client.log");
    AROLog::Log().SetLogLevel(logINFO);

    srand(time(NULL));
    std::vector<Peer * > peer_list;
    unsigned int peer_size = 3;
    for (unsigned int i = 0; i < peer_size; i++) peer_list.push_back(new Peer(i,peer_list));
    peer_list[0]->set_peer_type(Peer::GOOD_PEER);
    peer_list[1]->set_peer_type(Peer::GOOD_PEER);
    peer_list[2]->set_peer_type(Peer::PRIORITY_PEER);
    
    for (unsigned int i = 0; i < peer_size; i++) peer_list[i]->get_online();
    
    while(std::cin.get()!= 'q'){}
   
    for (unsigned int i = 0; i < peer_size; i++) delete peer_list[i];
    AROLog::Log().CloseLog();
    return 0;
}
