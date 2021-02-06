#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define BLU "\x1B[34m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define PUR "\x1B[35m"
#define RED "\x1B[31m"
#define END "\x1B[0m"

#define MAX 1048576 /* The maximum number of bytes in virtual memory */
#define MAX_LINE 80 /* The maximum length command */

int bytes = 0; /* The total number of bytes requested by the user */
int allocated = 0; /* The total number of bytes in use by processes */

bool shouldrun = true; /* boolean to determine when the user quits */
bool debug = false; /* boolean to determine whether or not to print info */

typedef struct process {
    char *name; /* name of the process (i.e. P0) */
    int size; /* size to allocate in bytes */
    int start; /* start address in virtual memory */
    int end; /* end address in virtual memory */
} process;

typedef struct name {
    struct name *next; /* pointer to the next name in the list */
    struct name *prev; /* pointer to the previous name in the list */
    int processNum; /* the process' number (i.e. 10 given P10) */
} name;

typedef struct node {
    struct node *next; /* pointer to the next node in the list */
    struct node *prev; /* pointer to the previous node in the list */
    struct process *process; /* pointer to the process belonging to the node */
    bool hole; /* flag to determine if node's process is a hole */
} node;

struct node *head = NULL; /* head of the doubly linked list of processes */
struct node *tail = NULL; /* tail of the doubly linked list of processes */
struct name *namehead = NULL; /* head of the doubly linked list of names */

node *temptail = NULL;
node *temphead = NULL;

/* creates and returns a process, initializing name and size */
struct process * createProcess(char *name, int size); 

/* releases a process from memory, if present, creating a hole */
int releaseProcess(char *name);

/* creates a hole in memory and merges said hole with surrounding holes */
int makeProcessHole(char *name);

/* creates a node given a process & start & end addresses */
void createNode(struct process *p, int start, int end);

/* prints the doubly linked list */
void printLinkedList();

/* frees the doubly linked list from memory */
void freeLinkedList();

/* locates and returns a process in the doubly linked list- 
   returns null if process is not in the list */
struct node *locateProcess(char *name);

/* allocates a process in memory according to Worst Fit, Best Fit,
   or First Fit algorithm (indicated by the flag) only if there is
   a hole large enough for the requested allocation size */
int allocateProcess(char *command, char flag);

/* searches list of process names for duplicates and returns
   true if duplicate found, false if not */
bool duplicate();

/* negates a processes' name if a memory size is caught, so
   the user can reuse the unsuccessful process' name again */
void negateProcess(char *name);

/* combines two adjacent holes into one node */
void combineHoles(struct node *b, struct node *a);

/* combines three adjacent holes into one node */
void combineThreeHoles(struct node *a, struct node *b, struct node *c);

/* creates and returns a hole node */
struct node * createHole(struct process *p, int start, int end);

/* allocates a process into a hole that was previously a process */
void allocateProcessIntoHole(struct node *holeNode, struct process *processNode);

/* print the fields of a given node */
void printNode(node *n);

/* print the names of processes previously allocated */
void printNames();

/* free the doubly linked list holding names of processes */
void freeNames();

/* add a name to the doubly linked list holding names of processes */
void addName(char *n);

/* allocates a given process into the largest hole in memory */
void worstFit(struct process *p);

/* allocates a given process into the smallest hole in memory
   that is large enough for said process */
void bestFit(struct process *p);

/* allocate a given process into the first hole in memory that is
   large enough (first meaning starting from address 0) */
void firstFit(struct process *p);

/* reports the status of memory */
void stat();

/* compacts all holes into one hole and places all processes
   adjacent to each other */
void compact();

/* printing for error handling */
void printRequestError();
void printReleaseError(int howMany);
void noMemoryLeft(char *name);

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf(RED "\nPlease enter a positive number of bytes to be allocated.\n\n" END);
        return -1;
    }

    bytes = atoi(argv[1]);

    if (bytes <= 0) {
        printf(RED "\nPlease enter a positive number of bytes to be allocated.\n\n" END);
        return -1;
    } else if (bytes > MAX) {
        printf(RED "\nPlease enter a positive number of bytes less than or equal to %d.\n\n" END, MAX);
        return -1;
    }

    if (debug) {
        printf("\nMaximum number of bytes: %d\n\n", bytes);
    }

    while (shouldrun) {
        printf(BLU "allocator" END "$ ");
        fflush(stdout);

        char command[MAX_LINE];
        fgets(command, MAX_LINE, stdin);

        if (strcmp(command, "X\n") == 0 || strcmp(command, "q\n") == 0) {
            shouldrun = 0; /* exit */
            
        } else if (command[strlen(command) - 2] == 'F') {
            allocateProcess(command, 'F'); /* First fit */

        } else if (command[strlen(command) - 2] == 'B') {
            allocateProcess(command, 'B'); /* Best fit */

        } else if (command[strlen(command) - 2] == 'W') {
            allocateProcess(command, 'W'); /* Worst fit */

        } else if (strcmp(command, "C\n") == 0) {
            compact();

        } else if (strcmp(command, "STAT\n") == 0) {
            stat();

        } else {
            releaseProcess(command);

        }
    }

    freeNames();
    freeLinkedList();

    return 0;
}

