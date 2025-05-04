#include <iostream> 
#include <vector>
#include <fstream>
#include <queue>
#include <deque>
#include <iomanip>

using namespace std;

#define NFFMIN 1000
#define ESSENTIAL_FRAMES 10
#define MAX_USER_FRAMES 12288
#define MAX_PROCESS_PAGES 2048

#define u_s_int unsigned short int

int NFF = 0;
int num_processes, num_searches;

// variables to track statistics
int total_page_faults = 0, total_page_accesses = 0, deg_of_multiprog, total_page_replacements = 0;
int total_attempts[4] = {0, 0, 0, 0};

// frame structure
typedef struct {
    u_s_int frame_number;
    int last_pid;
    int last_page_no;
}frame;

// page entry structure
struct page_entry {
    u_s_int frame;
    u_s_int counter;

    page_entry() {
        frame = 0;
        counter = 0;
    }
};

// utility structure to hold page details (used for getting victim page details)
struct page_details {
	int pid;
	int page_no;
	u_s_int frame_no;
    u_s_int counter;
};

typedef struct page_entry page_entry;
typedef struct page_details page_details;

// singleton class for main memory unit
class main_memory_unit {
private:
    deque<frame> FFLIST;
    static main_memory_unit* instance;
    
    main_memory_unit() {
        FFLIST.resize(MAX_USER_FRAMES);
        for(u_s_int i=0; i<MAX_USER_FRAMES; i++) {
            FFLIST[i].frame_number = i;
            FFLIST[i].last_pid = -1;
            FFLIST[i].last_page_no = -1;
        }
        NFF = MAX_USER_FRAMES;
    }

public:
    static main_memory_unit* getInstance() {
        if (instance == nullptr) {
            instance = new main_memory_unit();
        }
        return instance;
    }

    u_s_int replace_page(int pid, page_details victim, int req, int arr[]) {
		/*
		Check 1: if free frame exists with owner i and page no p, use it to store new page p.
		*/
       	for(auto it = FFLIST.begin(); it != FFLIST.end(); it++) {
			if((it->last_pid == pid) && (it->last_page_no == req)) {
                u_s_int frame_no = it->frame_number;
                #ifdef VERBOSE
                    cout << "\t\tAttempt 1: Page found in free frame " << frame_no << endl;
                #endif
				FFLIST.erase(it);
				FFLIST.push_back({victim.frame_no, victim.pid, victim.page_no});
                arr[0]++;
                total_attempts[0]++;
				return frame_no;
			}
		}

		/*
		Check 2: check if there is a free frame with no owner. if so, use it
		*/
		for(auto it = FFLIST.rbegin(); it != FFLIST.rend(); it++) {
			if(it->last_pid == -1) {
                u_s_int frame_no = it->frame_number;
                #ifdef VERBOSE
                    cout << "\t\tAttempt 2: Free frame " << frame_no << " owned by no process found" << endl;
                #endif
				FFLIST.erase(prev(it.base()));
                FFLIST.push_back({victim.frame_no, victim.pid, victim.page_no});
                arr[1]++;
                total_attempts[1]++;
				return frame_no;
			}
		}

		/*
		Check 3: find a free frame with last owner i and use it for new page p.
		*/
		for(auto it = FFLIST.rbegin(); it != FFLIST.rend(); it++) {
			if(it->last_pid == pid) {
                u_s_int frame_no = it->frame_number;
                #ifdef VERBOSE
                    cout << "\t\tAttempt 3: Own page " << it->last_page_no << " found in free frame " << frame_no << endl;
                #endif
				FFLIST.erase(prev(it.base()));
                FFLIST.push_back({victim.frame_no, victim.pid, victim.page_no});
                arr[2]++;
                total_attempts[2]++;
				return frame_no;
			}
		}

		
		u_s_int random_frame = rand() % FFLIST.size();
        u_s_int frame_no = FFLIST[random_frame].frame_number;

        #ifdef VERBOSE
            cout << "\t\tAttempt 4: Free frame " << frame_no << " owned by Process " << FFLIST[random_frame].last_pid << " chosen" << endl;
        #endif
        
		FFLIST.erase(FFLIST.begin() + random_frame);
		FFLIST.push_back({victim.frame_no, victim.pid, victim.page_no});
        arr[3]++;
        total_attempts[3]++;
		return frame_no;
    }

