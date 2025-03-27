#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define msleep(x) usleep(x*100000)

int m, n, remaining_visitors, done = 0;
int *BA, *BC, *BT;

pthread_mutex_t bmtx;

pthread_barrier_t EOS, *BB;

typedef struct {
    int value;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
} semaphore;

semaphore boat, rider;

void down(semaphore *s) {
    pthread_mutex_lock(&(s->mtx));
    s->value--;
    if(s->value < 0) {
        pthread_cond_wait(&(s->cv), &(s->mtx));
    }
    pthread_mutex_unlock(&(s->mtx));
}

void up(semaphore* s) {
    pthread_mutex_lock(&(s->mtx));
    s->value++;
    if(s->value <= 0) {
        pthread_cond_signal(&(s->cv));
    }
    pthread_mutex_unlock(&(s->mtx));
}

void *boat_thread(void * args) {
    int boat_id = *((int*)args);
    printf("Boat %d Ready\n", boat_id+1);

    pthread_mutex_lock(&bmtx);
        BA[boat_id] = 1;
        BC[boat_id] = -1;
        pthread_barrier_init(&BB[boat_id], NULL, 2);
    pthread_mutex_unlock(&bmtx);

    while(1) {
        down(&boat);

        pthread_mutex_lock(&bmtx);
            if (done) {  
                pthread_mutex_unlock(&bmtx);
                break;
            }
            BA[boat_id] = 1;
            BC[boat_id] = -1;
        pthread_mutex_unlock(&bmtx);
        
        up(&rider);
        
        pthread_barrier_wait(&BB[boat_id]);

        pthread_mutex_lock(&bmtx);
            int rider_id = BC[boat_id];
            int rtime = BT[boat_id];
            BA[boat_id] = 0; 
        pthread_mutex_unlock(&bmtx);

        printf("Boat\t%d\tStart of ride for visiter %3d\n", boat_id+1, rider_id+1);
        msleep(rtime);
        printf("Boat\t%d\tEnd of ride for visiter %3d (ride time = %3d)\n", boat_id+1, rider_id+1, rtime);
        
        pthread_mutex_lock(&bmtx);
            remaining_visitors--;
            if(remaining_visitors == 0) {
                done = 1;
                pthread_mutex_unlock(&bmtx);

                pthread_mutex_lock(&(boat.mtx));
                for(int i = 0; i < m; i++) {
                    pthread_cond_signal(&(boat.cv));
                }
                pthread_mutex_unlock(&(boat.mtx));
                
                pthread_barrier_wait(&EOS);
                break;
            }
        pthread_mutex_unlock(&bmtx);
    }

    free(args);
    pthread_exit(NULL);
}

void *visitor_thread(void * args) {
    int vid = *((int*)args);
    int vtime = rand()%91+30;
    int rtime = rand()%46+15;

    printf("Visitor\t%d\tStarts sightseeing for %3d minutes\n", vid+1, vtime);
    msleep(vtime);
    up(&boat);
    down(&rider);
    printf("Visitor\t%d\tReady to ride a boat (ride time = %3d)\n", vid+1, rtime);
    
    int boat_id = -1;
    while(boat_id == -1) {
        for(int i=0; i<m; i++) {
            pthread_mutex_lock(&bmtx);
            if(BA[i] == 1 && BC[i] == -1) {
                boat_id = i;
                BC[i] = vid;
                BT[i] = rtime;
                pthread_mutex_unlock(&bmtx);
                break;
            }
            pthread_mutex_unlock(&bmtx);
        }
        
        if(boat_id == -1) {
            usleep(5000);
        }
    }
    
    printf("Visitor\t%d\tFinds boat %2d\n", vid+1, boat_id+1);
    pthread_barrier_wait(&BB[boat_id]);
    msleep(rtime);
    printf("Visitor\t%d\tLeaving\n", vid+1);

    free(args);
    pthread_exit(NULL);            
}

void free_and_destroy() {
    for(int i=0; i<m; i++) {
        pthread_barrier_destroy(&BB[i]);
    }
    
    pthread_barrier_destroy(&EOS);
    pthread_mutex_destroy(&bmtx);
    pthread_mutex_destroy(&boat.mtx);
    pthread_mutex_destroy(&rider.mtx);
    pthread_cond_destroy(&boat.cv);
    pthread_cond_destroy(&rider.cv);
    
    free(BA);
    free(BC);
    free(BT);
    free(BB);
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    if(argc < 3) {
        printf("Usage: %s <m> <n>\n", argv[0]);
        exit(1);
    }

    m = atoi(argv[1]);
    n = atoi(argv[2]);

    if(m < 5 || m > 10 || n < 20 || n > 100) {
        printf("Invalid parameters: 5 <= m <= 10, 20 <= n <= 100\n");
        exit(1);
    }

    remaining_visitors = n;

    BA = (int*)malloc(m*sizeof(int));        
    BB = (pthread_barrier_t*)malloc(m*sizeof(pthread_barrier_t)); 
    BC = (int*)malloc(m*sizeof(int));        
    BT = (int*)malloc(m*sizeof(int));        

    boat  = (semaphore){0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
    rider = (semaphore){0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};

    pthread_mutex_init(&bmtx, NULL);
    pthread_barrier_init(&EOS, NULL, 2);
    
    pthread_t tboat[m], tvisitor[n];

    void* (*fptr)(void*);

    for(int i=0; i<m+n; i++) {
        int *arg = (int*)malloc(sizeof(int)), ret;

        if(i < m) {
            *arg = i;
            fptr = &boat_thread;
        }
        else {
            *arg = i-m;
            fptr = &visitor_thread;
        }

        ret = pthread_create((i<m)?&tboat[i]:&tvisitor[i-m], NULL, fptr, arg);

        if(ret != 0) {
            perror("Thread creation failed: ");
            i--;
        }

        usleep(5000);
    }

    pthread_barrier_wait(&EOS);
    
    for(int i=0; i<m+n; i++) {
        pthread_join((i<m)?tboat[i]:tvisitor[i-m], NULL);
    }

    free_and_destroy();
    
    return 0;
}