void stat() {

    node *n;
    if (head) {
        printf("\n");
        for (n = tail; n != NULL; n = n->prev) {
            if (n->hole) {
                printf("Addresses [%d:%d] " RED "Unused\n" END, n->process->start, n->process->end);
            } else {
                printf("Addresses [%d:%d] " BLU "Process %s\n" END, n->process->start, n->process->end, n->process->name);
            }
        }
        printf("\n");
    } else {
        printf("\nAddresses [%d:%d] " RED "Unused\n\n" END, 0, bytes - 1);
    }
}

void compactProcess(struct process *p, bool isHole) {

    struct node *new = (struct node *) malloc(sizeof(struct node));
    struct process *newprocess = createProcess(p->name, p->size);
    new->process = newprocess;

    if (isHole) {
        new->hole = true;
    } else {
        new->hole = false;
    }

    new->process->start = temphead->process->end + 1;
    new->process->end = new->process->size + new->process->start - 1;

    new->next = temphead;
    new->prev = NULL;

    temphead->prev = new;
    temphead = new;
}

void compact() {

    printf("\nCompacting all free memory together... ");

    int freeBytes = 0;

    temptail = NULL;
    temphead = NULL;

    node *n;
    for (n = tail; n != NULL; n = n->prev) {
        if (n->hole) {
            freeBytes += n->process->size;
        } else {
            if (!temptail) {

                struct node *new = (struct node *) malloc(sizeof(struct node));
                struct process *newprocess = createProcess(n->process->name, n->process->size);
                new->process = newprocess;
                new->hole = false;

                temptail = new;
                temphead = new;
                new->next = NULL;
                new->prev = NULL;
                new->process->start = 0;
                new->process->end = n->process->size - 1;
                new->process->size = n->process->size;
                new->process->name = n->process->name;

            } else {
                compactProcess(n->process, false);
            }
        }
    }

    freeLinkedList();

    if (freeBytes > 0) {
        struct process *hole = createProcess("hole", freeBytes);
        if (debug) {
            printf("Free bytes: %d\n", freeBytes);
            printf("Hole start: %d\n", temphead->process->end + 1);
            printf("Hole end: %d\n", bytes - 1);
        }
        struct node *h = createHole(hole, temphead->process->end + 1, bytes - 1);
        compactProcess(h->process, true);
        head = temphead;
        tail = temptail;
    }

   printf(GRN "compacted.\n\n" END);

}

void worstFit(struct process *p /* a process with only a name and size */) {

    if (!head) {
        if (p->size < bytes) {
            createNode(p, 0, p->size - 1);
            struct process *hole = createProcess("hole", bytes - p->size);
            struct node *h = createHole(hole, p->size, bytes - 1);
            struct node *prc = locateProcess(p->name);
            h->next = prc;
            prc->prev = h;
            head = h;
        } else if (p->size == bytes) {
            createNode(p, 0, p->size - 1);
        } else {
            noMemoryLeft(p->name);
            negateProcess(p->name);
            return;
        }
    } else {

        /* find the largest hole that is big enough and allocate */
        node *n;
        node *largest = NULL;
        for (n = tail; n != NULL; n = n->prev) {
            if (n->hole) {
                if (n->process->size >= p->size) {
                    if (largest) {
                        if (n->process->size > largest->process->size) {
                            largest = n;
                        }
                    } else {
                        largest = n;
                    }
                }
            }
        }

        if (largest) {
            allocateProcessIntoHole(largest, p);
        } else {
            noMemoryLeft(p->name);
            negateProcess(p->name);
            return;
        }
    }
    
    allocated += p->size;
    printf(GRN "\nProcess %s created with %d bytes allocated.\n\n" END, p->name, p->size);

    if (debug) {
        printf(PUR "%d bytes allocated so far.\n\n" END, allocated);
    }
}

