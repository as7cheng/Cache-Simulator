/* Name: Shihan Cheng
 * CS login: shihan
 * Section(s): 001
 *
 * csim.c - A cache simulator that can replay traces from Valgrind
 *     and output statistics such as number of hits, misses, and
 *     evictions.  The replacement policy is LRU.
 *
 * Implementation and assumptions:
 *  1. Each load/store can cause at most one cache miss plus a possible eviction.
 *  2. Instruction loads (I) are ignored.
 *  3. Data modify (M) is treated as a load followed by a store to the same
 *  address. Hence, an M operation can result in two cache hits, or a miss and a
 *  hit plus a possible eviction.
 *
 * The function print_summary() is given to print output.
 * Please use this function to print the number of hits, misses and evictions.
 * This is crucial for the driver to evaluate your work.
 */

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/****************************************************************************/
/***** DO NOT MODIFY THESE VARIABLE NAMES ***********************************/

/* Globals set by command line args */
int s = 0; /* set index bits */
int E = 0; /* associativity */
int b = 0; /* block offset bits */
int verbosity = 0; /* print trace if set */
char* trace_file = NULL;

/* Derived from command line args */
int B; /* block size (bytes) B = 2^b */
int S; /* number of sets S = 2^s In C, you can use the left shift operator */

/* Counters used to record cache statistics */
int hit_cnt = 0;
int miss_cnt = 0;
int evict_cnt = 0;

/*****************************************************************************/


/* Type: Memory address
 * Use this type whenever dealing with addresses or address masks
 */
typedef unsigned long long int mem_addr_t;

/* Type: Cache line
 * TODO
 *
 * NOTE:
 * You might (not necessarily though) want to add an extra field to this struct
 * depending on your implementation
 *
 * For example, to use a linked list based LRU,
 * you might want to have a field "struct cache_line * next" in the struct
 */
typedef struct cache_line {
    char valid;
    mem_addr_t tag;
    long long count;
    // struct cache_line * next;
} cache_line_t;

typedef cache_line_t* cache_set_t;
typedef cache_set_t* cache_t;

/* The cache we are simulating */
cache_t cache;

/* This counter is for replacement policy LRU*/
long long counter = 1;

/* Build 2 masks for extracting s-bits and t-bits */
mem_addr_t s_Mask = 0;
mem_addr_t t_Mask = 0;

/* init_cache -
 * Allocate data structures to hold info regrading the sets and cache lines
 * use struct "cache_line_t" here
 * Initialize valid and tag field with 0s.
 * use S (= 2^s) and E while allocating the data structures here
 */
void init_cache() {
    // Initialize S and B
    S = pow(2, s);
    B = pow(2, b);
    
    // Initialization of s_Mask and t_Mask
    int power = B;
    // Set all of s-bits as 1
    // Set other bits as 0 in mask with for loop
    for (int i = 0 ; i < s; i++) {
        s_Mask += power;
        power *= 2;
    }
    // Set all of t-bits as 1
    // Set b-bits and s-bits as 0 in mask
    t_Mask -= pow(2, (b + s));
    // Alocate spaces for cache
    cache = malloc(sizeof(cache_set_t) * S);
    // Check the returned value of malloc()
    if (cache == NULL) {
        // If it is NULL, exsit.
        exit(1);
    }
    // Build the spaces of lines for every set by this for loop
    for (int i = 0; i < S; i++) {
        *(cache + i) = malloc(sizeof(cache_line_t) * E);
        // Check the returned value of malloc()
        if (*(cache + i) == NULL){
            exit(1);
        }
        // Then initialize the cache blocks for every line
        for (int j = 0; j < E; j++) {
            (*(cache + i) + j)->valid = '0';
            (*(cache + i) + j)->tag = 0;
            (*(cache + i) + j)->count = 0;
        }
    }
}


/* free_cache - free each piece of memory you allocated using malloc
 * inside init_cache() function
 */
void free_cache() {
    // Freeing set by set
    for (int i = 0; i < S; i++) {
        free(*(cache + i));
        // Set pointer as NULL after freeing
        *(cache + i) = NULL;
    }
    // Free the cache
    free(cache);
    // Set pointer as NULL after freeing
    cache = NULL;
}

/* access_data - Access data at memory address addr.
 *   If it is already in cache, increase hit_cnt
 *   If it is not in cache, bring it in cache, increase miss count.
 *   Also increase evict_cnt if a line is evicted.
 *   you will manipulate data structures allocated in init_cache() here
 */
