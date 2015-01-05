#include "fcgi_stdio.h"
#include <stdlib.h>
#include <string.h>

#define POTENTIALLY_PRIME 0
#define COMPOSITE 1
#define VALS_IN_SIEVE_TABLE 1000000
#define MAX_NUMBER_OF_PRIMES 78600

long int sieve_table[VALS_IN_SIEVE_TABLE];
long int prime_table[MAX_NUMBER_OF_PRIMES];

void initialize_prime_table(void) {
	long int prime_counter = 1;
	long int current_prime = 2, c, d;

	while (current_prime < VALS_IN_SIEVE_TABLE) {
		for (c = current_prime; c <= VALS_IN_SIEVE_TABLE; c += current_prime) {
			sieve_table[c] = COMPOSITE;
		}

		for (d = current_prime + 1; sieve_table[d] == COMPOSITE; d++) {
			prime_table[++prime_counter] = d;
			current_prime = d;
		}
	}
}

void main(void) {
	char* query_string;
	long int n;

	initialize_prime_table();

	while(FCGI_Accept() >= 0) {
		printf("Content-Type: text/html\r\n\r\n");

		printf("<title>Primes</title>");
		query_string = getenv("QUERY_STRING");
	}
}