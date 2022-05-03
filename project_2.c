#include "queue.c"
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 40; // frequency of emergency
float p = 0.2;               // probability of a ground job (launch & assembly)

int job_id = 1;
// 0 => Launch
// 1 => Landing
// 2 => Assembly
// 3 => Emergency
int t=2;

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


void *LandingJob(void *arg);
void *LaunchJob(void *arg);
void *EmergencyJob(void *arg);
void *AssemblyJob(void *arg);
void *ControlTower(void *arg);

pthread_attr_t attr;

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

int main(int argc, char **argv)
{
    // -p (float) => sets p
    // -t (int) => simulation time in seconds
    // -s (int) => change the random seed
    /*
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

    */

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
    Queue *launch_q = ConstructQueue(1000);
    Queue *landing_q = ConstructQueue(1000);
    Queue *assembly_q = ConstructQueue(1000);
    Queue *emergency_q = ConstructQueue(1000);

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

    /* Create threads to perform the dotproduct  */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_mutex_lock(&jlock);
    printf("before tower creation\n");
    pthread_create(&tower_thread, &attr, ControlTower, (void *)NULL);

    printf("before launch creation\n");
    pthread_create(&launch_thread, &attr, LaunchJob, (void *)NULL);



    pthread_join(tower_thread, NULL);
    printf("after tower join \n");

    pthread_join(launch_thread, NULL);
    printf("after launch join \n");

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

    printf("ID of Departing Plane: %d\n", j.ID);

    pthread_mutex_lock(&jlock);
    Enqueue(launch_q, j);

    if (j.ID == 1)
    { // if the plane is the first plane
        printf("Tower! this is captain speaking. This is the first plane. Its ID is: %d\n", j.ID);
        pthread_cond_signal(&cond_tower);
    }
    pthread_cond_wait(&cond_launch, &jlock);
    printf("after condition wait in launch\n");
    pthread_mutex_unlock(&jlock);
    printf("after mutexunlock in launch \n");
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

    pthread_cond_signal(&cond_launch);
    //DestructQueue(launch_q); // SORUN SORUN SORUN SORUN 
   // pthread_sleep(2*t);
   pthread_mutex_unlock(&jlock);
    
    pthread_exit(NULL);

}