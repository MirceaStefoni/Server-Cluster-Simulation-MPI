#include "server.h"


// Define Command tags
#define CMD_PRIMES 1
#define CMD_PRIMEDIVISORS 2
#define CMD_ANAGRAMS 3
#define CMD_MATRIXADD 4
#define CMD_MATRIXMULT 5
#define CMD_TERMINATE 99

// Define Maximum limits
#define MAX_WORKERS 32
#define MAX_COMMANDS 32
#define MAX_LINE_LENGTH 256  // 512


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                MAIN SERVER                                                       //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void main_server(const char *command_file, int num_workers)
{
    // Sanity checks
    if (num_workers > MAX_WORKERS) {
        fprintf(stderr, "Number of workers exceeds maximum limit\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Open command file
    FILE *file = fopen(command_file, "r");
    if (!file)
    {
        perror("Error opening command file");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Open server log file
    FILE *server_log = fopen("SERVER_LOG.txt", "w");    // Write or Append
    if (!server_log)
    {
        perror("Error opening server log file");
        fclose(file);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Static arrays for managing tasks and workers
    int worker_status[MAX_WORKERS] = {0}; // 0 = free, 1 = busy
    MPI_Request send_cmd_requests[MAX_WORKERS];
    MPI_Request send_param_requests[MAX_WORKERS];
    MPI_Request recv_requests[MAX_WORKERS];
    char recv_buffers[MAX_WORKERS][MAX_LINE_LENGTH];   // result
    char client_ids[MAX_WORKERS][16];
    char worker_commands[MAX_WORKERS][32];
    char worker_params[MAX_WORKERS][64];

    // Initialize requests
    for (int i = 0; i < num_workers; i++) {
        recv_requests[i] = MPI_REQUEST_NULL;
        send_cmd_requests[i] = MPI_REQUEST_NULL;
        send_param_requests[i] = MPI_REQUEST_NULL;
    }

    // Static array for commands
    char commands[MAX_COMMANDS][MAX_LINE_LENGTH];
    int total_commands = 0;
    char line[MAX_LINE_LENGTH];

    // Read all commands first
    while (fgets(line, sizeof(line), file) && total_commands < MAX_COMMANDS)
    {
        // Skip empty lines
        if (strlen(line) <= 1) continue;

        // Copy command to commands array
        strcpy(commands[total_commands], line);
        total_commands++;
    }

    // Check if we've exceeded command capacity
    if (total_commands >= MAX_COMMANDS) {
        fprintf(stderr, "Warning: Maximum number of commands exceeded\n");
    }

    int commands_sent = 0;
    int commands_completed = 0;

    // Main processing loop
    while (commands_completed < total_commands)
    {
        // Check for completed receives and process them
        for (int worker = 0; worker < num_workers; worker++)
        {
            if (recv_requests[worker] != MPI_REQUEST_NULL)
            {
                int flag = 0;
                MPI_Status status;
                MPI_Test(&recv_requests[worker], &flag, &status);
                
                if (flag) 
                {
                    // Result received, log and write to client log
                    char log_filename[32];
                    snprintf(log_filename, sizeof(log_filename), "%s_LOG.txt", client_ids[worker]);
                    
                    FILE *log_file = fopen(log_filename, "w");  // Write or Append
                    if (log_file) 
                    {
                        fprintf(log_file, "Result: %s\n", recv_buffers[worker]);
                        fclose(log_file);
                    }

                    // Log completion
                    char worker_id[16];
                    snprintf(worker_id, sizeof(worker_id), "Worker %d", worker + 1);
                    log_server_event(server_log, "COMPLETED", client_ids[worker], worker_commands[worker], worker_id);

                    // Mark worker as free
                    worker_status[worker] = 0;
                    commands_completed++;
                    recv_requests[worker] = MPI_REQUEST_NULL;
                }
            }

            // If worker is free and we have more commands to send
            if (worker_status[worker] == 0 && commands_sent < total_commands)
            {
                // Parse command
                char client_id[16], command[32], param[64];
                sscanf(commands[commands_sent], "%s %s %s", client_id, command, param);

                // Determine command type
                int cmd_type;
                if (strcmp(command, "PRIMES") == 0)
                    cmd_type = CMD_PRIMES;
                else if (strcmp(command, "PRIMEDIVISORS") == 0)
                    cmd_type = CMD_PRIMEDIVISORS;
                else if (strcmp(command, "ANAGRAMS") == 0)
                    cmd_type = CMD_ANAGRAMS;
                else
                {
                    fprintf(stderr, "Unknown command: %s\n", command);
                    commands_sent++;
                    continue;
                }

                // Log received command
                log_server_event(server_log, "RECEIVED", client_id, command, NULL);

                // Prepare for sending and receiving
                int worker_rank = worker + 1; // Workers start at rank 1

                // Send command type
                MPI_Isend(&cmd_type, 1, MPI_INT, worker_rank, 0, MPI_COMM_WORLD, &send_cmd_requests[worker]);

                // Send parameter
                MPI_Isend(param, strlen(param) + 1, MPI_CHAR, worker_rank, 0, MPI_COMM_WORLD, &send_param_requests[worker]);

                // Log dispatched command
                char worker_id[16];
                snprintf(worker_id, sizeof(worker_id), "Worker %d", worker_rank);
                log_server_event(server_log, "DISPATCHED", client_id, command, worker_id);

                // Prepare to receive result
                MPI_Irecv(recv_buffers[worker], MAX_LINE_LENGTH, MPI_CHAR, worker_rank, 0, MPI_COMM_WORLD, &recv_requests[worker]);

                // Store client ID and command for logging
                strcpy(client_ids[worker], client_id);
                strcpy(worker_commands[worker], command);
                strcpy(worker_params[worker], param);

                // Mark worker as busy
                worker_status[worker] = 1;
                commands_sent++;
            }
        }
    }

    // Terminate workers
    for (int i = 1; i <= num_workers; i++)
    {
        int cmd_type = CMD_TERMINATE;
        MPI_Send(&cmd_type, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }

    fclose(file);
    fclose(server_log);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                               WORKER SERVERS                                                     //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void worker_server(int rank)
{
    while (1)
    {
        int cmd_type;
        MPI_Recv(&cmd_type, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (cmd_type == CMD_TERMINATE)
        {
            break;
        }

        char param[64];
        MPI_Recv(param, sizeof(param), MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        char result[256];
        if (cmd_type == CMD_PRIMES)
        {
            int n = atoi(param);
            int count = count_primes(n);
            snprintf(result, sizeof(result), "Primes up to %d: %d", n, count);
        }
        else if (cmd_type == CMD_PRIMEDIVISORS)
        {
            int n = atoi(param);
            int count = count_prime_divisors(n);
            snprintf(result, sizeof(result), "Prime divisors of %d: %d", n, count);
        }
        else if (cmd_type == CMD_ANAGRAMS) // can only generate 256 chars
        {
            FILE *output = tmpfile();
            generate_anagrams(param, output);
            fseek(output, 0, SEEK_SET);
            size_t read_bytes = fread(result, 1, sizeof(result) - 1, output);
            result[read_bytes] = '\0'; // Null-terminate the string
            fclose(output);
        }
        else
        {
            snprintf(result, sizeof(result), "Unknown command");
        }

        MPI_Send(result, strlen(result) + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                               SERIAL SERVER                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void serial_server(const char* command_file)
{
    FILE *file = fopen(command_file, "r");
    if (!file)
    {
        perror("Error opening command file");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    FILE *server_log = fopen("SERVER_SERIAL_LOG.txt", "w"); // Write or Append
    if (!server_log)
    {
        perror("Error opening server log file");
        fclose(file);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "WAIT", 4) == 0)
        {
            int wait_time;
            sscanf(line, "WAIT %d", &wait_time);
            sleep(wait_time);
            continue;
        }

        char client_id[16], command[32], param[64];
        sscanf(line, "%s %s %s", client_id, command, param);

        // Log when command is received
        log_server_event(server_log, "RECEIVED", client_id, command, NULL);

        // Log when command is dispatched
        char worker_id[16];
        snprintf(worker_id, sizeof(worker_id), "Worker Serial");
        log_server_event(server_log, "DISPATCHED", client_id, command, worker_id);


        char result[MAX_LINE_LENGTH];

        if (strcmp(command, "PRIMES") == 0)
        {
            int n = atoi(param);
            int count = count_primes(n);
            snprintf(result, sizeof(result), "Primes up to %d: %d", n, count);
        }
        else if (strcmp(command, "PRIMEDIVISORS") == 0)
        {
            int n = atoi(param);
            int count = count_prime_divisors(n);
            snprintf(result, sizeof(result), "Prime divisors of %d: %d", n, count);
        }
        else if (strcmp(command, "ANAGRAMS") == 0)
        {
            FILE *output = tmpfile();
            generate_anagrams(param, output);
            fseek(output, 0, SEEK_SET);
            size_t read_bytes = fread(result, 1, sizeof(result) - 1, output);
            result[read_bytes] = '\0'; // Null-terminate the string
            fclose(output);
        }
        else
        {
            fprintf(stderr, "Unknown command: %s\n", command);
            continue;
        }

        // Log when command is completed
        log_server_event(server_log, "COMPLETED", client_id, command, worker_id);

        // Write the result to the appropriate client's log file
        char log_filename[32];
        snprintf(log_filename, sizeof(log_filename), "%s_SERIAL_LOG.txt", client_id);
        FILE *log_file = fopen(log_filename, "w"); // write sau append
        if (!log_file)
        {
            perror("Error opening log file");
        }
        else
        {
            fprintf(log_file, "Result: %s\n", result);
            fclose(log_file);
        }
    }

    fclose(file);
    fclose(server_log);
}