#define COUNT   10000
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

int main( int argc, const char* argv[] ){
    int rc;
    int i;
    int count;
    double elapsed_time;
    struct timespec start,end;
    // clock_t begin, end;
    count = COUNT;
    //begin = clock();
    //clock_gettime(CLOCK_MONOTONIC, &start);
    double store[COUNT];

    for ( i = 0; i < count; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        rc = getpid();
        clock_gettime(CLOCK_MONOTONIC, &end);
        store[i] = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        //ioctl(fd, RET_TIMER,&val2);
        //temp = test2;
        // unsigned long int t = val2-val1;
        // std::cout<<"evalauted time is : "<<t<<std::endl;
    }

    for(int i = 0 ; i < COUNT ; i++){
      printf("%f\n", store[i]);
    }

    //end = clock();
    //clock_gettime(CLOCK_MONOTONIC, &end);
    // Calculate the elapsed time in seconds with nanosecond precision
    // elapsed_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    // printf("rc %d\n",rc);
    // //double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    // //printf("Time %lf \n", time_spent);
    // printf("Elapsed time: %f nano seconds\n", elapsed_time);
}