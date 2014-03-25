#include "peer.h"
#include "AROLogInterface.h"

int main( int argc, char * argv[] ){
    //Creating data
    srand(time(NULL));
    unsigned int size = 170;
    uint64_t * pool = new uint64_t [size];
    struct timeval tp; gettimeofday(&tp, NULL); uint64_t base_time = tp.tv_sec * 1e6 + tp.tv_usec;
    for (int i = 0; i < size; i++) pool[i] = (base_time -= rand()  / (RAND_MAX / 1000000000));
    for (unsigned int i = 0; i < size/2; i++) std::swap(pool[i], pool[size-i-1]);
    AROLogInterface::Log().OpenLog("/Users/yuehuang/Desktop/Decision/Log","linux_client.log");
    AROLogInterface::Log().SetLogLevel(logINFO);
    std::vector<Peer * > peer_list;
    unsigned int peer_size = 2;
    for (unsigned int i = 0; i < peer_size; i++) peer_list.push_back(new Peer(i,peer_list));
    peer_list[0]->set_peer_type(Peer::PRIORITY_PEER);
//    peer_list[2]->set_peer_type(Peer::GOOD_PEER);
    peer_list[1]->set_peer_type(Peer::PRIORITY_PEER);
    for (unsigned int i =0; i < peer_size; i++) peer_list[i]->load_cache(pool, size);
    for (unsigned int i =0; i < peer_size; i++) peer_list[i]->test();
    
//    for (unsigned int i = 0; i < peer_size; i++) peer_list[i]->get_online();
    while(std::cin.get()!= 'q'){}
    
    for (unsigned int i = 0; i < peer_size; i++) delete peer_list[i];
    AROLogInterface::Log().CloseLog();
    delete [] pool;
    
    return 0;
}
