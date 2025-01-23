#include "schedule.h"
#include <string.h>
#include <math.h>

#define MAX_PROCSSES 1000

int number_of_processes;
int global_time = 0;

PriorityQueue Event_Queue;
queue Ready_Queue;   

void populate_process_info(char* filename, process_info PCB[]) {
    FILE* fp = fopen(filename, "r");
    fscanf(fp, "%d", &number_of_processes);

    int line = 1;

    while(line <= number_of_processes)
    {
        int current_number = -2;
        int i = 0;
        
        PCB[line].burst_head = NULL;
        list* tail; 

        while(current_number != -1) 
        {
            fscanf(fp, "%d", &current_number);

            if(i ==  0) {
                PCB[line].id = current_number;
            }
            else if(i == 1) {
                PCB[line].arrival_time = current_number;
            }
            else if(current_number != -1) {
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
        PCB[line].number_of_bursts = i - 3;
        PCB[line].running_time = 0;
        line++;
    }
    fclose(fp);

    return;
}

void Schedule_Processes(process_info PCB[]) {
    for(int i=1; i<=number_of_processes; i++) {
        enqueue(&Event_Queue, PCB[i].arrival_time, PCB[i].id, FRESH_ARRIVAL);
        // printf("**\n");
    }

    printf("Processes scheduled\n");

    return;
}

void Scheduler(process_info PCB[], int q) {
    printf("Scheduler started\n");
    while(isEventQueueEmpty(&Event_Queue) == 0) {
        event_info current_event = peek(&Event_Queue);
        dequeue(&Event_Queue);

        // printf("Process ID: %d, time: %d, state: %d\n", current_event.PID, current_event.arrival_time, current_event.state_completed);

        process_info* current_process = &PCB[current_event.PID];

        if(current_event.state_completed == FRESH_ARRIVAL) {
            current_process->current_burst = current_process->burst_head;
            // printf("Inserting process ID in ready queue: %d\n", current_process->id);
            Ready_Queue = insert(Ready_Queue, current_process->id);
        }
        else if(current_event.state_completed == CPU_BURST_COMPLETED) {
            if(current_process->current_burst->next == NULL) {
                printf("Process (PID: %d) complete at time: %d\n", current_process->id, current_process->running_time + current_process->arrival_time);
            }
            else {
                current_process->current_burst = current_process->current_burst->next;
                current_process->running_time += current_process->current_burst->data;
                enqueue(&Event_Queue, current_process->running_time + current_process->arrival_time, current_event.PID, IO_COMPLETED);
            }
        }
        else if(current_event.state_completed == IO_COMPLETED) {
            current_process->current_burst = current_process->current_burst->next;
            Ready_Queue = insert(Ready_Queue, current_event.PID);
        }
        else if(current_event.state_completed == CPU_TIMED_OUT) {
            // printf("Burst left: %d\n", current_process->current_burst->data);
            Ready_Queue = insert(Ready_Queue, current_event.PID);
        }  

        if(isEmpty(Ready_Queue) == 0) {
            int current = front(Ready_Queue);
            Ready_Queue = pop(Ready_Queue);
            // printf("Current process ID in ready queue: %d\n", current);

            int cpu_burst_time = PCB[current].current_burst->data;
            int cpu_time = 0;
            if(cpu_burst_time <= q) {
                PCB[current].running_time += cpu_burst_time;
                cpu_time = cpu_burst_time;
                enqueue(&Event_Queue, PCB[current].running_time + PCB[current].arrival_time, PCB[current].id, CPU_BURST_COMPLETED);
            }
            else {
                PCB[current].running_time += q;
                cpu_time = q;
                PCB[current].current_burst->data -= q;
                enqueue(&Event_Queue, PCB[current].running_time + PCB[current].arrival_time, PCB[current].id, CPU_TIMED_OUT);
            }
        } 
    }
}

int main() {
    char process_details[9] = "input.txt";

    process_info PCB[MAX_PROCSSES];

    populate_process_info(process_details, PCB);

    int quanta = 10;
    Ready_Queue = init();
    init(Event_Queue);

    Schedule_Processes(PCB);

    Scheduler(PCB, quanta); 
}