#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <omp.h>
#include <omp-tools.h>
#include <execinfo.h>
#include <assert.h>
#include <pthread.h>
#include <sys/resource.h>
#include <bits/stdc++.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/syscall.h>

using namespace std;

uint64_t global_id = 0;

//pthread_mutex_t lock_t;

#define MAX_SPLIT 100000000
#define PERIOD_IN_NANOS (100UL * 1000000UL)
#define NSEC_PER_SEC 1000000000

static ompt_get_thread_data_t ompt_get_thread_data;
static ompt_get_unique_id_t ompt_get_unique_id;
static ompt_enumerate_states_t ompt_enumerate_states;

typedef struct modified_timer{
        int32_t id;
        unsigned long int time_val;
} updated_timer;

#define START_TIMER _IOW('S',2,int32_t*) // start the timer
#define MOD_TIMER _IOW('U',3,int32_t*) // modify the timer
#define DEL_TIMER _IOW('D',3,int32_t*) // delete the timer
#define MOD_TIMER_NEW _IOW('UT',3,updated_timer*)

struct sigevent timer_event;
typedef struct timespec timespec;
timespec start_time, end_time;

timespec max_timeout; // on callback work end thread can sleep or work but unknown so no time noted

typedef struct thread_info {
  int id;
  int fd;
} thread_info;

extern "C" int my_next_id() // to assign unique id's to thread (openmp might assign an id of finished thread to another)
{
  static uint64_t ID=0;
  int ret = (int) __sync_fetch_and_add(&ID,1);
  //assert(ret<MAX_THREADS && "Maximum number of allowed threads is limited by MAX_THREADS");
  return ret;
}

timespec getRegionElapsedTime(timespec start, timespec stop)
{
  timespec elapsed_time;
  if ((stop.tv_nsec - start.tv_nsec) < 0) 
  {
    elapsed_time.tv_sec = stop.tv_sec - start.tv_sec - 1;
    elapsed_time.tv_nsec = stop.tv_nsec - start.tv_nsec + 1000000000;
  } 
  else 
  {
    elapsed_time.tv_sec = stop.tv_sec - start.tv_sec;
    elapsed_time.tv_nsec = stop.tv_nsec - start.tv_nsec;
  }
  return elapsed_time;
}

timespec timespec_normalise(timespec ts)
{
  while(ts.tv_nsec >= NSEC_PER_SEC)
  {
    ++(ts.tv_sec);
    ts.tv_nsec -= NSEC_PER_SEC;
  }
  
  while(ts.tv_nsec <= -NSEC_PER_SEC)
  {
    --(ts.tv_sec);
    ts.tv_nsec += NSEC_PER_SEC;
  }
  
  if(ts.tv_nsec < 0 && ts.tv_sec > 0)
  {
    /* Negative nanoseconds while seconds is positive.
     * Decrement tv_sec and roll tv_nsec over.
    */
    
    --(ts.tv_sec);
    ts.tv_nsec = NSEC_PER_SEC - (-1 * ts.tv_nsec);
  }
  else if(ts.tv_nsec > 0 && ts.tv_sec < 0)
  {
    /* Positive nanoseconds while seconds is negative.
     * Increment tv_sec and roll tv_nsec over.
    */
    
    ++(ts.tv_sec);
    ts.tv_nsec = -NSEC_PER_SEC - (-1 * ts.tv_nsec);
  }
  
  return ts;
}

timespec timespec_add(timespec ts1, timespec ts2)
{
  /* Normalise inputs to prevent tv_nsec rollover if whole-second values
   * are packed in it.
  */
  ts1 = timespec_normalise(ts1);
  ts2 = timespec_normalise(ts2);
  
  ts1.tv_sec  += ts2.tv_sec;
  ts1.tv_nsec += ts2.tv_nsec;
  
  return timespec_normalise(ts1);
}

timespec timespec_sub(timespec ts1, timespec ts2)
{
  /* Normalise inputs to prevent tv_nsec rollover if whole-second values
   * are packed in it.
  */
  ts1 = timespec_normalise(ts1);
  ts2 = timespec_normalise(ts2);
  
  ts1.tv_sec  -= ts2.tv_sec;
  ts1.tv_nsec -= ts2.tv_nsec;
  
  return timespec_normalise(ts1);
}

extern "C" void
ompt_test ()
{
  printf("Recording an iteration\n");
  struct sched_param param;
  pthread_attr_t attr;
  int policy;
  // get parallel id here
  ompt_data_t *current_thread = ompt_get_thread_data();
  thread_info* temp_thread_data = (thread_info*) current_thread->ptr;

  pthread_attr_init(&attr);
  pthread_t thread = pthread_self();
  int result = pthread_getschedparam(thread,&policy,&param);
  if (result != 0) {
    printf("scheduling policy not retreived\n");
  }

  else {
    printf("scheduling policy priority is : %d\n", param.sched_priority);
  }

  int32_t id = syscall(__NR_gettid);
  printf("scheduling cpu is %d for id %d\n: ", sched_getcpu(),id);
  printf("modification thread id: %d\n", temp_thread_data->id);
  updated_timer test;
  test.id = id;
  test.time_val = 10000;
  ioctl(temp_thread_data->fd, MOD_TIMER_NEW, &test);
  printf("modification call made\n");
}

