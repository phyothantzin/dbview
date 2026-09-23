#include <arpa/inet.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "common.h"
#include "srvpoll.h"

void fsm_reply_hello(clientstate_t *client, dbproto_hdr_t *header) {
  header->type = htonl(MSG_HELLO_RES);
  header->len = htons(1);
  dbproto_hello_res *hello = (dbproto_hello_res *)&header[1];
  hello->proto = htons(PROTO_VER);

  write(client->fd, header, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_res));
}

void fsm_reply_hello_err(clientstate_t *client, dbproto_hdr_t *header) {
  header->type = htonl(MSG_ERROR);
  header->len = htons(0);

  write(client->fd, header, sizeof(dbproto_hdr_t));
}

void fsm_reply_add(clientstate_t *client, dbproto_hdr_t *header) {
  header->type = htonl(MSG_EMPLOYEE_ADD_RES);
  header->len = htons(1);

  write(client->fd, header, sizeof(dbproto_hdr_t));
}

void fsm_reply_add_err(clientstate_t *client, dbproto_hdr_t *header) {
  header->type = htonl(MSG_ERROR);
  header->len = htons(0);

  write(client->fd, header, sizeof(dbproto_hdr_t));
}

void send_employees(struct dbheader_t *header, struct employee_t **employees,
                    clientstate_t *client) {
  dbproto_hdr_t *hdr = (dbproto_hdr_t *)client->buffer;
  hdr->type = htonl(MSG_EMPLOYEE_LIST_RES);
  hdr->len = htons(header->count);

  write(client->fd, hdr, sizeof(dbproto_hdr_t));

  dbproto_employee_list_res *employee = (dbproto_employee_list_res *)&hdr[1];

  struct employee_t *employees_ptr = *employees;

  int i = 0;
  for (; i < header->count; i++) {
    strncpy(&employee->name, employees_ptr[i].name, sizeof(employee->name));
    strncpy(&employee->address, employees_ptr[i].address,
            sizeof(employee->address));
    employee->hours = htonl(employees_ptr[i].hours);
    write(client->fd, employee, sizeof(dbproto_employee_list_res));
  }
}

void handle_client_fsm(struct dbheader_t *header, struct employee_t **employees,
                       clientstate_t *client, int dbfd) {
  dbproto_hdr_t *hdr = (dbproto_hdr_t *)client->buffer;

  hdr->type = ntohl(hdr->type);
  hdr->len = ntohs(hdr->len);

  if (client->state == STATE_HELLO) {
    printf("Client state is STATE_HELLO\n");
    if (hdr->type != MSG_HELLO_REQ || hdr->len != 1) {
      printf("Didn't get the MES_HELLO in Hello state\n");
    }

    dbproto_hello_req *hello = (dbproto_hello_req *)&hdr[1];
    hello->proto = ntohs(hello->proto);
    if (hello->proto != PROTO_VER) {
      printf("Protocol version mismatch...\n");
      fsm_reply_hello_err(client, hdr);
      return;
    }

    fsm_reply_hello(client, hdr);
    client->state = STATE_MSG;
    printf("Client upgraded to STATE_MSG\n");
  }

  if (client->state == STATE_MSG) {
    if (hdr->type == MSG_EMPLOYEE_ADD_REQ) {
      dbproto_add_employee_req *employee = (dbproto_add_employee_req *)&hdr[1];

      printf("Adding employee: %s\n", employee->data);

      if (add_employee(header, employees, employee->data) != STATUS_SUCCESS) {
        fsm_reply_add_err(client, hdr);
        printf("Employee add error\n");
        return;
      } else {
        printf("Employee added success, now outputting\n");
        fsm_reply_add(client, hdr);
        output_file(dbfd, header, *employees);
        list_employees(header, *employees);
      }
    }

    if (hdr->type == MSG_EMPLOYEE_LIST_REQ) {
      printf("Listing employees\n");
      send_employees(header, employees, client);
    }
  }
}

void init_clients(clientstate_t *states) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    states[i].fd = -1;
    states[i].state = STATE_NEW;
    memset(&states[i].buffer, '\0', BUFF_SIZE);
  }
}

int find_free_slot(clientstate_t *states) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (states[i].fd == -1) {
      return i;
    }
  }

  return -1;
}

int find_slot_by_fd(clientstate_t *states, int fd) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (states[i].fd == fd) {
      return i;
    }
  }

  return -1;
}
