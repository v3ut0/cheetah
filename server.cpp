#include <map>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include "server.h"
#include "socket.h"

void Server::serve(int port_number) {
  int listen_sd, new_sd, len, on = 1, max_sd = -1;
  struct sockaddr_in addr;
  char buffer[1024];
  fd_set working_set;
  fd_set master_set;
  Scheduler * scheduler = new Scheduler();
  std::map<int, Socket*>::iterator it;
  std::map<int, Socket*> sockets;

  if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("- socket failed\n");
    exit(-1);
  }
  if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
    perror("- setsockopt failed\n");
    close(listen_sd);
    exit(-1);
  }
  if (ioctl(listen_sd, FIONBIO, (char *)&on) < 0) {
    perror("- ioctl() failed\n");
    close(listen_sd);
    exit(-1);
  }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port_number);
  if (bind(listen_sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("- bind failed\n");
    close(listen_sd);
    exit(-1);
  }
  if (listen(listen_sd, 32) < 0) {
    perror("- listen failed\n");
    close(listen_sd);
    exit(-1);
  }
  printf("- server started at port %d\n", port_number);
  max_sd = listen_sd;
  FD_ZERO(&master_set);
  FD_SET(listen_sd, &master_set);
  while (true) {
    memcpy(&working_set, &master_set, sizeof(master_set));
    if (select(max_sd + 1, &working_set, NULL, NULL, NULL) < 0) {
      perror("- select failed\n");
      break;
    }
    for (int i = 0; i <= max_sd; i += 1) {
      if (FD_ISSET(i, &working_set)) {
        if (i == listen_sd) {
          // server socket
          if ((new_sd = accept(listen_sd, NULL, NULL)) < 0) {
            perror("- accept failed\n");
            break;
          }
          Socket *socket = new Socket(new_sd);
          socket->onConnected(scheduler);
          sockets.insert(std::make_pair(new_sd, socket));
          if (new_sd > max_sd) max_sd = new_sd;
          FD_SET(new_sd, &master_set);
        } else {
          // client socket
          while (true) {
            len = recv(i, buffer, sizeof(buffer), MSG_DONTWAIT);
            if (len < 0) break;
            if (len == 0) {
              // close connection
              if (sockets.find(i) != sockets.end()) {
                Socket *socket = sockets.find(i)->second;
                socket->onDisconnected();
                delete socket;
                sockets.erase(i);
              }
              close(i);
              FD_CLR(i, &master_set);
              if (i == max_sd) {
                while (!FD_ISSET(max_sd, &master_set)) max_sd -= 1;
              }
              break;
            }
            if (sockets.find(i) != sockets.end()) {
              Socket *socket = sockets.find(i)->second;
              socket->onReceived(buffer, len);
            }
          }
        }
      }
    }
  }
}
