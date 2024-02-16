#include <bits/stdc++.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define COUNT   10000
using namespace std;

#define NSEC_PER_SEC 1000000000

#define GET_TIMER _IOWR('R',3,receive_timer*) // retreive the current time from the kernel
//#define START_TIMER _IOW('S',2,int32_t*) // start the timer
#define RET_TIMER _IOR('W',1,int64_t*) //

typedef struct receive_timer {
  unsigned long int time_val_to_sub;
  unsigned long int time_val;
} receive_timer;

receive_timer receiveTime(int fd, timespec t){
  receive_timer rtime;
  rtime.time_val_to_sub = t.tv_sec*1000*1000*1000 + t.tv_nsec;
  int result = ioctl(fd, GET_TIMER, &rtime);
  if (result == -1) {
    std::cerr << "ioctl error: " << strerror(errno) << std::endl;
     //std::cerr << "ioctl error (request code " << request << "): " << strerror(errno) << std::endl;
  }
  return rtime;
}

timespec returnKernelSubTime(int fd, timespec test){
  timespec temp;
  //clock_gettime(CLOCK_MONOTONIC, &temp);
  receive_timer t1 = receiveTime(fd, test);
  temp.tv_sec = t1.time_val / NSEC_PER_SEC;
  temp.tv_nsec = t1.time_val % NSEC_PER_SEC;

  return temp;
  //timespec temp_2  = temp_thread_data->thread_current_timeout[temp_thread_data->thread_current_timeout.size()-1].et;
  //temp_thread_data->thread_current_timeout[temp_thread_data->thread_current_timeout.size()-1].et = getRegionElapsedTime(temp_2,temp);
}

int main(int argc, char **argv){
    
    timespec temp;
    temp.tv_sec = 0;
    temp.tv_nsec = 0;

    int rc;
    int i;
    int count;
    double elapsed_time;
    struct timespec start,end;
    // clock_t begin, end;
    count = COUNT;

    int fd = open("/dev/etx_device", O_RDWR);
    int fd_test = fd;
    //temp_thread_data->id = syscall(__NR_gettid);
    if(fd_test < 0) {
        std::cout<<"Cannot open device file..."<<std::endl;
        return 0;
    }

   // ioctl(fd)

    // for(int i = 0 ; i < 1000 ; i++){
    //     timespec test = returnKernelSubTime(fd, temp);
    //     timespec test2 = returnKernelSubTime(fd,test);
    //     //temp = test2;
    //     unsigned long int t = (test2.tv_sec*1000*1000*1000) + (test2.tv_nsec);

    //     std::cout<<"evalauted time is : "<<t<<std::endl;
    // }
    double store[COUNT];
    int64_t val1, val2;

    for(int i = 0 ; i < COUNT ; i++){
        //int64_t val1, val2;
        clock_gettime(CLOCK_MONOTONIC, &start);
        ioctl(fd, RET_TIMER,&val1);
        clock_gettime(CLOCK_MONOTONIC, &end);
        //ioctl(fd, RET_TIMER,&val2);
        //temp = test2;
        // unsigned long int t = val2-val1;
        // std::cout<<"evalauted time is : "<<t<<std::endl;
        store[i] = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    }

    

    //elapsed_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    //double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    //printf("Time %lf \n", time_spent);
    for(int i = 0 ; i < COUNT ; i++){
      printf("%f\n", store[i]);
    }
    //printf("Elapsed time: %f nano seconds\n", elapsed_time);

    return 0;
}