void bestFit(struct process *p /* a process with only a name and size */) {

    if (!head) {
        if (p->size < bytes) {
            createNode(p, 0, p->size - 1);
            struct process *hole = createProcess("hole", bytes - p->size);
            struct node *h = createHole(hole, p->size, bytes - 1);
            struct node *prc = locateProcess(p->name);
            h->next = prc;
            prc->prev = h;
            head = h;
        } else if (p->size == bytes) {
            createNode(p, 0, p->size - 1);
        } else {
            noMemoryLeft(p->name);
            negateProcess(p->name);
            return;
        }
    } else {
        
        /* find the smallest hole that is big enough and allocate */
        node *n;
        node *smallest = NULL;
        for (n = tail; n != NULL; n = n->prev) {
            if (n->hole) {
                if (n->process->size >= p->size) {
                    if (smallest) {
                        if (n->process->size < smallest->process->size) {
                            smallest = n;
                        }
                    } else {
                        smallest = n;
                    }
                }
            }
        }

        if (smallest) {
            allocateProcessIntoHole(smallest, p);
        } else {
            noMemoryLeft(p->name);
            negateProcess(p->name);
            return;
        }
    }
    
    allocated += p->size;
    printf(GRN "\nProcess %s created with %d bytes allocated.\n\n" END, p->name, p->size);

    if (debug) {
        printf(PUR "%d bytes allocated so far.\n\n" END, allocated);
    }
}

void firstFit(struct process *p /* a process with only a name and size */) {
    
    if (!head) {
        if (p->size < bytes) {
            createNode(p, 0, p->size - 1);
            struct process *hole = createProcess("hole", bytes - p->size);
            struct node *h = createHole(hole, p->size, bytes - 1);
            struct node *prc = locateProcess(p->name);
            h->next = prc;
            prc->prev = h;
            head = h;
        } else if (p->size == bytes) {
            createNode(p, 0, p->size - 1);
        } else {
            noMemoryLeft(p->name);
            negateProcess(p->name);
            return;
        }
    } else {
        node *n;
        for (n = tail; n != NULL; n = n->prev) {

            /* if we reach the head, we've run out of holes prior to 
               the highest allocated process in memory */
            
            if (n == head) {
                if (n->hole) {
                    if (n->process->size >= p->size) {
                        allocateProcessIntoHole(n, p);
                        break;
                    }
                } else {
                    noMemoryLeft(p->name);
                    negateProcess(p->name);
                    return;
                }
            } else {
                if (n->hole) {
                    if (n->process->size >= p->size) {
                        allocateProcessIntoHole(n, p);
                        break;
                    }
                }
            }
        }
    }

    allocated += p->size;
    printf(GRN "\nProcess %s created with %d bytes allocated.\n\n" END, p->name, p->size);

    if (debug) {
        printf(PUR "%d bytes allocated so far.\n\n" END, allocated);
    }
}

int allocateProcess(char *command, char flag) {

    char **parsed = malloc(sizeof(char *) * 4);
    char *space = strtok(command, " ");

    int i = 0;
    while (space) {
        if (i > 3) {
            printRequestError();
            return -1;
        }
        parsed[i] = space;
        i++;
        space = strtok(NULL, " ");
    }

    if (strcmp(parsed[0], "RQ") != 0) {
        printf(RED "\nPlease request allocation using the \"RQ\" command.\n\n" END);
        return -1;
    } else if (atoi(parsed[2]) <= 0) {
        printf(RED "\nPlease enter a valid positive number of bytes.\n\n" END);
        return -1;
    }

    struct process *p;
    p = (struct process *) malloc(sizeof(struct process));

    bool dup = duplicate(parsed[1]);

    if (dup) {
        printf(RED "\nThe process name %s has already been used.\n", parsed[1]);
        printf("Please choose a different name.\n\n" END);
        return -1;
    }

    addName(parsed[1]);

    if (debug) {
        printNames();
    }
    
    p->name = (char *) malloc(sizeof(char *));
    strcpy(p->name, parsed[1]);

    p->size = atoi(parsed[2]);
    p->start = -1; /* temporary */
    p->end = -1; /* temporary */

    if (flag == 'W') {
        printf("\nUsing " RED "Worst Fit" END " Memory Allocation...\n");
        worstFit(p);

    } else if (flag == 'B') {
        printf("\nUsing " GRN "Best Fit" END " Memory Allocation...\n");
        bestFit(p);

    } else if (flag == 'F') {
        printf("\nUsing " YEL "First Fit" END " Memory Allocation...\n");
        firstFit(p);
    }

    return 0;
}

