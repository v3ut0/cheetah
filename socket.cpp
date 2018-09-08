#include <sys/socket.h>
#include <regex>
#include "socket.h"

Socket::Socket(int id) {
    this->id = id;
    this->buffer = "";
}

long Socket::emit(int to, long opcode, std::string queue_name, long job_id, std::string data) {
    std::string int_prefix = ":";
    std::string str_prefix = "+";
    std::string sep = "\r\n";
    std::string packet = "";
    packet += int_prefix + std::to_string(opcode) + sep;
    packet += str_prefix + queue_name + sep;
    packet += int_prefix + std::to_string(job_id) + sep;
    packet += str_prefix + data + sep;
    return send(to, packet.c_str(), packet.length(), 0);
}

void Socket::onConnected(Scheduler *scheduler) {
    this->scheduler = scheduler;
}

void Socket::onReceived(char *buffer, int len) {
    this->buffer += std::string(buffer, buffer + len);
    std::size_t sep_index = this->buffer.find("\r\n");
    while (sep_index != std::string::npos) {
        std::string data = this->buffer.substr(0, sep_index + 2);
        std::string content = data.substr(1, data.length() - 3);
        this->elems.push_back(content);
        this->buffer = this->buffer.substr(sep_index + 2, this->buffer.length() - data.length());
        sep_index = this->buffer.find("\r\n");
    }
    std::regex int_regex = std::regex("[0-9]+");
    while (this->elems.size() >= 4) {
        if (std::regex_match(this->elems[0], int_regex) && std::regex_match(this->elems[2], int_regex)) {
            long opcode = std::stol(this->elems[0]);
            long job_id = std::stol(this->elems[2]);
            std::string queue_name = this->elems[1];
            std::string job_data = this->elems[3];
            switch (opcode) {
                case Socket::BE_A_WORKER:
                    scheduler->be_a_worker(this->id, queue_name);
                    scheduler->process_job(queue_name);
                    break;
                case Socket::BE_A_JOB:
                    scheduler->be_a_job(this->id, queue_name, job_id, job_data);
                    scheduler->process_job(queue_name);
                    break;
                case Socket::BE_A_PROGRESS:
                    scheduler->be_a_progress(queue_name, job_id, job_data);
                    break;
                case Socket::BE_A_RESULT:
                    scheduler->be_a_result(queue_name, job_id, job_data);
                    scheduler->process_job(queue_name);
                    break;
            }
            
        };
        for (int i = 0; i < 4; i++) {
            this->elems.erase(this->elems.begin());
        }
    }
}

void Socket::onDisconnected() {
    scheduler->remove_client(this->id);
}

