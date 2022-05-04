#include "queue.c"
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int simulationTime = 20;     // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 40; // frequency of emergency
float p = 0.2;               // probability of a ground job (launch & assembly)

int job_id = 1;
// 0 => Launch
// 1 => Landing
// 2 => Assembly
// 3 => Emergency
int t = 2;

// define condition variables and locks
pthread_cond_t cond_landing;
pthread_cond_t cond_launch;
pthread_cond_t cond_assembly;
pthread_cond_t cond_tower;
pthread_cond_t cond_emergency;
pthread_mutex_t jlock;

// define job queues
Queue *launch_q;
Queue *landing_q;
Queue *assembly_q;
Queue *emergency_q;
Queue *padA_q;
Queue *padB_q;

pthread_attr_t attr;
time_t rawtime;
struct tm *inital_timeinfo;
struct tm *end_timeinfo;

void *LandingJob(void *arg);
void *LaunchJob(void *arg);
void *EmergencyJob(void *arg);
void *AssemblyJob(void *arg);
void *ControlTower(void *arg);
void *PadA(void *arg);
void *PadB(void *arg);
struct tm *add_sim_time_to_current_time(struct tm *timeinfo);
struct tm *calculate_ending_time();
int is_in_simulation(int hour, int minute, int sec);

// pthread sleeper function
int pthread_sleep(int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if (pthread_mutex_init(&mutex, NULL))
    {
        return -1;
    }
    if (pthread_cond_init(&conditionvar, NULL))
    {
        return -1;
    }
    struct timeval tp;
    // When to expire is an absolute time, so get the current time and add it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds;
    timetoexpire.tv_nsec = tp.tv_usec * 1000;

    pthread_mutex_lock(&mutex);
    int res = pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);

    // Upon successful completion, a value of zero shall be returned
    return res;
}

// function adding simulation time seconds to given time object and returns a new time object
struct tm *add_sim_time_to_current_time(struct tm *timeinfo)
{
    timeinfo->tm_sec += simulationTime;
    // mktime(timeinfo);
    return timeinfo;
}
struct tm *calculate_ending_time()
{
    time(&rawtime);
    inital_timeinfo = localtime(&rawtime);
    // printf("Current local time hour %d, minute %d, seconds %d\n", inital_timeinfo->tm_hour,inital_timeinfo->tm_min, inital_timeinfo->tm_sec);

    end_timeinfo = add_sim_time_to_current_time(inital_timeinfo);
    // printf("ending time info hour %d, minute %d, seconds %d\n", end_timeinfo->tm_hour,end_timeinfo->tm_min, end_timeinfo->tm_sec);
    return end_timeinfo;
}
// function return pozitif if simulation continues, 0 if it ends
// it gets current time and compares it with ending time hour minute and sec values
int is_in_simulation(int ending_hour, int ending_minute, int ending_sec)
{
    // printf("inside is_in_simulation\n");
    time_t rawtime;
    time(&rawtime);
    struct tm *current_timeinfo = localtime(&rawtime);

    int diff = (ending_hour - current_timeinfo->tm_hour) * 3600 + (ending_minute - current_timeinfo->tm_min) * 60 +
               (ending_sec - current_timeinfo->tm_sec);
    printf("diff is: %d\n", diff);
    return diff > 0;
}