////////////////////////////////////////////////////////////////////////

// static void countCounter (thread_data *ptr)
// {
//   __sync_add_and_fetch(&(ptr->node->counter),1);
// }

////////////////////////////////////////////////////////////////////////


extern "C"  void
on_ompt_callback_thread_begin(
  ompt_thread_t thread_type,
  ompt_data_t *thread_data)
{
    //int fd;
    int core_id;

    core_id = my_next_id()%6;

    printf("core used is : %d\n",core_id);

    pthread_t thread;
    pthread_attr_t attr;
    struct sched_param param;
    int policy;

    thread = pthread_self();

    // Initialize thread attributes
    pthread_attr_init(&attr);

    int result = pthread_getschedparam(thread, &policy, &param);
    if (result != 0) {
      printf("scheduling policy not retreived\n");
    }

    else {
      printf("scheduling policy priority is : %d\n", param.sched_priority); 
    }

    // Set thread attributes to make it a real-time thread
    // pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    // 

    // Set the desired priority for the thread
    param.sched_priority = 10;
    policy = SCHED_FIFO;
    //pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_setschedparam(thread, policy, &param); //pthread_attr_setschedparam is used before pthread_create

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);

    thread_info* temp_thread_data = (thread_info*) malloc(sizeof(thread_info));
  
    // printf("----------------------- thread begin ---------------------\n");
    int fd = open("/dev/etx_device", O_RDWR);
    temp_thread_data->fd = fd;
    temp_thread_data->id = syscall(__NR_gettid);
    if(temp_thread_data->fd < 0) {
        printf("Cannot open device file...\n");
        return;
    }

    thread_data->ptr = temp_thread_data;  // storing the thread data such that accessible to all events

    int32_t id = syscall(__NR_gettid);
    printf("Thread id and core is %d & %d:\n", id, core_id);

    // thread_data->value = my_next_id();
    ioctl(temp_thread_data->fd, START_TIMER, &id);
}


extern "C" void
on_ompt_callback_parallel_begin(
  ompt_data_t *encountering_task_data,
  const ompt_frame_t *encountering_task_frame,
  ompt_data_t *parallel_data,
  unsigned int requested_parallelism,
  int flags,
  const void *codeptr_ra,
  unsigned int id)
{
  printf("-------- end of parallel begin -----------\n");
}

extern "C"  void
on_ompt_callback_work(
  ompt_work_t wstype,
    ompt_scope_endpoint_t endpoint,
    ompt_data_t *parallel_data,
    ompt_data_t *task_data,
    uint64_t count,
    const void *codeptr_ra,
    unsigned int sub_parallel_id)
{

  ompt_data_t *current_thread = ompt_get_thread_data();
  thread_info* temp_thread_data = (thread_info*) current_thread->ptr;
}


extern "C"  void
on_ompt_callback_parallel_end(
  ompt_data_t *parallel_data,
  ompt_data_t *encountering_task_data,
  int flags,
  const void *codeptr_ra)
{
  ompt_data_t *current_thread = ompt_get_thread_data();
}

extern "C"  void
on_ompt_callback_thread_end(
  ompt_data_t *thread_data)
{
    ompt_data_t *current_thread = ompt_get_thread_data();
    thread_info* temp_thread_data = (thread_info*) current_thread->ptr;

    int32_t id = syscall(__NR_gettid);
    ioctl(temp_thread_data->fd, DEL_TIMER, &id);
    close(temp_thread_data->fd); // closing the file descriptor here
    // execute del timer here given system id has to be sent
}

#define register_callback_t(name, type)                       \
do{                                                           \
  type f_##name = &on_##name;                                 \
  if (ompt_set_callback(name, (ompt_callback_t)f_##name) ==   \
      ompt_set_never)                                         \
    printf("0: Could not register callback '" #name "'\n");   \
}while(0)

#define register_callback(name) register_callback_t(name, name##_t)

extern "C" int ompt_initialize(
  ompt_function_lookup_t lookup,
  int initial_device_num,
  ompt_data_t *tool_data)
{
  // printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@ %d\n", parallel_region[2][0].parallel_id);

  ompt_set_callback_t ompt_set_callback = (ompt_set_callback_t) lookup("ompt_set_callback");
  ompt_get_thread_data = (ompt_get_thread_data_t) lookup("ompt_get_thread_data");
  ompt_get_unique_id = (ompt_get_unique_id_t) lookup("ompt_get_unique_id");
  ompt_enumerate_states = (ompt_enumerate_states_t) lookup("ompt_enumerate_states");

  register_callback(ompt_callback_parallel_begin);
  register_callback(ompt_callback_parallel_end);
  register_callback(ompt_callback_thread_begin);
  register_callback(ompt_callback_thread_end);
  register_callback(ompt_callback_work);

  bool flag = false;

  if(flag){
    ompt_test();
  }

  printf("Checking intial\n");
  return 1; //success
}

extern "C" void ompt_finalize(ompt_data_t* data)
{
  printf("Checking final\n");
}

extern "C" ompt_start_tool_result_t* ompt_start_tool(
  unsigned int omp_version,
  const char *runtime_version)
{
  static ompt_start_tool_result_t ompt_start_tool_result = {&ompt_initialize,&ompt_finalize,{.ptr=NULL}};
  return &ompt_start_tool_result;
}