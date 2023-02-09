#include <errno.h> 
#include <pthread.h>
#include <stdio.h>  
#include <stdlib.h> 
#include <string.h> 
#include <sys/time.h>
#include <unistd.h>

#define MAX_CUSTOMER 50
#define MAX_CLERK 5

struct customer_info { /// A struct to record the customer information read from customers.txt
    int Winner_selected; 
    int user_id;
    int class_type;
    double arrival_time;
    double service_time;
};

void *customer_entry(void *customer);
void *ServingCustomers(void *customer);
double getTime();
int check_clerk();
void assign_clerk(int clerk_id);
void free_clerk(int clerk_id);

/* global variables */

struct timeval init_time;  //this variable is used to record the simulation start time; 


pthread_cond_t MUTCUS[MAX_CUSTOMER]; //
pthread_mutex_t MT;
pthread_mutex_t TM;
static int Ncustomers;// total number of customers coming in
int presentnum = 0; 
int ClerkID[MAX_CLERK];
int Economy= 0;// total people in economy class
int Business= 0;// total people in business class
double businesstime = 0;// business waittime
double economytime = 0; // economy wait time

int main(int argc, char *argv[])
{

    // initialize all the condition variables
    FILE *fp = NULL;
    struct customer_info *custom_info[MAX_CUSTOMER];
    pthread_mutex_init(&MT, NULL);
    pthread_mutex_init(&TM, NULL);
    pthread_t  customID[MAX_CUSTOMER];
    pthread_t serving;
    int user_id;
    int class_type;
    int service_time;
    int arrival_time;
    int Bot = 0;

    gettimeofday(&init_time, NULL);

    /** Read customer information from txt file and store them in the structure you created 
		
	1. Allocate memory(array, link list etc.) to store the customer information.
	2. File operation: fopen fread getline/gets/fread ..., store information in data structure you created
	*/
    if (argc < 2) {
        fprintf(stderr, "Wrong input file\n");
        exit(1);
    } else {
        if ((fp = fopen(argv[1], "r"))) {
            fscanf(fp, "%d\n ", &Ncustomers);
            int bri = 0;
            while (bri<Ncustomers) {
                fscanf(fp, "%d:%d,%d,%d\n ", &user_id, &class_type, &arrival_time, &service_time);
                struct customer_info *custom = NULL;
                custom = (struct customer_info*)malloc(sizeof(struct customer_info));
                custom->user_id = user_id;
                custom->class_type = class_type;
                custom->arrival_time = arrival_time/10.00;
                custom->service_time = service_time/10.00;
                custom->Winner_selected = 0;
                bri++;
                custom_info[Bot] = custom;
                Bot++;
               
            }
            fclose(fp);
        } 
    }
    // initialize status of clerks using ClerkID 
    for (int brop = 0; brop < MAX_CLERK; brop++) {
        ClerkID[brop] = 0;
    }
                
    // initialize conditional variables for all customers
    for (int i = 0; i < Ncustomers; i++) { 
        pthread_cond_init(&MUTCUS[i], NULL);
    }
    // Thread for serving customers and customers serving time
     for (int i = 0; i < Ncustomers; i++){
        if(pthread_create(&customID[i], NULL, ServingCustomers, (void *)custom_info[i])!=0 ){
            perror("Failed to create thread");
        }
     }
    // thread to create a queue for customers entry 
    if(pthread_create(&serving, NULL, customer_entry, (void *)custom_info) !=0){
            perror("Failed to create thread");
    }

    // wait for all customer threads to terminate
    for (int sl = 0; sl < Ncustomers; sl++) {
      if(pthread_join(customID[sl], NULL)){
        perror("Failed to join thread");
      }
    }
    if(pthread_join(serving, NULL)!=0){
            perror("Failed to join thread");
    }

    // destroy mutex & condition variable (optional)
    pthread_mutex_destroy(&MT);
    pthread_mutex_destroy(&TM);

    for (int br = 0; br < Ncustomers; br++){
        pthread_cond_destroy(&MUTCUS[br]);
    }
    // calculate the average waiting time of all customers
    double totime = businesstime + economytime;
    double tot=Business+ Economy;
    printf("The average waiting time for all customers in the system is: %.2f seconds.\n",totime/tot);

    // free malloc memory
    for (int i = 0; i < Ncustomers; i++) {
        free(custom_info[i]);
    }
    return 0;
}

