#include "peer.h"
#include "AROLogInterface.h"
#include "commonDefines.h"
#include "diff.h"
int main( int argc, char * argv[] ){
    //Creating data
    srand(time(NULL));
    
    uint64_t * pool = new uint64_t [POOL_SIZE];
    struct timeval tp; gettimeofday(&tp, NULL); uint64_t base_time = tp.tv_sec * 1e6 + tp.tv_usec;
    for (int i = 0; i < POOL_SIZE; i++) pool[i] = (base_time -= rand()  / (RAND_MAX / 1000000000));
    for (unsigned int i = 0; i < POOL_SIZE/2; i++) std::swap(pool[i], pool[POOL_SIZE-i-1]);
    AROLogInterface::Log().OpenLog("/Users/yuehuang/Desktop/Decision/Log","linux_client.log");
    AROLogInterface::Log().SetLogLevel(logINFO);
    std::vector<Peer * > peer_list;
    unsigned int peer_size = 1;
    for (unsigned int i = 0; i < peer_size; i++) peer_list.push_back(new Peer(i,peer_list));
    peer_list[0]->set_peer_type(Peer::PRIORITY_PEER);
//    peer_list[2]->set_peer_type(Peer::GOOD_PEER);

    for (unsigned int i =0; i < peer_size; i++) peer_list[i]->load_cache(pool, POOL_SIZE);
//    for (unsigned int i =0; i < peer_size; i++) peer_list[i]->test();
    
//    for (unsigned int i = 0; i < peer_size; i++) peer_list[i]->get_online();
    while(std::cin.get()!= 'q'){}
    
    for (unsigned int i = 0; i < peer_size; i++) delete peer_list[i];
    
//    Diff::get().out_put_diff();
    
    AROLogInterface::Log().CloseLog();
    delete [] pool;
    
    return 0;
}
