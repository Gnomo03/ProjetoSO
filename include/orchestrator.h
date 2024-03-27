#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

void execute_command(const char *cmd);
void setup_fifo(const char *fifo_path);

#endif