int releaseProcess(char *command) {

    char **parsed = malloc(sizeof(char *) * 4);
    char *space = strtok(command, " ");

    int i = 0;
    while (space) {
        if (i > 1) {
            printReleaseError(1);
            return -1;
        }
        parsed[i] = space;
        i++;
        space = strtok(NULL, " ");
    }

    if ((strcmp(parsed[0], "RL") != 0) && parsed[1]) {
        printf(RED "\nPlease request release using the \"RL\" command.\n\n" END);
        return -1;
    }

    if (!parsed[1]) {
        if (strcmp(parsed[0], "RL\n") == 0) {
            printReleaseError(-1);
        } else {
            printf(RED "Invalid command.\n" END);
        }
        return -1;
    }

    strtok(parsed[1], "\n");

    makeProcessHole(parsed[1]);

    if (debug) {
        printLinkedList();
    }

    return 0;
}

void allocateProcessIntoHole(struct node *holeNode, struct process *processNode) {

    if (holeNode->process->size == processNode->size) {
        holeNode->process->name = processNode->name;
        holeNode->hole = false;
    } else if (holeNode->process->size > processNode->size) {
        holeNode->process->name = processNode->name;
        
        int previousEnd = holeNode->process->end;
        holeNode->process->end = processNode->size + holeNode->process->start - 1;
        holeNode->hole = false;

        struct process *newProcess = createProcess("hole", holeNode->process->size - processNode->size);
        struct node *newHole = createHole(newProcess, holeNode->process->end + 1, previousEnd);

        if (holeNode->prev) {
            newHole->prev = holeNode->prev;
            holeNode->prev->next = newHole;
        } else {
            if (holeNode == head) {
                newHole->prev = NULL;
                head = newHole;
            } else {
                printf(RED "\nHouston, we have a problem.\n\n" END);
            }
        }

        newHole->next = holeNode;
        holeNode->prev = newHole;

        holeNode->process->size = processNode->size;

    } else if (holeNode->process->size < processNode->size) {
        printf(RED "\nNot enough room in the hole for the process... something is wrong.\n\n" END);
    }
}

struct node *locateProcess(char *name) {

    node *n;
    for (n = head; n != NULL; n = n->next) {
        if (strcmp(n->process->name, name) == 0) {
            return n;
        }
    }

    return NULL;
}

void printRequestError() {

    printf(RED "\nToo many arguments entered in the command.\n");
    printf("To request memory allocation, structure a command as follows:\n" END);
    printf("\nRQ [process name] [process bytes] [algorithm flag]\n\n");
}

void printReleaseError(int howMany) {

    if (howMany > 0) {
        printf(RED "\nIncorrect number of arguments entered in the command.\n");
    } else {
        printf(RED "\nToo few arguments entered in the command.\n");
    }
    printf("To request memory allocation, structure a command as follows:\n" END);
    printf("\nRQ [process name] [number of bytes] [strategy]\n\n");
    printf(RED "To release allocated memory, structure a command as follows:\n" END);
    printf("\nRL [process name]\n\n");
}

void printNode(node *n) {

    if (debug) {
        printf("\n");
        if (n == head) {
            printf(BLU "Head:\n" END);
        } 
        if (n == tail) {
            printf(BLU "Tail:\n" END);
        }
        printf(GRN "Process %s\n" END, n->process->name);
        printf("%d bytes\n", n->process->size);
        printf("start addr: %d\n", n->process->start);
        printf("  end addr: %d\n", n->process->end);
        if (n->hole) {
            printf(RED "Process %s is a hole!\n" END, n->process->name);
        }
    }
}

struct process * createProcess(char *name, int size) {

    struct process *p;
    p = (struct process *) malloc(sizeof(struct process));

    p->name = name;
    p->size = size;
    p->start = -1;
    p->end = -1;

    return p;
}

struct node * createHole(struct process *p, int start, int end) {

    node *new = (node *) malloc(sizeof(node));
    new->process = p;

    if ((end - start + 1) != p->size) {
        printf(RED "\nSize in bytes and addresses do not match.\n\n" END);
    }

    new->process->start = start;
    new->process->end = end;
    new->hole = true;

    return new;
}

void createNode(struct process *p, int start, int end) {

    node *new = (node *) malloc(sizeof(node));
    new->process = p;

    if ((end - start + 1) != p->size) {
        printf(RED "\nSize in bytes and addresses do not match.\n\n" END);
    }

    new->process->start = start;
    new->process->end = end;
    new->hole = false;
    new->next = NULL;
    new->prev = NULL;

    if (!head) {
        head = new;
    } else {
        new->next = head;
        head->prev = new;
        head = new;
    }
    
    if (!tail) {
        tail = new;
    }
}

