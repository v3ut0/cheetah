#include <iostream>
#include <vector>
#include <regex>
#include "config.h"
#include "server.h"

std::vector<std::string> get_cmds(int argc, char *argv[], std::vector<std::string> options) {
  std::vector<std::string> cmds;
  for (int i = 0; i < argc; i += 1) {
    std::string arg(argv[i]);
    for (auto option: options) {
      if (0 == arg.find(option)) {
        std::size_t found = arg.find_last_of(option);
        if (arg[found + 1] == '=') {
          cmds.push_back(arg.substr(found + 2));
        } else if (i + 1 < argc) {
          cmds.push_back(argv[i + 1]);
          i += 1;
        } else {
          cmds.push_back("");
        }
      }
    }
  }
  return cmds;
}

int main(int argc, char *argv[]) {
  int config_port = 1991;
  std::vector<std::string> helps;
  helps.push_back("--help");
  helps.push_back("-h");
  std::vector<std::string> versions;
  versions.push_back("--version");
  versions.push_back("-v");
  std::vector<std::string> ports;
  ports.push_back("--port");
  ports.push_back("-p");
  std::vector<std::string> h = get_cmds(argc, argv, helps); 
  if (h.size() > 0) {
    printf("  Usage: cheetah [options]\n");
    printf("    options:\n");
    printf("      -p, --port     port number\n");
    printf("      -h, --help     show usage\n");
    printf("      -v, --version  show version\n");
    return 0;
  }
  std::vector<std::string> v = get_cmds(argc, argv, versions);
  if (v.size() > 0) {
    printf("v%d.%d\n", 0, 1);
    return 0;
  }
  std::vector<std::string> p = get_cmds(argc, argv, ports);
  if (p.size() > 0) {
    std::string port_number = p[0];
    if (!std::regex_match(port_number, std::regex("[0-9]+"))) {
      printf("invalid port number\n");
      return 0;
    }
    config_port = std::stoi(port_number);
  }
  Server::serve(config_port);
}
