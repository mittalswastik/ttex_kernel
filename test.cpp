#include <bits/stdc++.h>
#include <omp.h>
#include<sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

void test(int b){
    int fd;

    

    #pragma omp parallel
    {
        int i, j = 0, a;
        
        #pragma omp for schedule(static)
            for(i=0 ; i<b ; i++){
                a = 2*i;
            }
    }
}

int main(int argc, char** argv){

    int a;

    cin>>a;

    test(a);

    return 0;
}