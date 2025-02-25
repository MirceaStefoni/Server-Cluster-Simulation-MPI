#include "commands.h"

// Count the number of primes up to n
int count_primes(int n)
{
    int count = 0;
    for (int i = 2; i <= n; i++)
    {
        int is_prime = 1;
        for (int j = 2; j <= sqrt(i); j++)
        {
            if (i % j == 0)
            {
                is_prime = 0;
                break;
            }
        }
        if (is_prime)
            count++;
    }
    return count;
}

// Count the number of prime divisors of n
int count_prime_divisors(int n)
{
    int count = 0;
    for (int i = 2; i <= n; i++)
    {
        if (n % i == 0)
        {
            int is_prime = 1;
            for (int j = 2; j <= sqrt(i); j++)
            {
                if (i % j == 0)
                {
                    is_prime = 0;
                    break;
                }
            }
            if (is_prime)
                count++;
        }
    }
    return count;
}

// Swap two characters in a string
void swap(char *a, char *b)
{
    char temp = *a;
    *a = *b;
    *b = temp;
}

// Helper function to generate permutations
void generate_permutations(char *str, int start, int end, FILE *output, bool *has_written)
{
    if (start == end)
    {
        fprintf(output, "%s\n", str);
        *has_written = true;
        return;
    }
    for (int i = start; i <= end; i++)
    {
        swap(&str[start], &str[i]);
        generate_permutations(str, start + 1, end, output, has_written);
        swap(&str[start], &str[i]); // backtrack
    }
}

// Function to generate all anagrams
void generate_anagrams(const char *name, FILE *output)
{
    char temp[strlen(name) + 1];
    strcpy(temp, name);

    bool has_written = false; // To check if at least one permutation is written
    fprintf(output, "Anagrams of %s:\n", name);
    generate_permutations(temp, 0, strlen(temp) - 1, output, &has_written);

    if (!has_written)
    {
        fprintf(output, "No anagrams generated.\n");
    }
}

void log_server_event(FILE *server_log, const char *event, const char *client_id, const char *command, const char *worker_id)
{
    char timestamp[64];
    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    if (worker_id)
    {
        fprintf(server_log, "[%s] %s | Client: %s | Command: %s | Worker: %s\n", timestamp, event, client_id, command, worker_id);
    }
    else
    {
        fprintf(server_log, "[%s] %s | Client: %s | Command: %s\n", timestamp, event, client_id, command);
    }
    fflush(server_log); // Ensure the log is written immediately
}