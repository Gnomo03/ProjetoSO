#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>

#define FIFO_PATH "/tmp/server_fifo"
#define RESPONSE_FIFO_PATH "/tmp/response_fifo"
#define BUFFER_SIZE 1024

void send_command_to_server(const char *command);
void read_status();

#endif
