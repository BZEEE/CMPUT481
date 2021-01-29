
#include <iostream>
#include <time.h>
#include <cstdlib>
#include <pthread.h>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <random>

// g++ sharedMemory.cpp -std=c++11 -lpthread

using namespace std;

struct thread_data {
    int threadId;
    long int *list;
    // int listStartIndex;
    long int listSize;
};

int compare(const void* a, const void* b) {
    // returns a negative integer if the first argument is less than the second
    // returns a positive integer if the first argument is greater than the second
    // returns zero if they are equal
	const long int* x = (long int*) a;
	const long int* y = (long int*) b;

	if (*x > *y) {
        return 1;
    }	
	else if (*x < *y) {
        return -1;
    } else {
        return 0;
    }
}

void *sortOnThread(void *threadData) {
    // get args passed to thread
    struct thread_data *threadArgs;
    threadArgs = (struct thread_data *) threadData;
    int id = threadArgs->threadId;
    long int* array = threadArgs->list;
    long int listSize = threadArgs->listSize;
    // perform quick sort on partitioned data using quicksort, sorts in place
    qsort(array, listSize, sizeof(long int), compare);
    //close the thread
    pthread_exit(NULL);
}

int main() {
    // generate a random array of 1 million integers
    // use time as the seed since it is a number that changes all the time
    // default_random_engine generator;
    srand(time(NULL));
    
    const long int listSize = 32000000;
    long int *array = new long int[listSize];
    for (long int i = 0; i < listSize; i++) {
        array[i] = ((unsigned long int) (rand() * rand() * rand())) % 2147483647;
    }

    // display unsorted list
    // cout << "\nBefore:\n";
    // for(int i = 0; i < listSize; i++) {
    //     cout << array[i] << " ";
    // };
    // cout << "\n";

    // set parameters
    const long int numThreads = 16;
	pthread_t phase1Threads [numThreads];
    struct thread_data phase1ThreadData[numThreads];
    // set start and end partition indices
    long int *start = &array[0];
    long int partitionSize = floor(listSize / numThreads);

    // start the clock
    clock_t startClock = clock(); 

    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 1 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // pass (n/p) items to each processor, create a thread for each process
    for (long int i = 0; i < numThreads; i++) {
        // segment list into (list size / number of threads) partitions, pass each partition to a thread to sort it
        phase1ThreadData[i].threadId = i;
        phase1ThreadData[i].list = &array[i * partitionSize];
        // threadData[i].listStartIndex = i * partitionSize;
        // the list and number of processors isnt always evenly divisible
        // have to account for the last array
        if (i == numThreads - 1) {
            phase1ThreadData[i].listSize = listSize - (i * partitionSize);
        } else {
            phase1ThreadData[i].listSize = partitionSize;
        };
        pthread_create(&phase1Threads[i], NULL, sortOnThread, (void *) &phase1ThreadData[i]);
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
        // pthread_join(phase1Threads[i], NULL);
    }

    // join all the threads together to sync end of phase 1 with beginning of phase 2
    for (long int i = 0; i < numThreads; i++) {
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
        pthread_join(phase1Threads[i], NULL);
    }

    // stop the clock for Phase 1
    clock_t phase1Stop = clock();

    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 2 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // pick (number of processors - 1) pivots from each thread partition

    long int sampleSize = numThreads * numThreads;
    long int gatheredRegularSample[sampleSize];
    long int partitionPivotSeparation = floor(partitionSize / numThreads);
    for (long int i = 0; i < numThreads; i++) {
        for (long int j = 0; j < numThreads; j++) {
            // create a smaller array with all the pivot values from the partitioned array
            gatheredRegularSample[(i * numThreads) + j] = array[(i * partitionSize) + (j * partitionPivotSeparation)];
        }
    }
    // sort the smaller pivot array, sorts in place
    qsort(gatheredRegularSample, sampleSize, sizeof(long int), compare);

    // pick new pivots from pivot Array
    long int regularSamplePivots[numThreads-1];
    long int regularSamplePivotSeparation = floor(sampleSize / numThreads);
    for (long int i = 1; i < numThreads; i++) {
        regularSamplePivots[i-1] = gatheredRegularSample[i * regularSamplePivotSeparation];
    }

    // stop the clock for Phase 2
    clock_t phase2Stop = clock();
    
    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 3 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // exchange partitions with each processor based on regular sample pivots
    // loop through list size_t
    // but know when to compare to a certain pivot

    // for (int i = 0; i < numThreads - 1; i++) {
    //     cout << regularSamplePivots[i] << " ";
    // }
    // // cout << partitionSize << "\n";
    // cout << "\n";


    vector<long int> exchangedPartitions[numThreads];
    long int samplePivotCounter;

    for (long int i = 0; i < numThreads; i++) {
        // figure out break points
        samplePivotCounter = 0;
        for (long int j = 0; j < phase1ThreadData[i].listSize; j++) {
            long int arrayElement = array[(i * partitionSize) + j];
            if (samplePivotCounter == numThreads - 1) {
                // all the elements at the back after the last partition automatically go to the very last processor
                exchangedPartitions[samplePivotCounter].push_back(arrayElement);
            } else if (arrayElement <= regularSamplePivots[samplePivotCounter]) {
                // add to exchanged partition process thread that equals the counter index
                exchangedPartitions[samplePivotCounter].push_back(arrayElement);
            } else {
                // incerement the pivot counter for the next pivot and stay on the current element
                samplePivotCounter++;
                j--;
            }
        }
    }

    // cout << "\nsup\n";
    // for (long int i = 1; i < numThreads; i++) {
    //     merge(exchangedPartitions[0].begin(), exchangedPartitions[0].end(), exchangedPartitions[i].begin(), exchangedPartitions[i].end(), exchangedPartitions[0].begin());
    // }
    // cout << "\nbruh\n";
    // copy(exchangedPartitions[0].begin(), exchangedPartitions[0].end(), array[0]);


    // stop the clock for Phase 3
    clock_t phase3Stop = clock();

    // for (int i = 0; i < numThreads; i++) {
    //     for (int j = 0; j < exchangedPartitions[i].size(); j++) {
    //         cout << exchangedPartitions[i][j] << " ";
    //     };
    //     cout << "\n";
    // }


    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 4 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // merger the vectors back to the main array memory slots
    // this allows us to skip the merge step for phase 4, since
    // the exchanged partitions or sorted in place right next to each other
    // by each process
    long int mainMemoryCounter = 0;
    for (long int i = 0; i < numThreads; i++) {
        for (long int j = 0; j < exchangedPartitions[i].size(); j++) {
            array[mainMemoryCounter] = exchangedPartitions[i][j];
            mainMemoryCounter++;
        };
    }

    pthread_t phase3Threads [numThreads];
    struct thread_data phase3ThreadData[numThreads];

    // pass each exchanged partition to the processor to run quicksort again
    long int startIndex = 0;
    for (long int i = 0; i < numThreads; i++) {
        // segment list into (list size / number of threads) partitions, pass each partition to a thread to sort it
        phase3ThreadData[i].threadId = i;
        phase3ThreadData[i].list = &array[startIndex];
        phase3ThreadData[i].listSize = exchangedPartitions[i].size();
        startIndex = startIndex + exchangedPartitions[i].size();
        pthread_create(&phase3Threads[i], NULL, sortOnThread, (void *) &phase3ThreadData[i]);
    }

    // join all the threads together to sync end of phase 1 with beginning of phase 2
    for (long int i = 0; i < numThreads; i++) {
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
        pthread_join(phase3Threads[i], NULL);
    }

    // stop the clock for Phase 4
    clock_t phase4Stop = clock();

    // Calculating total time taken by the program. 
    cout << "Time taken for Phase 1: " << fixed  << double(phase1Stop - startClock) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl;
    cout << "Time taken for Phase 2: " << fixed  << double(phase2Stop - phase1Stop) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl;
    cout << "Time taken for Phase 3: " << fixed  << double(phase3Stop - phase2Stop) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl;
    cout << "Time taken for Phase 4: " << fixed  << double(phase4Stop - phase3Stop) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl; 
    cout << "Total time: " << fixed  << double(phase4Stop - startClock) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl;  

    // print sorted array
    // cout << "\nAfter:\n";
    // for(int i = 0; i < listSize; i++) {
    //     cout << array[i] << " ";
    // };
    // cout << "\n";


    // cout << "\nTest Correctness:\n";
    // for(int i = 0; i < listSize - 1; i++) {
    //     if (array[i] > array[i+1]) {
    //         cout << "false\n";
    //     }
    // };
    // cout << "\n";

    pthread_exit(NULL); // last thing that main should do

  
    
    // return 0;
}


