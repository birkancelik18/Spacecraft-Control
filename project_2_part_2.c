#include "queue.c"
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 40; // frequency of emergency
float p = 0.2;               // probability of a ground job (launch & assembly)

int job_id = 1;

int t;

// define condition variables and locks
pthread_cond_t cond_tower;
pthread_cond_t cond_padA;
pthread_cond_t cond_padB;

pthread_mutex_t jlock;
pthread_mutex_t idLock;
pthread_mutex_t towerLock;
pthread_mutex_t padALock;
pthread_mutex_t padBLock;

// define job queues
Queue *launch_q;
Queue *landing_q;
Queue *assemble_q;
Queue *emergency_q;


// declare threads
pthread_t launch_thread;
pthread_t landing_thread;
pthread_t assembly_thread;
pthread_t tower_thread;
pthread_t emergency_thread;
pthread_t padA_thread;
pthread_t padB_thread;

pthread_attr_t attr;
time_t rawtime;
struct tm *inital_timeinfo;
struct tm *end_timeinfo;

int ending_hour;
int ending_minute;
int ending_sec;

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

    end_timeinfo = add_sim_time_to_current_time(inital_timeinfo);
    return end_timeinfo;
}
// function return pozitif if simulation continues, 0 if it ends
// it gets current time and compares it with ending time hour minute and sec values
int is_in_simulation(int ending_hour, int ending_minute, int ending_sec)
{
    time_t rawtime;
    time(&rawtime);
    struct tm *current_timeinfo = localtime(&rawtime);
    int diff = (ending_hour - current_timeinfo->tm_hour) * 3600 + (ending_minute - current_timeinfo->tm_min) * 60 +
               (ending_sec - current_timeinfo->tm_sec);
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
    assemble_q = ConstructQueue(1000);
    emergency_q = ConstructQueue(1000);

    pthread_mutex_init(&jlock, NULL);
    pthread_mutex_init(&idLock, NULL);
    pthread_mutex_init(&towerLock, NULL);
    pthread_mutex_init(&padALock, NULL);
    pthread_mutex_init(&padBLock, NULL);

    pthread_cond_init(&cond_padA, NULL);
    pthread_cond_init(&cond_padB, NULL);

    t = 2;
    end_timeinfo = calculate_ending_time();
    ending_hour = end_timeinfo->tm_hour;
    ending_minute = end_timeinfo->tm_min;
    ending_sec = end_timeinfo->tm_sec;

    // create threads
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&tower_thread, &attr, ControlTower, (void *)NULL);

    pthread_create(&padA_thread, &attr, PadA, (void *)NULL);

    pthread_create(&padB_thread, &attr, PadB, (void *)NULL);

    int counter = simulationTime;
    while (is_in_simulation(ending_hour, ending_minute, ending_sec))
    {

        double random = (rand() % 100) / 100.0;

        if (random < p / 2)
        {
            // launch
            pthread_create(&launch_thread, &attr, LaunchJob, (void *)NULL);
        }
        else if (p / 2 <= random && random < p)
        {
            // assembly
            pthread_create(&assembly_thread, &attr, AssemblyJob, (void *)NULL);
        }
        else
        {
            // land
            pthread_create(&landing_thread, &attr, LandingJob, (void *)NULL);
        }

        pthread_sleep(t);
    }
    pthread_attr_destroy(&attr);

    return 0;
}

// the function that creates plane threads for landing
void *LandingJob(void *arg)
{
    Job j;
    j.type = 1;
    pthread_mutex_lock(&idLock);
    j.ID = job_id;
    job_id++;
    pthread_mutex_unlock(&idLock);

    pthread_mutex_lock(&jlock);
    Enqueue(landing_q, j);

    pthread_cond_signal(&cond_tower);

    pthread_mutex_unlock(&jlock);
    pthread_exit(NULL);
}

// the function that creates plane threads for departure
void *LaunchJob(void *arg)
{

    Job j;
    j.type = 0;
    pthread_mutex_lock(&idLock);
    j.ID = job_id;
    job_id++;
    pthread_mutex_unlock(&idLock);

    pthread_mutex_lock(&jlock);
    Enqueue(launch_q, j);

    pthread_cond_signal(&cond_tower);

    pthread_mutex_unlock(&jlock);
    pthread_exit(NULL);
}

// the function that creates plane threads for emergency landing
void *EmergencyJob(void *arg)
{
}