    // get the first free frame: the front of the array (deque)
    u_s_int get_free_frame() {
        u_s_int frame_no = FFLIST.front().frame_number;
        FFLIST.pop_front();
        NFF--;

        return frame_no;
    }

    // add a frame back to the free frame list
    void add_frame_back(u_s_int frame_no) {
        FFLIST.push_back({frame_no, -1, -1});
        NFF++;
    }
};

main_memory_unit* main_memory_unit::instance = nullptr;

/*
process class;

each process stores its page_table and interacts with the MMU for frame management
each process tracks its own page faults, accesses, and replacements
*/
class process {
    int pid;
    int array_size;
    int num_searches;
    int curr_search;
    vector<int> search_indices;
    vector<page_entry> page_table;
    main_memory_unit* mmu;

public:
    // public variables for allowing interface to retreive statistics
    int accesses;
    int page_faults;
    int page_replacements;
    int attempts[4];

    process() {}
    process(int pid, int array_size, vector<int> search_indices) {
        mmu = main_memory_unit::getInstance();

        this->pid = pid;
        this->array_size = array_size;
        this->search_indices = search_indices;
        this->num_searches = search_indices.size();
        this->page_table.resize(MAX_PROCESS_PAGES, page_entry());
        this->curr_search = 0;

        this->accesses = 0;
        this->page_faults = 0;
        this->page_replacements = 0;        
        for(int i=0; i<4; i++) {
            this->attempts[i] = 0;
        }

        this->allocate_essential_frames();
    }

    void allocate_essential_frames() {
        for(int i = 0; i < ESSENTIAL_FRAMES; i++) {
            page_table[i].frame = mmu->get_free_frame();
            page_table[i].counter = 0xffff;
        }
    }

    page_details get_victim_page() {
        int min_index = -1;
        for(int i = ESSENTIAL_FRAMES; i < MAX_PROCESS_PAGES; i++) {
            // victim page must be valid
            if((page_table[i].frame>>15 & 1)) {
                if(min_index == -1) {
                    min_index = i;
                }
                else if(page_table[i].counter < page_table[min_index].counter) {
                    min_index = i;
                }
            }
        }
        u_s_int min_frame = page_table[min_index].frame & ((1<<14) - 1);

        // return the victim page details
        return {this->pid, min_index, min_frame, page_table[min_index].counter};
    }
    
    // allocate a frame for the page
    int allocate_frame(int index) {
        int p = 10 + (index>>10);

        // if valid, can directly use that frame
        if((page_table[p].frame>>15) & 1) {
            return 0;
        }

        // else it is a page fault
        total_page_faults++;
        this->page_faults++;

        // if free frame decreases to threshold, need to call for page replacement
        if(NFF <= NFFMIN) {
            this->page_replacements++;
            total_page_replacements++;

            page_details q = get_victim_page();
            // update q's page table
            page_table[q.page_no].frame &= ((1<<14) - 1);

            // update p's page table
            page_table[p].frame = (mmu->replace_page(this->pid, q, p, this->attempts));

            #ifdef VERBOSE
                cout << "\tFault on Page " << setw(4) << p << ": To replace Page " << q.page_no << " at Frame " << q.frame_no << " [history = " << q.counter << "]" << endl;
            #endif
        }
        // otherwise just allocate a fresh free frame
        else {
            page_table[p].frame = mmu->get_free_frame();

            #ifdef VERBOSE
                cout << "\tFault on Page " << setw(4) << p << ": Free frame " << (page_table[p].frame & ((1<<14) - 1)) << " found" << endl;
            #endif
        }

        // update history to be maximum
        page_table[p].counter = 0xffff;
        return 0;
        
    }
    
