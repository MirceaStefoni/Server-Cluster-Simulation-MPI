#ifndef __SERVER_H
#define __SERVER_H

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "commands.h"

void main_server(const char *command_file, int num_workers);

void worker_server(int rank);

void serial_server(const char *command_file);


#endif