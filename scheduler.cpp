#include <sys/socket.h>
#include <algorithm>
#include "scheduler.h"
#include "socket.h"

Scheduler::Scheduler() {
  this->job_sequence = 0;
}

void Scheduler::create_queue_if_not_exists(std::string queue_name) {
  if (this->queues.find(queue_name) == this->queues.end()) {
    struct Queue *queue = new Queue;
    this->queues.insert(std::make_pair(queue_name, queue));
  }
}

void Scheduler::be_a_worker(int worker_id, std::string queue_name) {
  this->create_queue_if_not_exists(queue_name);
  struct Queue *queue = this->queues.find(queue_name)->second;
  queue->waiting_workers.push_back(worker_id);
  printf("- be a worker \n");
}

void Scheduler::be_a_job(
    int job_socket_id,
    std::string queue_name,
    long client_job_id,
    std::string data
) {
  this->create_queue_if_not_exists(queue_name);
  struct Queue *queue = this->queues.find(queue_name)->second;
  // Save data + add to waiting job
  queue->jobs.insert(std::make_pair(this->job_sequence, data));
  queue->waiting_jobs.push_back(this->job_sequence);
  // Create a link from socket id
  struct Link *link = new Link;
  link->job_socket_id = job_socket_id;
  link->worker_socket_id = -1;
  link->c_id = client_job_id;
  link->s_id = this->job_sequence;
  queue->links.insert(std::make_pair(this->job_sequence, link));
  // Increase sequence
  this->job_sequence ++;
  printf("- request a worker\n");
}

void Scheduler::be_a_progress(std::string queue_name, long job_id, std::string data) {
  if (this->queues.find(queue_name) == this->queues.end()) return;
  struct Queue *queue = this->queues.find(queue_name)->second;
  if (queue->links.find(job_id) == queue->links.end()) return;
  struct Link *link = queue->links.find(job_id)->second;
  Socket::emit(link->job_socket_id, Socket::PROGRESS_IT, queue_name, link->c_id, data);
  printf("- report progress\n");
}

void Scheduler::be_a_result(std::string queue_name, long job_id, std::string data) {
  if (this->queues.find(queue_name) == this->queues.end()) return;
  struct Queue *queue = this->queues.find(queue_name)->second;
  if (queue->links.find(job_id) == queue->links.end()) return;
  struct Link *link = queue->links.find(job_id)->second;
  Socket::emit(link->job_socket_id, Socket::DONE_IT, queue_name, link->c_id, data);
  //delete links, push worker back to waiting
  queue->waiting_workers.push_back(link->worker_socket_id);
  queue->links.erase(job_id);
  delete link;
  printf("- report result\n");
}

void Scheduler::process_job(std::string queue_name) {
  if (this->queues.find(queue_name) == this->queues.end()) return;
  struct Queue *queue = this->queues.find(queue_name)->second;
  printf("- w: %ld, j: %ld\n", queue->waiting_workers.size(), queue->waiting_jobs.size());
  if (queue->waiting_jobs.size() > 0 && queue->waiting_workers.size() > 0) {
    int worker_socket_id = queue->waiting_workers.front();
    long s_id = queue->waiting_jobs.front();
    queue->waiting_workers.pop_front();
    queue->waiting_jobs.pop_front();
    printf("- send job %ld to worker\n", s_id);
    // find job and update link
    std::string data = queue->jobs.find(s_id)->second;
    struct Link *link = queue->links.find(s_id)->second;
    link->worker_socket_id = worker_socket_id;
    Socket::emit(worker_socket_id, Socket::PROCESS_IT, queue_name, s_id, data);
    // delete job
    queue->jobs.erase(s_id);
  }
}

void Scheduler::remove_client(int socket_id) {
  std::map<std::string, Queue*>::iterator qit = queues.begin();
  printf("+ remove client \n");
  while (qit != queues.end()) {
    struct Queue * queue = qit->second;
    std::string queue_name = qit->first;
    std::list<int>::iterator ww_it = std::find(
      queue->waiting_workers.begin(),
      queue->waiting_workers.end(),
      socket_id
    );
    if (ww_it != queue->waiting_workers.end()) {
      printf("+ remove free worker\n");
      queue->waiting_workers.erase(ww_it);
    } else {
      std::map<long, Link*>::iterator lit = queue->links.begin();
      while (lit != queue->links.end()) {
        struct Link *link = lit->second;
        long s_id = link->s_id;
        if (link->job_socket_id == socket_id || link->worker_socket_id == socket_id) {
          queue->links.erase(lit++);
          // Remove jobs
          if (queue->jobs.find(s_id) != queue->jobs.end()) {
            queue->jobs.erase(s_id);
          }
          if (link->worker_socket_id == -1) {
            // remove waiting jobs
            queue->waiting_jobs.remove(s_id);
          } else {
            if (link->worker_socket_id == socket_id) {
              // worker died, notify error to consumer
              printf("+ worker died, send notice \n");
              std::string job_data = "{\"d\":{\"e\":\"WORKER_DIED\"}}";
              Socket::emit(
                link->job_socket_id,
                Socket::DONE_IT,
                queue_name,
                link->c_id,
                job_data
              );
            } else {
              // consumer died, push worker back to waiting_worker
              queue->waiting_workers.push_back(link->worker_socket_id);
            }
          }
          // Remove link
          delete link;
          continue;
        }
        lit++;
      }
    }
    printf(
      "- wj: %ld, ww: %ld, jobs: %ld, links: %ld\n",
      queue->waiting_jobs.size(),
      queue->waiting_workers.size(),
      queue->jobs.size(),
      queue->links.size()
    );
    qit ++;
  }
}