// the function that creates plane threads for emergency landing
void *AssemblyJob(void *arg)
{
    Job j;
    j.type = 2;
    pthread_mutex_lock(&idLock);
    j.ID = job_id;
    job_id++;
    pthread_mutex_unlock(&idLock);

    pthread_mutex_lock(&jlock);
    Enqueue(assemble_q, j);
    pthread_cond_signal(&cond_tower);

    pthread_mutex_unlock(&jlock);
    pthread_exit(NULL);
}

// the function that controls the air traffic
void *ControlTower(void *arg)
{
    pthread_mutex_lock(&towerLock);

    while (is_in_simulation(ending_hour, ending_minute, ending_sec))
    {
        pthread_cond_wait(&cond_tower, &towerLock);
        pthread_mutex_lock(&jlock);
        int launch_queue_size = launch_q->size;
        int landing_queue_size = landing_q->size;
        int assemble_queue_size = assemble_q->size;

        if (landing_queue_size > 0)
        {

            pthread_cond_signal(&cond_padA);
            pthread_cond_signal(&cond_padB);
            pthread_mutex_unlock(&jlock);
        }
        else
        {
            if (launch_queue_size > 0)
            {
                pthread_cond_signal(&cond_padA);
                pthread_mutex_unlock(&jlock);
            }
            if (assemble_queue_size > 0)
            {
                pthread_cond_signal(&cond_padB);
                pthread_mutex_unlock(&jlock);
            }
        }
    }

    pthread_exit(NULL);
}

// the function that creates plane threads for padA
void *PadA(void *arg)
{
    pthread_mutex_lock(&padALock);
    while (is_in_simulation(ending_hour, ending_minute, ending_sec))
    {
        pthread_cond_wait(&cond_padA, &padALock);
        pthread_mutex_lock(&jlock);
        int turn = 0;
        int launch_queue_size = launch_q->size;
        int landing_queue_size = landing_q->size;
        int assemble_queue_size = assemble_q->size;

        if (launch_queue_size >= 3 && turn == 0)
        {
            Job j = Dequeue(launch_q);
            pthread_mutex_unlock(&jlock);

            pthread_sleep(2 * t);
            turn = 1;
        }
        else if (launch_queue_size >= 3 && turn == 1)
        {
            Job j = Dequeue(landing_q);
            pthread_mutex_unlock(&jlock);
            pthread_sleep(t);
            turn = 0;
        }
        else if (landing_queue_size > 0)
        {
            Job j = Dequeue(landing_q);
            pthread_mutex_unlock(&jlock);
            pthread_sleep(t);
        }
        else if (launch_queue_size > 0)
        {
            Job j = Dequeue(launch_q);
            pthread_mutex_unlock(&jlock);
            pthread_sleep(2 * t);
        }

        pthread_mutex_unlock(&jlock);

        pthread_mutex_unlock(&padALock);
    }
    pthread_exit(NULL);
}
// the function that creates plane threads for padB
void *PadB(void *arg)
{
    pthread_mutex_lock(&padBLock);
    while (is_in_simulation(ending_hour, ending_minute, ending_sec))
    {
        pthread_cond_wait(&cond_padB, &padBLock);
        pthread_mutex_lock(&jlock);
        int turn = 0;
        int launch_queue_size = launch_q->size;
        int landing_queue_size = landing_q->size;
        int assemble_queue_size = assemble_q->size;

        if (assemble_queue_size >= 3 && turn == 0)
        {
            Job j = Dequeue(assemble_q);
            pthread_mutex_unlock(&jlock);
            pthread_sleep(6 * t);
            turn = 1;
        }
        else if (assemble_queue_size >= 3 && turn == 1)
        {
            Job j = Dequeue(landing_q);
            pthread_mutex_unlock(&jlock);
            pthread_sleep(t);
            turn = 0;
        }
        else if (landing_queue_size > 0)
        {
            Job j = Dequeue(landing_q);
            pthread_mutex_unlock(&jlock);
            pthread_sleep(t);
        }
        else if (assemble_queue_size > 0)
        {
            Job j = Dequeue(assemble_q);
            pthread_mutex_unlock(&jlock);
            pthread_sleep(6 * t);
        }

        pthread_mutex_unlock(&jlock);

        pthread_mutex_unlock(&padBLock);
    }
    pthread_exit(NULL);
}