void access_data(mem_addr_t addr) {
    // This is the target set
    mem_addr_t set = (addr & s_Mask) / B;
    // This is the tag of memory
    mem_addr_t tag = (addr & t_Mask) / pow(2, (b+s));
    
    // Check if the cache is empty
    if (cache == NULL) {
        exit(1);
    }
    // Check if the set is empty
    if (*(cache + set) == NULL) {
        exit(1);
    }
    // Using for count lines
    int E_line;
    // Create a hit flag
    bool hit = false;
    // Track the cache line by line
    for (E_line = 0; E_line < E; E_line++) {
        if ( (*(cache + set) + E_line)->tag == tag &&
            (*(cache + set) + E_line)->valid == '1') {
            hit_cnt++;
            // Set hit flag to true
            hit = true;
            break;
        }
    }
    // When cache hit, update LRU counter and return
    if (hit == true) {
        (*(cache + set) + E_line)->count = counter;
        counter++;
        return;
    }
    
    // When cache miss
    miss_cnt++; // Update miss_cnt
    // Initialize the unused lines as the lines
    int unused_line = E;
    // Initialize the evicted lines as 0
    int evict_line = 0;
    // Set the counter as the largest int
    long long min = 2147483647;
    bool unused = false;
    // Trace cache set line by line
    for (int E_line = 0; E_line < E; E_line++) {
        // If the unused line found
        if ( (*(cache + set) + E_line)->valid == '0') {
            unused = true;
            unused_line = E_line;
            break;
        }
        // If the LRU line found
        if ((*(cache + set) + E_line)->count < min) {
            // Set the min as the line's counter
            min = (*(cache + set) + E_line)->count;
            // Then the evicted line is the line
            evict_line = E_line;
        }
    }
    // If there is an unused line
    if (unused == true) {
        // Set the tag
        (*(cache + set) + unused_line)->tag = tag;
        // Set its v-bit as 1
        (*(cache + set) + unused_line)->valid = '1';
        // Set its count
        (*(cache + set) + unused_line)->count = counter;
        // Update the counter and return
        counter++;
        return;
    }
    // If no unused line exsits, then a victim line is needed
    evict_cnt++; // Update evict_cnt
    // Set its tag
    (*(cache + set) + evict_line)->tag = tag;
    // Set its v-bit as 1
    (*(cache + set) + evict_line)->valid = '1';
    // Set its count
    (*(cache + set) + evict_line)->count = counter;
    // Update counter and return
    counter++;
    return;
}

/* replay_trace - replays the given trace file against the cache
 * reads the input trace file line by line
 * extracts the type of each memory access : L/S/M
 * one "L" as a load i.e. 1 memory access
 * one "S" as a store i.e. 1 memory access
 * one "M" as a load followed by a store i.e. 2 memory accesses
 */
void replay_trace(char* trace_fn) {
    char buf[1000];
    mem_addr_t addr = 0;
    unsigned int len = 0;
    FILE* trace_fp = fopen(trace_fn, "r");
    
    if (!trace_fp) {
        fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno));
        exit(1);
    }
    
    while (fgets(buf, 1000, trace_fp) != NULL) {
        if (buf[1] == 'S' || buf[1] == 'L' || buf[1] == 'M') {
            sscanf(buf+3, "%llx,%u", &addr, &len);
            
            if (verbosity)
                printf("%c %llx,%u ", buf[1], addr, len);
            
            // 1. address accessed in variable - addr
            // 2. type of acccess(S/L/M)  in variable - buf[1]
            // call access_data function here depending on type of access
            if (buf[1] == 'S') {
                access_data(addr);
            }
            if (buf[1] == 'L') {
                access_data(addr);
            }
            if (buf[1] == 'M') {
                access_data(addr);
                access_data(addr);
            }
            
            if (verbosity)
                printf("\n");
        }
    }
    
    fclose(trace_fp);
}

/*
 * print_usage - Print usage info
 */
void print_usage(char* argv[]) {
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}

/*
 * print_summary - Summarize the cache simulation statistics. Student cache simulators
 *                must call this function in order to be properly autograded.
 */
void print_summary(int hits, int misses, int evictions) {
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    FILE* output_fp = fopen(".csim_results", "w");
    assert(output_fp);
    fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
    fclose(output_fp);
}

/*
 * main - Main routine
 */
int main(int argc, char* argv[]) {
    // Parse the command line arguments: -h, -v, -s, -E, -b, -t
    char c;
    while ((c = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
        switch (c) {
            case 'b':
                b = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'h':
                print_usage(argv);
                exit(0);
            case 's':
                s = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            case 'v':
                verbosity = 1;
                break;
            default:
                print_usage(argv);
                exit(1);
        }
    }
    
    /* Make sure that all required command line args were specified */
    if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        print_usage(argv);
        exit(1);
    }
    
    /* Initialize cache */
    init_cache();
    
    replay_trace(trace_file);
    
    /* Free allocated memory */
    free_cache();
    
    /* Output the hit and miss statistics for the autograder */
    print_summary(hit_cnt, miss_cnt, evict_cnt);
    
    return 0;
}


