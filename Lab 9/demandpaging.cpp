#include <iostream>
#include <fstream>
#include <queue>
#include <deque>
#include <vector>
#include <algorithm>

using namespace std;

#define MAX_USER_FRAMES 12288
#define ESSENTIAL_PAGES 10

int num_processes, num_searches;
queue<int> swapped_out_list;
deque<int> ready_queue;
queue<unsigned short int> free_frame_list;

// variables to track statistics
int total_page_faults = 0, total_page_swaps = 0, total_page_accesses = 0, deg_of_multiprog;

vector<vector<unsigned short int> > page_table;

void load_essential_segments(int pid) {
    for(int i=0; i<ESSENTIAL_PAGES; i++) {
        page_table[pid][i] = free_frame_list.front() | (1<<15);
        free_frame_list.pop();
    }
}

void load_processes() {  
    for(int i=0; i<num_processes; i++) {
        ready_queue.push_back(i);
        load_essential_segments(i);
    }
}

void swap_out(int pid) {
    for(int i=0; i<(1<<11); i++) {
        // if frame is alloted, free it
        unsigned short int frame = page_table[pid][i];
        if(frame>>15 & 1) {
            frame = frame & ((1<<15) - 1);
            free_frame_list.push(frame); 
            page_table[pid][i] = 0;
        }   
    }
}

int allocate_frame(int pid, int index) {
    // if frame is allocated already, do nothing
    if((page_table[pid][10+(index>>10)]>>15) & 1) {
        return 0;
    }
    
    total_page_faults++;

    if(free_frame_list.empty()) {
        swap_out(pid);
        deg_of_multiprog = min(deg_of_multiprog, (int)ready_queue.size());
        swapped_out_list.push(pid);
        return -1;
    }
    
    page_table[pid][10+(index>>10)] = free_frame_list.front() | (1<<15);
    free_frame_list.pop();

    return 0;
}

int bin_search(int l, int r, int x, vector<int>& curr_search, int pid) {
    #ifdef VERBOSE
        cout << "\tSearch " << curr_search[pid] + 1 << " by Process " << pid << endl; 
    #endif  

    while(l<r) {
        int mid = l + (r-l)/2;
        total_page_accesses++;

        if(mid < x) 
            l = mid + 1;
        else 
            r = mid;

        if(allocate_frame(pid, mid) < 0)
            return -1;
    }

    curr_search[pid]++;
    return 0;
}

void simulate_demand_paging(vector<vector<int> >& search_indices, vector<int>& process_array_size, vector<int>& curr_search) {
    while(ready_queue.empty() == false) {
        int pid = ready_queue.front();
        ready_queue.pop_front();
        
        int search_index = search_indices[pid][curr_search[pid]];
        int process_size = process_array_size[pid]; 
        
        if(bin_search(0, process_size-1, search_index, curr_search, pid) < 0) {
            printf("+++ Swapping out process %4d  [%d active processes]\n", pid, (int)ready_queue.size());
            total_page_swaps++;
            continue;    
        }
        
        if(curr_search[pid] == num_searches) {
            swap_out(pid);
            if(swapped_out_list.empty() == false) {
                int pid = swapped_out_list.front();
                swapped_out_list.pop();
                load_essential_segments(pid);
                ready_queue.push_front(pid);

                printf("+++ Swapping in process %5d  [%d active processes]\n", pid, (int)ready_queue.size());
            }
        }
        else {
            ready_queue.push_back(pid);
        }
    }
}

int main() {
    // initialize the free frame list
    for(unsigned short int i=0; i<MAX_USER_FRAMES; i++) {
        free_frame_list.push(i);
    }

    ifstream search_file("search.txt");
    search_file >> num_processes >> num_searches;
    
    vector<vector<int> > search_indices(num_processes, vector<int>(num_searches));
    vector<int> process_array_size(num_processes), curr_search(num_processes, 0);

    for (int i = 0; i < num_processes; i++) {
        search_file >> process_array_size[i];
        for (int j = 0; j < num_searches; j++) {
            search_file >> search_indices[i][j];
        }
    }    
    
    cout << "+++ Simulation data read from file" << endl;

    page_table.resize(num_processes, vector<unsigned short int>(2048, 0));
    
    deg_of_multiprog = num_processes;
    
    load_processes();
    cout << "+++ Kernel data initialized" << endl;

    simulate_demand_paging(search_indices, process_array_size, curr_search);

    printf("+++ Page access summary\n");
    
    printf("\tTotal number of page accesses  =  %d\n", total_page_accesses);
    printf("\tTotal number of page faults    =  %d\n", total_page_faults);
    printf("\tTotal number of swaps          =  %d\n", total_page_swaps);
    printf("\tDegree of multiprogramming     =  %d\n", deg_of_multiprog);
}

