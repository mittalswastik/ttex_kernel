#include <bits/stdc++.h>
#include <omp.h>
#include<sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "profiler.h"

using namespace std;

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

    // cin>>a;

    // test(a);

    #pragma omp parallel num_threads(4)
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

    return 0;
}