void printLinkedList() {

    node *n;
    printf("\n");
    for (n = head; n != NULL; n = n->next) {
        printNode(n);
    }
}

int makeProcessHole(char *name) {

    struct node *n;
    n = locateProcess(name);
    if (!n) {
        printf(RED "\nProcess %s not located in memory.\n\n" END, name);
        return -1;
    }

    if (n->hole) {
        printf(YEL "\nProcess %s has already been released from memory, creating a hole from\n", name);
        printf("%d to %d, of size %d bytes.\n\n" END, n->process->start, n->process->end, n->process->size);
    } else {
        printf(PUR "\nProcess %s released from memory (%d bytes).\n\n" END, n->process->name, n->process->size);
        n->hole = true;
        allocated -= n->process->size;

        if (debug) {
            printNode(n);
        }

        if (n->prev && n->next) {
            if (n->prev->hole && !n->next->hole) {
                combineHoles(n, n->prev);
            } else if (n->next->hole && !n->prev->hole) {
                combineHoles(n->next, n);
            } else if (n->next->hole && n->prev->hole) {
                combineThreeHoles(n->next, n, n->prev);
            }
        } else if (n->prev && !n->next) {
            if (n->prev->hole) {
                combineHoles(n, n->prev);
            }
        } else if (n->next && !n->prev) {
            if (n->next->hole) {
                combineHoles(n->next, n);
            }
        }
    }

    return 0;
}

void combineHoles(struct node *a, struct node *b) {

    if (debug) {
        printf(YEL "Combining holes %s and %s\n", a->process->name, b->process->name);
    }
    
    b->next = a->next;
    if (a->next) {
        a->next->prev = a->prev;
    }
    b->process->size += a->process->size;
    b->process->start = a->process->start;

    if (a == tail) {
        tail = b;
    }

    free(a->process);
    free(a);
}

void combineThreeHoles(struct node *a, struct node *b, struct node *c) {

    if (debug) {
        printf(YEL "Combining holes %s, %s, and %s\n", a->process->name, b->process->name, c->process->name);
    }

    c->next = a->next;
    if (a->next) {
        a->next->prev = b->prev;
    }
    c->process->size += b->process->size + a->process->size;
    c->process->start = a->process->start;

    if (a == tail) {
        tail = c;
    }

    free(b->process);
    free(b);
    free(a->process);
    free(a);
}

void freeLinkedList() {

    struct node * n;
    while (head) {
        n = head;
        head = head->next;
        if (debug) {
            printf("freeing process %s... ", n->process->name);
        }
        free(n->process);
        free(n);
        if (debug) {
            printf(GRN "freed.\n" END);
        }
    }
}

void addName(char *n) {

    struct name *new = (struct name *) malloc(sizeof(struct name));

    char *noP = n + 1;   /* increment char * to just the number portion */
    int num = atoi(noP); /* convert char * to int */

    new->processNum = num;
    new->next = NULL;
    new->prev = NULL;

    if (!namehead) {
        namehead = new;
    } else {
        new->next = namehead;
        namehead->prev = new;
        namehead = new;
    }
}

void freeNames() {

    struct name * n;
    while (namehead) {
        n = namehead;
        namehead = namehead->next;
        if (debug) {
            printf("freeing name %d... ", n->processNum);
        }
        free(n);
        if (debug) {
            printf(GRN "freed.\n" END);
        }
    }
}

void printNames() {

    if (debug) {
        printf("\n-----------------\n");
        struct name *n;
        for (n = namehead; n != NULL; n = n->next) {
            printf(BLU "Process number: %d\n" END, n->processNum);
        }
        printf("-----------------\n");
    }
}

bool duplicate(char *name) {

    char *noP = name + 1;
    int num = atoi(noP);

    struct name *n;
    for (n = namehead; n != NULL; n = n->next) {
        if (n->processNum == num) {
            return true;
        }
    }

    return false;
}

void negateProcess(char *name) {

    char *noP = name + 1;
    int num = atoi(noP);

    struct name *n;
    for (n = namehead; n != NULL; n = n->next) {
        if (n->processNum == num) {
            n->processNum = -100000;
        }
    }
}

void noMemoryLeft(char *name) {

    printf(RED "\nNot enough memory is available to allocate process %s.\n\n" END, name);
}