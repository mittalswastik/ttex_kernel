#include <bits/stdc++.h>
#include <omp.h>
#include<sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "profiler_update.h"

using namespace std;

//int thread_priority = 0;

void test(int b){
    int fd;

    #pragma omp parallel
    {
        int i, j = 0, a;
        
        #pragma omp for schedule(static)
            for(i=0 ; i<20 ; i++){
                for(int j=0 ; j<100 ; j++){
                    a = 2*i;
                }

                ompt_test();
            }
    }
}

int main(int argc, char** argv){

    // int a;

    if(argc == 3){
        thread_priority = atoi(argv[1]);
        time_val_msec = atoi(argv[2]);
    }

    // cin>>a;

    // test(a);

    #pragma omp parallel num_threads(20)
    {
        int i, j = 0, a;
        
        #pragma omp for schedule(static)
            for(i=0 ; i<100 ; i++){
                for(int j=0 ; j<1000 ; j++){
                    a = 2*i;
                }

                sleep(3);

                ompt_test();
            }
    }

    return 0;
}