#include <stdio.h>
#include <stdlib.h>

#define MAX 1000
#define INF 1e9

typedef struct linked_list {
    int data;
    struct linked_list *next;
}list;

typedef enum {
    FRESH_ARRIVAL,
    IO_COMPLETED,
    CPU_TIMED_OUT,
    CPU_BURST_COMPLETED,
}process_state;

typedef struct process_info{
    int id;
    int arrival_time;
    int turnaround_time;
    int wait_time;
    int last_inserted;
    list *burst_head;
    list *current_burst;
}process_info;

typedef struct {
    int PID;
    int arrival_time;
    process_state state_completed;
}event_info;

/*-----------------------------------------------------*/

typedef struct {
    int arr[MAX]; 
    int q_front;
    int back;
}queue;

void init_fifo_queue(queue* Q) {
    Q->q_front = 0;
    Q->back = MAX - 1;
    return;
}

int isQEmpty(queue Q) {
    return (Q.q_front == (Q.back + 1) % MAX);
}

int isQFull(queue Q) {
    return (Q.q_front == (Q.back + 2) % MAX);
}

int queue_size(queue Q) {
    return (Q.back - Q.q_front) % MAX + 1;
} 

int q_front(queue Q) {
    if (isQEmpty(Q))
    {
        printf("\nq_front: Empty Queue\n");
        return -1;
    }

    return Q.arr[Q.q_front];
}

queue enqueue_q(queue Q, int id) {
    if (isQFull(Q)) {
        printf("\nenqueue_pq: Full Queue\n");
        return Q;
    }

    Q.back = (++Q.back) % MAX;
    Q.arr[Q.back] = id;

    return Q;
}

queue dequeue_q(queue Q) {
    if (isQEmpty(Q)) {
        printf("\ndequeue_pq: Empty Queue\n");
        return Q;
    }

    Q.q_front = (++Q.q_front) % MAX;
    return Q;
}
/* ---------------------------------------------------------*/

typedef struct {
    event_info items[MAX];
    int size;
} PriorityQueue;

void swap(event_info* a, event_info* b) {
    event_info temp = *a;
    *a = *b;
    *b = temp;
}

void init_pq(PriorityQueue* pq) {
    pq->size = 0;
}

int custom_compare(event_info* a, event_info* b) {
    if(a->arrival_time > b->arrival_time) {
        return 1;
    }
    else if(a->arrival_time == b->arrival_time) {
        if(
            (b->state_completed == FRESH_ARRIVAL || b->state_completed == IO_COMPLETED)
            && a->state_completed == CPU_TIMED_OUT
        ) 
        {
            return 1;
        }
        else if(
            (a->state_completed == IO_COMPLETED || a->state_completed == FRESH_ARRIVAL)
            && (b->state_completed == IO_COMPLETED || b->state_completed == FRESH_ARRIVAL) 
            && a->PID > b->PID
        ) 
        {
            return 1;
        }
    }
    return 0;
}

void heapifyUp(PriorityQueue* pq, int index) {
    if(index && custom_compare(&pq->items[(index - 1) / 2], &pq->items[index])) {
        swap(&pq->items[(index - 1) / 2], &pq->items[index]);
        heapifyUp(pq, (index - 1) / 2);
    }
}

void enqueue_pq(PriorityQueue* pq, int value, int id, int state_comp) {
    if (pq->size == MAX) {
        printf("Priority queue is full\n");
        return;
    }

    pq->items[pq->size].PID = id;
    pq->items[pq->size].arrival_time = value;
    pq->items[pq->size++].state_completed = state_comp;

    heapifyUp(pq, pq->size - 1);
}

int heapifyDown(PriorityQueue* pq, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if(left<pq->size && custom_compare(&pq->items[smallest], &pq->items[left])) {
        smallest = left;
    }

    if(right<pq->size && custom_compare(&pq->items[smallest], &pq->items[right])) {
        smallest = right;
    }

    if (smallest != index) {
        swap(&pq->items[index], &pq->items[smallest]);
        heapifyDown(pq, smallest);
    }
}

event_info dequeue_pq(PriorityQueue* pq) {
    event_info item;
    item.arrival_time = -1;
    item.PID = -1;

    if (!pq->size) {
        printf("Priority queue is empty\n");
        return item;
    }
    
    item = pq->items[0];
    pq->items[0] = pq->items[--pq->size];
    heapifyDown(pq, 0);
    return item;
}

event_info pq_top(PriorityQueue* pq) {
    event_info item;
    item.arrival_time = -1;
    item.PID = -1;

    if (!pq->size) {
        printf("Priority queue is empty\n");
        return item;
    }

    item = pq->items[0];
    return item;
}

int isPQEmpty(PriorityQueue* pq) {
    return pq->size == 0;
}

/*----------------------------------------------*/

int number_of_processes;
int cpu_idle;
double avg_wait_time;
long int total_turnaround_time;
long int cpu_idle_time, last_cpu_idle;
double cpu_utilization;
int global_time;

PriorityQueue Event_Queue;
queue Ready_Queue;   

int myRound(double x) {
    return (int)(x + 0.5);
}

// function to populate the PCB array by fetching details from the file
void populate_process_info(char* filename, process_info PCB[]) {
    FILE* fp = fopen(filename, "r");
    fscanf(fp, "%d", &number_of_processes);

    int line = 1;

    while(line <= number_of_processes) {
        int current_number = -2;
        int i = 0;
        
        PCB[line].burst_head = NULL;
        list* tail; 

        while(current_number != -1) {
            fscanf(fp, "%d", &current_number);

            if(i ==  0) PCB[line].id = current_number;
            else if(i == 1) PCB[line].arrival_time = current_number;
            else if(current_number != -1) 
            {
                if(i == 2) {
                    PCB[line].burst_head = (list*)malloc(sizeof(list));
                    PCB[line].burst_head->data = current_number;
                    tail = PCB[line].burst_head;
                }
                else {
                    tail->next = (list*)malloc(sizeof(list));
                    tail = tail->next;   
                    tail->data = current_number;
                }
            }   
            else {
                tail->next = NULL;
            }
            i++;
        }
        PCB[line].turnaround_time = 0;
        PCB[line].wait_time = 0;
        line++;
    }
    fclose(fp);

    return;
}

