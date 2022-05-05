# COMP 304 : Operating Systems
# Spacecraft Control Simulation with PThreads

The project simulates independently acting spacecrafts asking for permission to depart or land from the Control Tower. The simulation is implemented in C as a scheduling algorithm with mutex locks and condition variables.

## Completion of the Project

Simulation is fully functional. Each part is solved at related c files.

## Running the Project

To run the project in Linux environment: 

```
gcc -pthread  project_2.c
gcc -pthread  project_2_part_2.c
gcc -pthread  project_2_part_3.c
gcc -pthread  project_2_part_4.c
```

The output file accepts four parameters. -p flag determines treashold probability, -t flag determines simulation time, -s flag determines random seed, -r flag determines time limit note that -r flag only works in project_2_part_4.c file. They all have default values meaning all parameters are optional. 

```
./a.out 
```
---
## Design of the Project
---
### Structs
* The **Job** is used to represent the jobs which can be landing, assembly, launching, emergency. It has ID, type, requestTime, endTime, pad properties. ID is unique for each job. Other properties are stored for logging purposes. RequestTime stores when the job is created, endTime stores when the job is done, pad stores which pad the job has completed A or B. 
---
### Threads

The project starts with 3 threads: PadA, PadB and Control tower. Each waits for some conditions and works when the conditions are signalled by the jobs. Each job is also a thread. 

---

### Functions
_main_:
* Parses the command line arguments.
* Marks the ending time of the simulation.
* Creates padA, padB and control tower threads for once.
* generates a new job thread every t second namely landing, launching, assembly. If t is a mutliple of emergency frequency 2 emergency landings are created. 

_LandingJob_:
* Creates a landing job, puts it into landing queue.
* Signals the tower for newly created landing permission.

_LaunchJob_:
* Creates a launch job, puts it into launch queue.
* Signals the tower for newly created launch permission.

_EmergencyJob_:
* Creates an emergency job, puts it into emergency queue.
* Signals the tower for newly created emergency permission.

_AssemblyJob_:
* Creates an assembly job, puts it into assembly queue.
* Signals the tower for newly created assembly permission.

_ControlTower_ :
* waits for signal from spacecraft.
* gives permission to pads according to scheduling.
* manages starvation and emergency landing.

_PadA_: 
* Makes launch, landing and emergency jobs.
* Work according to signals coming from control tower. 
* Manages starvation and deadlock conditions. 

_PadB_:
* Makes assembly, landing and emergency jobs.
* Work according to signals coming from control tower. 
* Manages starvation and deadlock conditions. 

_printLog_:
* Creates log file for the jobs. 

_calculate_ending_time_ :
* Marks ending time of the simulation in real time.  

_simulationContinues_ :
* Checks if the simulation can continue.

_time_left_to_end_ :
* Calculates the time left to end of the simulation.

_is_in_simulation_:
* Returns true if current time is less than simulation ending time. 

---

### Locks & Conditions
* cond_tower : Condition that is used to wake towers up when new job is created. 
* cond_padA : Condition that tower uses to wake PadA up when PadA will be used. 
* cond_padB :  Condition that tower uses to wake PadB up when PadB will be used.

* jlock : Lock to protect shared Queues.
* idLock : Lock to protect job ID.
* towerLock : In order to use cond_tower.
* padALock : In order to use cond_padA.
* padBLock : In order to use cond_padB.

---

## Solution to Starvation Problem in Part-II

The control tower favors the landing spacecrafts and lets the waiting spacecrafts to land until either no more spacecrafts are waiting to land or 3 or more spacerafts are waiting to take-off from the ground or assemble. This might cause starvation for spacecrafts on the air, since a new job is created at every t seconds. Depending on the probablity newly created assembly and launching jobs will take the priority due to their increasing job amounts. In order to prevent this situation, we prioritize the jobs alternatingly. When there are 3+ waiting ground jobs tower first favor ground job then the next turn is given the landing so on and so forth.   

**Note: In part IV all the both of the outputs are in different log files. 

## Author of the Project

* İrem Karaca  
* Birkan Çelik
