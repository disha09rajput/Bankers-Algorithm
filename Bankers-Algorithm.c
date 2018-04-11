#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

int noOfResources,noOfProcesses;
int *resources;
int **allocated;
int **maxRequired;
int **need;
int *safeSeq;
int nProcessRan = 0;

pthread_mutex_t lockResources;
pthread_cond_t condition;

bool getSafeSeq();

void* processCode(void* );

int main(int argc, char** argv) {

	    srand(time(NULL));

        printf("\nNumber of processes: ");
        scanf("%d", &noOfProcesses);

        printf("\nNumber of resources: ");
        scanf("%d", &noOfResources);

        resources = (int *)malloc(noOfResources * sizeof(*resources));
        printf("\nCurrently Available resources (R1 R2 ...)? ");
        for(int i=0; i<noOfResources; i++)
                scanf("%d", &resources[i]);

        allocated = (int **)malloc(noOfProcesses * sizeof(*allocated));
        for(int i=0; i<noOfProcesses; i++)
                allocated[i] = (int *)malloc(noOfResources * sizeof(**allocated));

        maxRequired = (int **)malloc(noOfProcesses * sizeof(*maxRequired));
        for(int i=0; i<noOfProcesses; i++)
                maxRequired[i] = (int *)malloc(noOfResources * sizeof(**maxRequired));

        // allocated
        printf("\n");
        for(int i=0; i<noOfProcesses; i++) {
                printf("\nResource allocated to process %d (R1 R2 ...): ", i+1);
                for(int j=0; j<noOfResources; j++)
                        scanf("%d", &allocated[i][j]);
        }
        printf("\n");

	    // maximum required resources
        for(int i=0; i<noOfProcesses; i++) {
                printf("\nMaximum resource required by process %d (R1 R2 ...): ", i+1);
                for(int j=0; j<noOfResources; j++)
                        scanf("%d", &maxRequired[i][j]);
        }
        printf("\n");

	    // calculate need matrix
        need = (int **)malloc(noOfProcesses * sizeof(*need));
        for(int i=0; i<noOfProcesses; i++)
                need[i] = (int *)malloc(noOfResources * sizeof(**need));

        for(int i=0; i<noOfProcesses; i++)
                for(int j=0; j<noOfResources; j++)
                        need[i][j] = maxRequired[i][j] - allocated[i][j];

	    // get safe sequence
	    safeSeq = (int *)malloc(noOfProcesses * sizeof(*safeSeq));
        for(int i=0; i<noOfProcesses; i++) safeSeq[i] = -1;

        if(!getSafeSeq()) {
                printf("\nUnsafe State! The processes leads the system to a unsafe state.\n\n");
                exit(-1);
        }

        printf("\n\nSafe Sequence Found : ");
        for(int i=0; i<noOfProcesses; i++) {
                printf("%-3d", safeSeq[i]+1);
        }

        printf("\nExecuting Processes...\n\n");
        sleep(1);
	
	    // run threads
	    pthread_t processes[noOfProcesses];
        pthread_attr_t attr;
        pthread_attr_init(&attr);

	    int processNumber[noOfProcesses];
	    for(int i=0; i<noOfProcesses; i++) processNumber[i] = i;

        for(int i=0; i<noOfProcesses; i++)
                pthread_create(&processes[i], &attr, processCode, (void *)(&processNumber[i]));

        for(int i=0; i<noOfProcesses; i++)
                pthread_join(processes[i], NULL);

        printf("\nAll Processes Finished\n");	
	
	    // free resources
        free(resources);
        for(int i=0; i<noOfProcesses; i++) {
                free(allocated[i]);
                free(maxRequired[i]);
		free(need[i]);
        }
        free(allocated);
        free(maxRequired);
	    free(need);
        free(safeSeq);
}


bool getSafeSeq() {
	// get safe sequence
        int tempRes[noOfResources];
        for(int i=0; i<noOfResources; i++) tempRes[i] = resources[i];

        bool finished[noOfProcesses];
        for(int i=0; i<noOfProcesses; i++) finished[i] = false;
        int nfinished=0;
        while(nfinished < noOfProcesses) {
                bool safe = false;

                for(int i=0; i<noOfProcesses; i++) {
                        if(!finished[i]) {
                                bool possible = true;

                                for(int j=0; j<noOfResources; j++)
                                        if(need[i][j] > tempRes[j]) {
                                                possible = false;
                                                break;
                                        }

                                if(possible) {
                                        for(int j=0; j<noOfResources; j++)
                                                tempRes[j] += allocated[i][j];
                                        safeSeq[nfinished] = i;
                                        finished[i] = true;
                                        ++nfinished;
                                        safe = true;
                                }
                        }
                }

                if(!safe) {
                        for(int k=0; k<noOfProcesses; k++) safeSeq[k] = -1;
                        return false; // no safe sequence found
                }
        }
        return true; // safe sequence found
}

// process code
void* processCode(void *arg) {
        int p = *((int *) arg);

	// lock resources
        pthread_mutex_lock(&lockResources);

        // condition check
        while(p != safeSeq[nProcessRan])
                pthread_cond_wait(&condition, &lockResources);

	// process
        printf("\n--> Process %d", p+1);
        printf("\n\tAllocated : ");
        for(int i=0; i<noOfResources; i++)
                printf("%3d", allocated[p][i]);

        printf("\n\tNeeded    : ");
        for(int i=0; i<noOfResources; i++)
                printf("%3d", need[p][i]);

        printf("\n\tAvailable : ");
        for(int i=0; i<noOfResources; i++)
                printf("%3d", resources[i]);

        printf("\n"); sleep(1);

        printf("\tResource Allocated!");
        printf("\n"); sleep(1);
        printf("\tProcess Code Running...");
        printf("\n"); sleep(rand()%3 + 2); // process code
        printf("\tProcess Code Completed...");
        printf("\n"); sleep(1);
        printf("\tProcess Releasing Resource...");
        printf("\n"); sleep(1);
        printf("\tResource Released!");

	    for(int i=0; i<noOfResources; i++)
                resources[i] += allocated[p][i];

        printf("\n\tNow Available : ");
        for(int i=0; i<noOfResources; i++)
                printf("%3d", resources[i]);
        printf("\n\n");

        sleep(1);


        nProcessRan++;
        pthread_cond_broadcast(&condition);
        pthread_mutex_unlock(&lockResources);
	pthread_exit(NULL);
}
