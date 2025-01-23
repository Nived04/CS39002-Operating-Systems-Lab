#include <stdio.h>
#include <stdlib.h>
#define MAX 1000
#define MAXLEN 1000

typedef struct linked_list {
    int data;
    struct linked_list *next;
}list;

// #define ARRIVAL 1
// #define END 2
// #define CPU_BURST_END 3
// #define CPU_TIMEOUT 4
// #define REARRIVAL 5

typedef enum {
    FRESH_ARRIVAL,
    CPU_BURST_COMPLETED,
    IO_COMPLETED,
    CPU_TIMED_OUT,
}process_state;

typedef struct process_info{
    int id;
    int arrival_time;
    int number_of_bursts;
    int burst_type;
    int running_time;
    int wait_time;
    list *burst_head;
    list *current_burst;
}process_info;

typedef struct {
    int PID;
    int arrival_time;
    process_state state_completed;
}event_info;

typedef struct {
    int arr[MAXLEN]; // maximum size of the queue is MAXLEN-1
    int front;
    int back;
}queue;

queue init() {
    queue Q;

    Q.front = 0;
    Q.back = MAXLEN - 1;

    return Q;
}

int isEmpty(queue Q) {
    return (Q.front == (Q.back + 1) % MAXLEN);
}

int isFull(queue Q) {
    return (Q.front == (Q.back + 2) % MAXLEN);
}

int  queue_size(queue Q) {
    return (Q.back - Q.front) % MAXLEN + 1;
} 

int front(queue Q) {
    if (isEmpty(Q))
    {
        printf("\nfront: Empty Queue\n");
        return -1;
    }

    return Q.arr[Q.front];
}

queue insert(queue Q, int id) {
    if (isFull(Q))
    {
        printf("\nenqueue: Full Queue\n");
        return Q;
    }

    Q.back = (++Q.back) % MAXLEN;
    Q.arr[Q.back] = id;

    return Q;
}

queue pop(queue Q) {
    if (isEmpty(Q))
    {
        printf("\ndequeue: Empty Queue\n");
        return Q;
    }

    Q.front = (++Q.front) % MAXLEN;
    return Q;
}

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

void heapifyUp(PriorityQueue* pq, int index) {
    if (index && pq->items[(index - 1) / 2].arrival_time > pq->items[index].arrival_time) {
        swap(&pq->items[(index - 1) / 2], &pq->items[index]);
        heapifyUp(pq, (index - 1) / 2);
    }
    else if(index && pq->items[(index - 1) / 2].arrival_time == pq->items[index].arrival_time) {
        if(
            index 
            && (pq->items[index].state_completed == FRESH_ARRIVAL || pq->items[index].state_completed == IO_COMPLETED)
            && (pq->items[(index-1) / 2].state_completed == CPU_TIMED_OUT)
        ) {
            swap(&pq->items[(index - 1) / 2], &pq->items[index]);
            heapifyUp(pq, (index - 1) / 2);
        }
        else if(
            index
            && pq->items[index].state_completed == IO_COMPLETED 
            && pq->items[(index-1) / 2].state_completed == FRESH_ARRIVAL 
            && pq->items[(index-1) / 2].PID > pq->items[index].PID
        ){
            swap(&pq->items[(index - 1) / 2], &pq->items[index]);
            heapifyUp(pq, (index - 1) / 2);
        }
    }
}

void enqueue(PriorityQueue* pq, int value, int id, int state_comp) {
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

    if (left < pq->size && pq->items[left].arrival_time < pq->items[smallest].arrival_time)
        smallest = left;
    else if(left < pq->size && pq->items[left].arrival_time == pq->items[smallest].arrival_time) {
        if(
            (pq->items[left].state_completed == FRESH_ARRIVAL || pq->items[left].state_completed == IO_COMPLETED)
            && pq->items[smallest].state_completed == CPU_TIMED_OUT
        ) 
        {
            smallest = left;
        }
        else if(
            pq->items[left].state_completed == IO_COMPLETED 
            && pq->items[smallest].state_completed == FRESH_ARRIVAL 
            && pq->items[smallest].PID > pq->items[left].PID
        ) 
        {
            smallest = left;
        }
    }

    if (right < pq->size && pq->items[right].arrival_time < pq->items[smallest].arrival_time)
        smallest = right;
    else if(left < pq->size && pq->items[left].arrival_time == pq->items[smallest].arrival_time) {
        if(
            (pq->items[left].state_completed == FRESH_ARRIVAL || pq->items[left].state_completed == IO_COMPLETED)
            && pq->items[smallest].state_completed == CPU_TIMED_OUT
        ) 
        {
            smallest = right;
        }
        else if(
            pq->items[left].state_completed == IO_COMPLETED 
            && pq->items[smallest].state_completed == FRESH_ARRIVAL 
            && pq->items[smallest].PID > pq->items[right].PID
        ) 
        {
            smallest = right;
        }
    }

    if (smallest != index) {
        swap(&pq->items[index], &pq->items[smallest]);
        heapifyDown(pq, smallest);
    }
}

event_info dequeue(PriorityQueue* pq) {
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

event_info peek(PriorityQueue* pq) {
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

int isEventQueueEmpty(PriorityQueue* pq) {
    return pq->size == 0;
}