const char* state_messages[2] = {"upon arrival", "after IO completion"};

void Scheduler(process_info PCB[], int q) {
    while(isPQEmpty(&Event_Queue) == 0) {
        event_info current_event = pq_top(&Event_Queue);
        dequeue_pq(&Event_Queue);

        process_info* current_process = &PCB[current_event.PID]; 

        global_time = current_event.arrival_time;
        
        // FRESH_ARIVAL or IO_COMPLETED
        if(current_event.state_completed <= 1) {
            current_process->current_burst = (current_event.state_completed == FRESH_ARRIVAL) ? current_process->burst_head : current_process->current_burst->next;
            Ready_Queue = enqueue_q(Ready_Queue, current_event.PID);
            current_process->last_inserted = global_time;
#ifdef VERBOSE
            printf("%-10d: Process %d joins ready queue %s\n", global_time, current_event.PID, state_messages[current_event.state_completed]);
#endif
        }
        else if(current_event.state_completed == CPU_BURST_COMPLETED) {
            cpu_idle = 1;
            last_cpu_idle = global_time;
            // all bursts complete
            if(current_process->current_burst->next == NULL) {
                current_process->turnaround_time = global_time - current_process->arrival_time;
                printf("%-10d: Process %6d exits. Turnaround time = %4d (%d%%), Wait time = %d\n", 
                    global_time, current_process->id, current_process->turnaround_time, 
                    myRound(((current_process->turnaround_time*1.00)/(current_process->turnaround_time - current_process->wait_time)*100)), 
                    current_process->wait_time
                );
            }
            // burst not complete, move to next next burst (IO burst) and add the time to current global time before enqueing
            else {
                current_process->current_burst = current_process->current_burst->next;
                enqueue_pq(&Event_Queue, global_time + current_process->current_burst->data, current_event.PID, IO_COMPLETED);
            }
        }
        else if(current_event.state_completed == CPU_TIMED_OUT) {
            cpu_idle = 1;
            last_cpu_idle = global_time;
            Ready_Queue = enqueue_q(Ready_Queue, current_event.PID);
            current_process->last_inserted = global_time;
#ifdef VERBOSE
            printf("%-10d: Process %d joins ready queue after timeout\n", global_time, current_event.PID);
#endif
        }

        if(cpu_idle && !isQEmpty(Ready_Queue)) {
            cpu_idle = 0;
    
            int current = q_front(Ready_Queue);
            Ready_Queue = dequeue_q(Ready_Queue);

            PCB[current].wait_time += global_time - PCB[current].last_inserted;

            int cpu_burst_time = PCB[current].current_burst->data;
            int cpu_time = 0;
            if(cpu_burst_time <= q) {
                cpu_time = cpu_burst_time;
                enqueue_pq(&Event_Queue, global_time + cpu_burst_time, PCB[current].id, CPU_BURST_COMPLETED);
            }
            else {
                cpu_time = q;
                PCB[current].current_burst->data -= q;
                enqueue_pq(&Event_Queue, global_time + q, PCB[current].id, CPU_TIMED_OUT);
            }
#ifdef VERBOSE
            printf("%-10d: Process %d is scheduled to run for time %d\n", global_time, current, cpu_time);
#endif
            cpu_idle_time += (global_time - last_cpu_idle);
        }      
#ifdef VERBOSE
        if (cpu_idle)
            printf("%-10d: CPU goes idle\n", global_time);
#endif
    }
}

int main() {
    char process_details[21] = "proc.txt";

    char* headings[3] = {"\n**** FCFS Scheduling ****\n", "\n**** RR Scheduling with q = 10 ****\n", "\n**** RR Scheduling with q = 5 ****\n"};
    int quantum_values[3] = {INF, 10, 5};

    for(int i=0; i<3; i++) {
        cpu_idle = 1;
        avg_wait_time = 0;
        total_turnaround_time = 0;
        cpu_idle_time = 0;
        last_cpu_idle = 0;
        cpu_utilization = 0;
        global_time = 0;

        printf("%s", headings[i]);
#ifdef VERBOSE
        printf("%-10d: Starting\n", 0);
#endif

        process_info PCB[MAX];

        populate_process_info(process_details, PCB);

        init_fifo_queue(&Ready_Queue);
        init_pq(&Event_Queue);

        for(int i=1; i<=number_of_processes; i++) {
            enqueue_pq(&Event_Queue, PCB[i].arrival_time, PCB[i].id, FRESH_ARRIVAL);
        }

        Scheduler(PCB, quantum_values[i]); 

        for(int i=1; i<=number_of_processes; i++) {
            avg_wait_time += (PCB[i].wait_time/(1.00*number_of_processes));
        }

        total_turnaround_time = global_time;
        cpu_utilization = ((total_turnaround_time - cpu_idle_time)*100)/(total_turnaround_time*1.00);

        printf("\n");
        printf("Average wait time = %.2lf\n", avg_wait_time);
        printf("Total turnaround time = %ld\n", total_turnaround_time);
        printf("CPU idle time = %ld\n", cpu_idle_time);
        printf("CPU utilization = %.2lf%%\n", cpu_utilization);
    }
    printf("\n");
    return 0;
}