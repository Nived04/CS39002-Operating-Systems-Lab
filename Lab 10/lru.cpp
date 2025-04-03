#include <iostream> 
#include <vector>
#include <fstream>
#include <deque>

using namespace std;

#define NFFMIN 1000
#define ESSENTIAL_FRAMES 10
#define MAX_USER_FRAMES 12288
#define MAX_PROCESS_PAGES 2048

#define u_s_int unsigned short int

int NFF = 0;
int num_processes, num_searches;

// variables to track statistics
int total_page_faults = 0, total_page_accesses = 0, deg_of_multiprog;

typedef struct {
    u_s_int frame_number;
    int last_pid;
    int last_page_no;
}frame;

struct page_entry {
    u_s_int frame;
    u_s_int counter;

    page_entry() {
        frame = 0;
        counter = 0;
    }
};

struct page_details {
	int pid;
	int page_no;
	u_s_int frame_no;
};

typedef struct page_entry page_entry;
typedef struct page_details page_details;

class main_memory_unit {
    deque<frame> FFLIST;

public:
    main_memory_unit() {
        FFLIST.resize(MAX_USER_FRAMES);
        for(u_s_int i=0; i<MAX_USER_FRAMES; i++) {
            FFLIST[i].frame_number = i;
            FFLIST[i].last_pid = -1;
            FFLIST[i].last_page_no = -1;
        }
        NFF = MAX_USER_FRAMES;
    }

    void replace_page(int pid, page_details victim, int req = -1) {
        if(pid == -1) {
            return;
        }
        
		/*
		Check 1: if free frame exists with owner i and page no p, use it to store new page p.
		*/
       	for(auto it = FFLIST.begin(); it != FFLIST.end(); it++) {
			if((it->last_pid == pid) && (it->last_page_no == req)) {
				FFLIST.erase(it);
				FFLIST.push_back({victim.frame_no, victim.pid, victim.page_no});
				return;
			}
		}

		/*
		Check 2: check if there is a free frame with no owner. if so, use it
		*/
		for(auto it = FFLIST.begin(); it != FFLIST.end(); it++) {
			if(it->last_pid == -1) {
				FFLIST.erase(it);
                FFLIST.push_back({victim.frame_no, victim.pid, victim.page_no});
				return;
			}
		}

		/*
		Check 3: find a free frame with last owner i and use it for new page p.
		*/
		for(auto it = FFLIST.begin(); it != FFLIST.end(); it++) {
			if(it->last_pid == pid) {
				FFLIST.erase(it);
                FFLIST.push_back({victim.frame_no, victim.pid, victim.page_no});
				return;
			}
		}

		
		u_s_int random_frame = rand() % FFLIST.size();
		FFLIST.erase(FFLIST.begin() + random_frame);

		FFLIST.push_back({victim.frame_no, victim.pid, victim.page_no});

		return;
    }

    u_s_int get_free_frame(int pid = -1) {
        u_s_int frame_no = FFLIST.front().frame_number;
        FFLIST.pop_front();
        NFF--;

        return frame_no;
    }
};

class process {
    int pid;
    int array_size;
    int num_searches;
    vector<int> search_indices;
    vector<page_entry> page_table;
    main_memory_unit mmu;

public:
    process(int pid = -1, int array_size = 0, vector<int> search_indices = vector<int>()) {
        mmu = main_memory_unit();

        this->pid = pid;
        this->array_size = array_size;
        this->search_indices = search_indices;
        this->num_searches = search_indices.size();
        this->page_table.resize(MAX_PROCESS_PAGES, page_entry());

        this->allocate_essential_frames(this->pid);
    }

    void allocate_essential_frames(int pid) {
        for(int i = 0; i < ESSENTIAL_FRAMES; i++) {
            page_table[i].frame = mmu.get_free_frame();
            page_table[i].counter = 0xffff;
        }
        NFF -= ESSENTIAL_FRAMES;
    }

    page_details get_victim_page() {
        int min_cnt = 0xffff;
        int min_index = -1;
        for(int i = ESSENTIAL_FRAMES; i < MAX_PROCESS_PAGES; i++) {
            // victim page is a page with min history and valid bit = 1
            if((page_table[i].counter < min_cnt) && (page_table[i].frame>>15 & 1)) {
                min_cnt = page_table[i].counter;
                min_index = i;
            }
        }
        
        return {this->pid, min_index, page_table[min_index].frame & ((1<<14) - 1)};
    }
    
    int allocate_frame(int pid, int index) {
        int p = 10 + (index>>10);
        if((page_table[p].frame>>15) & 1) {
            return 0;
        }

        total_page_faults++;

        if(NFF == NFFMIN) {
            page_details q = get_victim_page();
            mmu.replace_page(this->pid, q, p);
        }
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
};

int main() {
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

    process* processes = new process[num_processes];    
    for(int i = 0; i < num_processes; i++) {
           processes[i] = process(i, process_array_size[i], search_indices[i]);
    }

}