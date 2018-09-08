#ifndef CHEETAH_SCHEDULER_H
#define CHEETAH_SCHEDULER_H

#include <stdio.h>
#include <map>
#include <list>
#include <string>
#include <iostream>

struct Link {
    int job_socket_id;
    int worker_socket_id;
    long s_id; //server job id
    long c_id; //client job id
};

struct Queue {
    std::list<int> waiting_workers; // socket_id
    std::list<long> waiting_jobs; // job_id
    std::map<long, std::string> jobs; // job_id -> job
    std::map<long, struct Link*> links; // job_id -> link
};

class Scheduler {
private:
    std::map<std::string, struct Queue*> queues;
    long job_sequence;
public:
    Scheduler();
    void be_a_worker(int socket_id, std::string queue_name);
    void be_a_job(int socket_id, std::string queue_name, long client_job_id, std::string data);
    void be_a_result(std::string queue_name, long job_id, std::string data);
    void be_a_progress(std::string queue_name, long job_id, std::string data);
    void process_job(std::string queue_name);
    void remove_client(int socket_id);
    void create_queue_if_not_exists(std::string queue_name);
};

#endif //CHEETAH_SCHEDULER_H