int main(int argc, char **argv)
{
    // -p (float) => sets p
    // -t (int) => simulation time in seconds
    // -s (int) => change the random seed

    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-p"))
        {
            p = atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "-t"))
        {
            simulationTime = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-s"))
        {
            seed = atoi(argv[++i]);
        }
    }

    srand(seed); // feed the seed

    /* Queue usage example
        Queue *myQ = ConstructQueue(1000);
        Job j;
        j.ID = myID;
        j.type = 2;
        Enqueue(myQ, j);
        Job ret = Dequeue(myQ);
        DestructQueue(myQ);
    */

    // your code goes here
    launch_q = ConstructQueue(1000);
    landing_q = ConstructQueue(1000);
    assembly_q = ConstructQueue(1000);
    emergency_q = ConstructQueue(1000);
    padA_q = ConstructQueue(1000);
    padB_q = ConstructQueue(1000);

    pthread_mutex_init(&jlock, NULL);
    pthread_cond_init(&cond_launch, NULL);
    pthread_cond_init(&cond_landing, NULL);
    pthread_cond_init(&cond_assembly, NULL);
    pthread_cond_init(&cond_emergency, NULL);

    // declare threads
    pthread_t launch_thread;
    pthread_t landing_thread;
    pthread_t assembly_thread;
    pthread_t tower_thread;
    pthread_t emergency_thread;
    pthread_t padA_thread;
    pthread_t padB_thread;

    end_timeinfo = calculate_ending_time();
    int ending_hour = end_timeinfo->tm_hour;
    int ending_minute = end_timeinfo->tm_min;
    int ending_sec = end_timeinfo->tm_sec;
    // printf("in simulation %d\n",is_in_simulation(end_timeinfo->tm_hour, end_timeinfo->tm_min, end_timeinfo->tm_sec));

    // create threads
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // printf("before tower creation\n");

    
    pthread_mutex_lock(&jlock);
    pthread_create(&tower_thread, &attr, ControlTower, (void *)NULL);

    // printf("before launch creation\n");
    pthread_create(&launch_thread, &attr, LaunchJob, (void *)NULL);

    while (is_in_simulation(ending_hour, ending_minute, ending_sec))
    {
        // burada thread yaratınca beklediğim gibi çalışmadı threadleri
        // join etmek yerine fonksiyonların içinde exit diyebiliriz
        printf("inside simulation time simulation\n");
        // printf("before padB_thread creation\n");
        pthread_create(&padB_thread, &attr, PadB, (void *)NULL);
        printf("After padB_thread creation\n");
        pthread_sleep(2);    
    }


    // printf("before padA_thread creation\n");
    pthread_create(&padA_thread, &attr, PadA, (void *)NULL);

    // printf("before padB_thread creation\n");
    pthread_create(&padB_thread, &attr, PadB, (void *)NULL);

    pthread_join(tower_thread, NULL);
    // printf("after tower join \n");

    pthread_join(launch_thread, NULL);
    // printf("after launch join \n");

    // pthread_join(padA_thread, NULL);
    //  printf("after tower join \n");

    pthread_join(padB_thread, NULL);
    // printf("after launch join \n");

    pthread_attr_destroy(&attr);

    return 0;
}

// the function that creates plane threads for landing
void *LandingJob(void *arg)
{
}

// the function that creates plane threads for departure
void *LaunchJob(void *arg)
{

    Job j;
    j.ID = job_id;
    job_id++;
    j.type = 0;

    // printf("ID of Departing Plane: %d\n", j.ID);

    pthread_mutex_lock(&jlock);
    Enqueue(launch_q, j);

    // printf("launchq size is: %d\n", (*launch_q).size);
    if (j.ID == 1)
    {
        printf("Waking tower up for launching permission...\n");
        pthread_cond_signal(&cond_tower);
    }
    pthread_cond_wait(&cond_launch, &jlock);
    // printf("after condition wait in launch\n");
    pthread_mutex_unlock(&jlock);
    // printf("after mutexunlock in launch \n");
    pthread_exit(NULL);
}

// the function that creates plane threads for emergency landing
void *EmergencyJob(void *arg)
{
}

// the function that creates plane threads for emergency landing
void *AssemblyJob(void *arg)
{
}

// the function that controls the air traffic
void *ControlTower(void *arg)
{
    pthread_cond_wait(&cond_tower, &jlock);
    printf("Tower is awaken...\n");
    // Start piece of a rocket needs to land, or a team of engineers and mechanics are ready to start
    // assembly, they will contact the control tower and notify the tower about it.

    pthread_cond_signal(&cond_launch);
    printf("Tower gave permission...\n");
    printf("Launch has started...\n");
    pthread_sleep(2 * t);
    Dequeue(launch_q);
    printf("Launched ended.\n");
    pthread_mutex_unlock(&jlock);

    pthread_exit(NULL);
}

// the function that creates plane threads for padA
void *PadA(void *arg)
{
    if (launch_q->size == 0 && landing_q->size == 0)
    {
        pthread_sleep(t);
    }
    else
    {
    }
}
// the function that creates plane threads for padB
void *PadB(void *arg)
{   
   printf("This is thread B\n");
   printf("Launchq size is:%d \n", launch_q->size);
}