// Look for an available clerk
int check_clerk(){
    for (int i = 0; i < MAX_CLERK; i++) {
        if (ClerkID[i] == 0) {
            return (i + 1);
        }
    }
    return -1;
}
// To assign a unique Clerk identification to each customer who is waiting and also control the arrival of each customer
void *customer_entry(void *cus_info)
{
    struct customer_info **p_myInfo = (struct customer_info **)cus_info;
    struct customer_info *lbus[MAX_CUSTOMER];
    struct customer_info *leco[MAX_CUSTOMER];
    int bstart = 0;
    int bfin = 0;
    int estart = 0;
    int efin = 0;
    double queue_enter_time = 0;
    int queuelength = 0; // Length of queue
    int queuestatus = 0; // Status of customers in the queue

    while (queuestatus != Ncustomers) { 
        queue_enter_time = getTime();
        for (int i = queuelength; i < Ncustomers; i++) {
            if ((p_myInfo[i]->arrival_time) <= queue_enter_time) {
                queuelength++;
                fprintf(stdout, "A customer arrives: customer ID %2d. \n", p_myInfo[i]->user_id);
                if (p_myInfo[i]->class_type == 1) {
                    lbus[bfin] = p_myInfo[i];
                    bfin++;
                    Business++;
                    int tot = bfin-bstart;
                    fprintf(stdout, "A customer enters a queue: the queue ID %d, and length of the queue %2d. \n",p_myInfo[i]->class_type, tot);
                } 
                if (p_myInfo[i]->class_type == 0){           
                    leco[efin] = p_myInfo[i];
                    efin++;
                    Economy++;
                    int tom = efin - estart;
                    fprintf(stdout, "A customer enters a queue: the queue ID %d, and length of the queue %2d. \n",p_myInfo[i]->class_type,tom);
                   
                }
            }
        }

        for (int i = check_clerk(); (bstart < bfin) && (i > 0); bstart++,i = check_clerk() ) {
            
            pthread_mutex_lock(&MT);
                presentnum = lbus[bstart]->user_id;
                lbus[bstart]->Winner_selected = i;
                ClerkID[i - 1] = 1;
                queuestatus++;
                int brew = presentnum - 1;
                pthread_cond_signal(&MUTCUS[brew]);
            pthread_mutex_unlock(&MT); 
        }
        for (int i = check_clerk(); (estart < efin) && (i > 0); estart++,i = check_clerk()) {
            pthread_mutex_lock(&MT); 
                presentnum = leco[estart]->user_id;
                leco[estart]->Winner_selected = i;
                ClerkID[i - 1] = 1;
                queuestatus++;
                int buy = presentnum - 1;
                pthread_cond_signal(&MUTCUS[buy]);
            pthread_mutex_unlock(&MT); 
        }
    }

    return NULL;
}
// Gets the time on the cpu
double getTime()
{
    struct timeval cur_time;
    double cur_secs, init_secs;
    init_secs = (init_time.tv_sec + (double)init_time.tv_usec / 1000000);
    gettimeofday(&cur_time, NULL);
    cur_secs = (cur_time.tv_sec + (double)cur_time.tv_usec / 1000000);

    return (cur_secs - init_secs);
}

void *ServingCustomers(void *guest)
{
    struct customer_info *presentguest = (struct customer_info *)guest;
    int location = 0;
    double timetaken= 0;
    double queue_enter_time = 0;
    pthread_mutex_lock(&MT); 
        while (!(presentguest->Winner_selected)) {  
            int bread =presentguest->user_id;
            pthread_cond_wait(&MUTCUS[(bread) - 1], &MT);
        }
        location= presentguest->Winner_selected;  
    pthread_mutex_unlock(&MT); 

    queue_enter_time = getTime();
    timetaken= queue_enter_time - presentguest->arrival_time;


    pthread_mutex_lock(&TM); 
        if (presentguest->class_type == 1) {  
            businesstime += timetaken;
        } 
        if( presentguest->class_type == 0) {
            economytime += timetaken;
        }
    pthread_mutex_unlock(&TM); 
    fprintf(stdout, "A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", queue_enter_time, presentguest->user_id, location);
    usleep((presentguest->service_time) * 1000000); 
    queue_enter_time = getTime();
    fprintf(stdout, "A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n",queue_enter_time, presentguest->user_id, location);

    pthread_mutex_lock(&MT); 
        ClerkID[location - 1] = 0;
    pthread_mutex_unlock(&MT); 

    

return NULL;
}


