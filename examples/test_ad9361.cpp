#include <iostream>
#include <signal.h>
#include "ad9361.h"

AD9361 ad9361;

static void handle_sig(int sig)
{
	cout << "Waiting for streaming to stop" << endl;
	ad9361.stopRxStream();
}

int main(int argc, char **argv)
{
    using namespace std;
    string devIp("192.168.2.1");
    
    // Init AD9361 device
    if(!ad9361.init(devIp)) {
        cerr << "Unable to initialize AD9361 context on " << devIp << endl;
        return -1;
    }

    // install sigact to interrupt streaming
    signal(SIGINT, handle_sig);
    
    // start rx stream
    if(!ad9361.startRxStream(96000000)) {
        cerr << "Unable to start RX streaming" << endl;
    }

    ad9361.deinit();
    cout << "Done, exiting" << endl;

    return 0;
}