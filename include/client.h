#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>

#define FIFO_PATH "/tmp/server_fifo"
#define BUFFER_SIZE 1024

void send_command_to_server(const char *command);

#endif
