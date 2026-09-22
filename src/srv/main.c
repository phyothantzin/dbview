#include "common.h"
#include "file.h"
#include "parse.h"
#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "srvpoll.h"

#define MAX_CLIENTS 256

clientstate_t clientStates[MAX_CLIENTS] = {0};

void print_usage(char *argv[]) {
  printf("Usage: %s -n -f <databse file>\n", argv[0]);
  printf("\t -n   - create new databse file\n");
  printf("\t -f   - (require) path to dababase file\n");
  return;
}

void poll_loop(unsigned short port, struct dbheader_t *header,
               struct employee_t *employees) {
  int listen_fd, conn_fd, freeSlot;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

  struct pollfd fds[MAX_CLIENTS + 1];
  int nfds = 1;
  int opt = 1;

  init_clients(clientStates);

  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (listen(listen_fd, 10) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  printf("Server listing on port %d\n", PORT);

  memset(fds, 0, sizeof(fds));
  fds[0].fd = listen_fd;
  fds[0].events = POLLIN;
  nfds = 1;

  while (1) {
    int ii = 1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clientStates[i].fd != -1) {
        fds[ii].fd = clientStates[i].fd;
        fds[ii].events = POLLIN;
        ii++;
      }
    }

    int n_events = poll(fds, nfds, -1);
    if (n_events == -1) {
      perror("poll");
      exit(EXIT_FAILURE);
    }

    if (fds[0].revents & POLLIN) {
      if ((conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr,
                            &client_len)) == -1) {
        perror("accept");
        continue;
      }

      printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr),
             ntohs(client_addr.sin_port));

      freeSlot = find_free_slot(clientStates);

      if (freeSlot == -1) {
        printf("Server full: closing new connections\n");
        close(conn_fd);
      } else {
        clientStates[freeSlot].fd = conn_fd;
        clientStates[freeSlot].state = STATE_HELLO;
        nfds++;
        printf("Slot %d has fd %d\n", freeSlot, clientStates[freeSlot].fd);
      }

      n_events--;
    }

    for (int i = 1; i <= nfds && n_events > 0; i++) {
      if (fds[i].revents & POLLIN) {
        n_events--;

        int fd = fds[i].fd;
        int slot = find_slot_by_fd(clientStates, fd);

        ssize_t bytes_read = read(fd, &clientStates[slot].buffer,
                                  sizeof(clientStates[slot].buffer));
        if (bytes_read <= 0) {
          close(fd);
          if (slot == -1) {
            printf("Tried to close fd that doesn't exist\n");
          } else {
            clientStates[slot].fd = -1;
            clientStates[slot].state = STATE_DISCONNECTED;
            printf("Client disconnected or error\n");
            nfds--;
          }
        } else {
          handle_client_fsm(header, employees, &clientStates[slot]);
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {
  int c;
  bool newfile = false;
  char *filepath = NULL;
  char *portarg = NULL;
  char *addString = NULL;
  int dbfd = -1;
  unsigned short port = 0;
  struct dbheader_t *header = NULL;
  struct employee_t *employees = NULL;
  bool list = false;

  while ((c = getopt(argc, argv, "nf:p:")) != -1) {
    switch (c) {
    case 'n':
      newfile = true;
      break;
    case 'f':
      filepath = optarg;
      break;
    case 'p':
      portarg = optarg;
      port = atoi(portarg);
      if (port == 0) {
        printf("Bad port %s\n", portarg);
      }
      break;
    case '?':
      printf("Unknown option -%c\n", c);
      break;
    default:
      return -1;
    }
  }

  if (filepath == NULL) {
    printf("filepath is the require argument\n");
    print_usage(argv);
    return 0;
  }

  if (newfile) {
    dbfd = create_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to create db file\n");
      return -1;
    }

    if (create_db_header(dbfd, &header) == STATUS_ERROR) {
      printf("Failed to create database header\n");
      return -1;
    };
  } else {
    dbfd = open_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to open db file\n");
      return -1;
    }

    if (validate_db_header(dbfd, &header) == STATUS_ERROR) {
      printf("Failed to validate database header\n");
      return -1;
    }
  }

  if (read_employees(dbfd, header, &employees) != STATUS_SUCCESS) {
    printf("Failed to read employees\n");
    return -1;
  };

  poll_loop(port, header, employees);

  output_file(dbfd, header, employees);

  return 0;
}
