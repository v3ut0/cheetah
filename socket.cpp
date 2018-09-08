#include <sys/socket.h>
#include <regex>
#include "socket.h"

Socket::Socket(int id) {
    this->id = id;
    this->buffer = "";
}

std::list<long> Socket::decompose_elems(std::string str, std::string separator, int max_size) {
    std::list<long> elem_sizes;
    long str_len = str.length();
    long sep_len = separator.length();
    long prev_index = 0;
    for (int i = 0; i < str_len - sep_len + 1; i += 1) {
        if (str.substr(i, sep_len).compare(separator) == 0) {
            elem_sizes.push_back(i + sep_len - prev_index);
            prev_index = i + sep_len;
            if (elem_sizes.size() == max_size) break;
        }
    }
    return elem_sizes;
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
    std::list<long> elem_sizes = Socket::decompose_elems(this->buffer, "\r\n", 4);
    while (elem_sizes.size() >= 4) {
        std::regex int_regex = std::regex("[0-9]+");
        std::vector<long> int_values;
        std::vector<std::string> str_values;
        for (auto elem_size : elem_sizes) {
            std::string content = this->buffer.substr(1, elem_size - 3);
            if (this->buffer[0] == ':' && std::regex_match(content, int_regex)) {
                int_values.push_back(std::stol(content));
            }
            if (this->buffer[0] == '+') {
                str_values.push_back(content);
            }
            this->buffer = this->buffer.substr(elem_size, this->buffer.length() - elem_size);
        }
        // VALIDATE
        if (int_values.size() >= 2 && str_values.size() >= 2) {
            long opcode = int_values[0];
            long job_id = int_values[1];
            std::string queue_name = str_values[0];
            std::string job_data = str_values[1];
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
        }
        // Next packet
        elem_sizes = Socket::decompose_elems(this->buffer, "\r\n", 4);
    }
}

void Socket::onDisconnected() {
    scheduler->remove_client(this->id);
}

