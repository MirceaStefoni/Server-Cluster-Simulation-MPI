#ifndef __COMMANDS_H
#define __COMMANDS_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

int count_primes(int n);

int count_prime_divisors(int n);

void swap(char *a, char *b);

void generate_permutations(char *str, int start, int end, FILE *output, bool *has_written);

void generate_anagrams(const char *name, FILE *output);

void log_server_event(FILE *server_log, const char *event, const char *client_id, const char *command, const char *worker_id);

#endif