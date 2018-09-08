#ifndef CHEETAH_SOCKET_H
#define CHEETAH_SOCKET_H

#include <set>
#include "scheduler.h"

struct IntStr {
    long int_val;
    bool is_int;
    std::string str_val;
};

class Socket {
private:
    int id;
    std::string buffer;
    Scheduler *scheduler;
public:
    Socket(int id);
    void onConnected(Scheduler *scheduler);
    void onReceived(char *buffer, int len);
    void onDisconnected();
    static long emit(int to, long opcode, std::string queue_name, long job_id, std::string data);
    static std::list<long> decompose_elems(std::string str, std::string separator, int max_size);
    // client -> server
    static const long BE_A_WORKER = 0;
    static const long BE_A_JOB = 1;
    static const long BE_A_RESULT = 2;
    static const long BE_A_PROGRESS = 3;
    // server -> client
    static const long PROCESS_IT = 100;
    static const long DONE_IT = 101;
    static const long PROGRESS_IT = 102;
};

#endif //CHEETAH_SOCKET_H

