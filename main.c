#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "server.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                    MAIN                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0)
    {
        if (argc != 2)
        {
            fprintf(stderr, "Usage: %s <command_file>\n", argv[0]);
            MPI_Finalize();
            return EXIT_FAILURE;
        }
    }

    if (size < 2)
    {
        if (rank == 0)
        {
            fprintf(stdout, "Starting serial server with one process.\n");

            double start_time_serial = MPI_Wtime();

            serial_server(argv[1]);

            double end_time_serial = MPI_Wtime();

            // Open performance log file
            FILE *performance_log = fopen("PERFORMANCE_LOG.txt", "a");
            if (!performance_log)
            {
                perror("Error opening performance log file");
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            }

            fprintf(performance_log, "Serial Result: %g\n", end_time_serial - start_time_serial);

            fclose(performance_log);
        }
    }
    else
    {
        if (rank == 0)
        {
            fprintf(stdout, "Starting parallel server with %d processes.\n", size);

            double start_time_parallel = MPI_Wtime();

            main_server(argv[1], size - 1);

            double end_time_parallel = MPI_Wtime();

            // Open performance log file
            FILE *performance_log = fopen("PERFORMANCE_LOG.txt", "a");
            if (!performance_log)
            {
                perror("Error opening performance log file");
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            }

            fprintf(performance_log, "Parallel %d Result: %g\n", size, end_time_parallel - start_time_parallel);

            fclose(performance_log);

        }
        else
        {
            worker_server(rank);
        }
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}