    bool bin_search() {
        #ifdef VERBOSE
        cout << "+++ Process " << pid << ": Search " << curr_search + 1 << endl; 
        #endif  
        
        int l = 0, r = array_size - 1;
        int x = search_indices[curr_search];
        
        while(l<r) {
            int mid = (r+l)/2;
            total_page_accesses++;
            this->accesses++;
            
            if(mid < x) 
            l = mid + 1;
            else 
            r = mid;
            
            allocate_frame(mid);
            
            // set the reference and valid bit to 1
            page_table[10+(mid>>10)].frame |= (3<<14); 
        }

        for(int i=ESSENTIAL_FRAMES; i<MAX_PROCESS_PAGES; i++) {
            if((page_table[i].frame>>15) & 1) {
                // decrement history value by right shifting once
                page_table[i].counter = (page_table[i].counter>>1);
                
                if((page_table[i].frame>>14) & 1) {
                    page_table[i].counter |= (1<<15);
                }
            }
            // set reference bit of all pages to 0
            page_table[i].frame &= (~(u_s_int)(1<<14));
        }
        
        curr_search++;

        if(curr_search == num_searches) {
            remove_frames();
            return 0;
        }

        return 1;
    }

    // remove all frames from the page table when the process ends
    void remove_frames() {
        for(int i=0; i<MAX_PROCESS_PAGES; i++) {
            if(page_table[i].frame>>15 & 1) {
                page_table[i].frame &= ((1<<15) - 1);
                mmu->add_frame_back(page_table[i].frame);
            }
        }
    }
};

queue<int> rr_queue;

// round robin queue for process scheduling
void simulate_page_replacement(process* processes, int num_processes) {
    while(!rr_queue.empty()) {
        int pid = rr_queue.front();
        rr_queue.pop();

        int ret = processes[pid].bin_search();

        if(ret) {
            rr_queue.push(pid);
        }
    }
}

void print_stat_values(int a, int b, int c, const int arr[]) {
    cout << setw(13) << a << setw(7) << b << " (" << fixed << setprecision(2) 
        << (b * 100.0 / a) << "%) " 
        << setw(7) << c << " (" << fixed << setprecision(2) 
        << (c * 100.0 / a) << "%) " 
        << setw(7) << arr[0] << " + " << setw(3) << arr[1] << " + " 
        << setw(3) << arr[2] << " + " << setw(3) << arr[3] << "  (" 
        << fixed << setprecision(2) << (arr[0] * 100.0 / c) << "% + " 
        << fixed << setprecision(2) << (arr[1] * 100.0 / c) << "% + " 
        << fixed << setprecision(2) << (arr[2] * 100.0 / c) << "% + " 
        << fixed << setprecision(2) << (arr[3] * 100.0 / c) << "%)" 
        << endl;
}

int main() {
    ifstream search_file("search.txt");
    search_file >> num_processes >> num_searches;
    
    vector<vector<int> > search_indices(num_processes, vector<int>(num_searches));
    vector<int> process_array_size(num_processes);

    for (int i = 0; i < num_processes; i++) {
        search_file >> process_array_size[i];
        for (int j = 0; j < num_searches; j++) {
            search_file >> search_indices[i][j];
        }
    }    
    
    process* processes = new process[num_processes];    
    for(int i = 0; i < num_processes; i++) {
           processes[i] = process(i, process_array_size[i], search_indices[i]);
           rr_queue.push(i);
    }  

    simulate_page_replacement(processes, num_processes);

	cout << "+++ Page access summary" << endl;
	cout << "\tPID     Accesses        Faults         Replacements                        Attempts" << endl;
	for(int i=0; i<num_processes; i++) {
        cout << "\t" << setw(3) << i;
        print_stat_values(processes[i].accesses, processes[i].page_faults, processes[i].page_replacements, processes[i].attempts);
    }

    cout << endl;

    cout << "\tTotal"; 
    print_stat_values(total_page_accesses, total_page_faults, total_page_replacements, total_attempts);

}