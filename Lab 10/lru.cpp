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
    u_s_int counter;
};

typedef struct page_entry page_entry;
typedef struct page_details page_details;

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
				FFLIST.erase(it);
				FFLIST.push_back({victim.frame_no, victim.pid, victim.page_no});
                arr[0]++;
				return it->frame_number;
			}
		}

		/*
		Check 2: check if there is a free frame with no owner. if so, use it
		*/
		for(auto it = FFLIST.begin(); it != FFLIST.end(); it++) {
			if(it->last_pid == -1) {
				FFLIST.erase(it);
                FFLIST.push_back({victim.frame_no, victim.pid, victim.page_no});
                arr[1]++;
				return it->frame_number;
			}
		}

		/*
		Check 3: find a free frame with last owner i and use it for new page p.
		*/
		for(auto it = FFLIST.begin(); it != FFLIST.end(); it++) {
			if(it->last_pid == pid) {
				FFLIST.erase(it);
                FFLIST.push_back({victim.frame_no, victim.pid, victim.page_no});
                arr[2]++;
				return it->frame_number;
			}
		}

		
		u_s_int random_frame = rand() % FFLIST.size();
        u_s_int frame_no = FFLIST[random_frame].frame_number;
		FFLIST.erase(FFLIST.begin() + random_frame);

		FFLIST.push_back({victim.frame_no, victim.pid, victim.page_no});

        arr[3]++;
		return frame_no;
    }

    u_s_int get_free_frame(int pid = -1) {
        u_s_int frame_no = FFLIST.front().frame_number;
        FFLIST.pop_front();
        NFF--;

        return frame_no;
    }

    void add_frame_back(u_s_int frame_no) {
        FFLIST.push_back({frame_no, -1, -1});
        NFF++;
    }
};

main_memory_unit* main_memory_unit::instance = nullptr;

class process {
    int pid;
    int array_size;
    int num_searches;
    vector<int> search_indices;
    vector<page_entry> page_table;
    main_memory_unit* mmu;

public:
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

        this->accesses = 0;
        this->page_faults = 0;
        this->page_replacements = 0;        
        for(int i=0; i<4; i++) {
            this->attempts[i] = 0;
        }

        this->allocate_essential_frames(this->pid);
    }

    void allocate_essential_frames(int pid) {
        for(int i = 0; i < ESSENTIAL_FRAMES; i++) {
            page_table[i].frame = mmu->get_free_frame();
            page_table[i].counter = 0xffff;
        }
    }

    page_details get_victim_page() {
        u_s_int min_cnt = 0xffff;
        int min_index = -1;
        for(int i = ESSENTIAL_FRAMES; i < MAX_PROCESS_PAGES; i++) {
            // victim page is a page with min history and valid bit = 1
            if((page_table[i].counter < min_cnt) && (page_table[i].frame>>15 & 1)) {
                min_cnt = page_table[i].counter;
                min_index = i;
            }
        }
        u_s_int min_frame = page_table[min_index].frame & ((1<<14) - 1);

        return {this->pid, min_index, min_frame, page_table[min_index].counter};
    }
    
    int allocate_frame(int index) {
        int p = 10 + (index>>10);

        if((page_table[p].frame>>15) & 1) {
            return 0;
        }

        total_page_faults++;
        this->page_faults++;

        if(NFF <= NFFMIN) {
            this->page_replacements++;
            page_details q = get_victim_page();
            page_table[q.page_no].frame &= ((1<<14) - 1);
            page_table[p].frame = (mmu->replace_page(this->pid, q, p, this->attempts)) | (1<<15);
            page_table[p].counter = 0xffff;

            #ifdef VERBOSE
                cout << "\tFault on Page " << setw(4) << p << ": To replace Page " << q.page_no << " at Frame " << q.frame_no << " [history = " << q.counter << "]" << endl;
            #endif

        }
        else {
            page_table[p].frame = mmu->get_free_frame(this->pid);

            #ifdef VERBOSE
                cout << "\tFault on Page " << setw(4) << p << ": Free frame " << (page_table[p].frame & ((1<<14) - 1)) << " found" << endl;
            #endif
        }

        return 0;

    }
    
    int bin_search(int l, int r, int x, vector<int>& curr_search, int pid) {
        #ifdef VERBOSE
            cout << "+++ Process " << pid << ": Search " << curr_search[pid] + 1 << endl; 
        #endif  
        
        while(l<r) {
            int mid = l + (r-l)/2;
            total_page_accesses++;
            this->accesses++;
    
            if(mid < x) 
                l = mid + 1;
            else 
                r = mid;
    
            allocate_frame(mid);

            page_table[10 + (mid>>10)].counter = 0xffff;
            page_table[10 + (mid>>10)].frame |= (3<<14); // set the reference and valid bit to 1
        }

        for(int i=ESSENTIAL_FRAMES; i<MAX_PROCESS_PAGES; i++) {
            if((page_table[i].frame>>15) & 1) {
                page_table[i].counter = (page_table[i].counter>>1);
                
                if((page_table[i].frame>>14) & 1) {
                    page_table[i].counter |= (1<<15);
                }
            }
            // set reference bit of all pages to 0
            page_table[i].frame &= (~(u_s_int)(1<<14));
        }
        
        curr_search[pid]++;
        return 0;
    }

    void remove_frames() {
        for(int i=ESSENTIAL_FRAMES; i<MAX_PROCESS_PAGES; i++) {
            if(page_table[i].frame>>15 & 1) {
                page_table[i].frame &= ((1<<15) - 1);
                mmu->add_frame_back(page_table[i].frame);
            }
        }
    }
};

queue<int> rr_queue;

void simulate_page_replacement(process* processes, int num_processes, vector<vector<int> >& search_indices, vector<int>& process_array_size, vector<int>& curr_search) {
    while(!rr_queue.empty()) {
        int pid = rr_queue.front();
        rr_queue.pop();

        int search_index = search_indices[pid][curr_search[pid]];
        int array_size = process_array_size[pid];

        if(processes[pid].bin_search(0, array_size-1, search_index, curr_search, pid) < 0) {
            continue;
        }

        if(curr_search[pid] == num_searches) {
            processes[pid].remove_frames();
        }
        else {
            rr_queue.push(pid);
        }
    }
}

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
    
    process* processes = new process[num_processes];    
    for(int i = 0; i < num_processes; i++) {
           processes[i] = process(i, process_array_size[i], search_indices[i]);
           rr_queue.push(i);
    }  

    simulate_page_replacement(processes, num_processes, search_indices, process_array_size, curr_search);

    int total_replacements = 0, total_attempts[4] = {0, 0, 0, 0};

	cout << "+++ Page access summary" << endl;
	cout << "\tPID     Accesses        Faults         Replacements                        Attempts" << endl;
	for(int i=0; i<num_processes; i++) {
        cout << "\t" << setw(3) << i << setw(13) << processes[i].accesses  
            << setw(7) << processes[i].page_faults << " (" << fixed << setprecision(2) 
            << (processes[i].page_faults * 100.0 / processes[i].accesses) << "%) " 
            << setw(7) << processes[i].page_replacements << " (" << fixed << setprecision(2) 
            << (processes[i].page_replacements * 100.0 / processes[i].accesses) << "%) " 
            << setw(7) << processes[i].attempts[0] << " + " << setw(3) << processes[i].attempts[1] << " + " 
            << setw(3) << processes[i].attempts[2] << " + " << setw(3) << processes[i].attempts[3] << "  (" 
            << fixed << setprecision(2) << (processes[i].attempts[0] * 100.0 / processes[i].page_replacements) << "% + " 
            << fixed << setprecision(2) << (processes[i].attempts[1] * 100.0 / processes[i].page_replacements) << "% + " 
            << fixed << setprecision(2) << (processes[i].attempts[2] * 100.0 / processes[i].page_replacements) << "% + " 
            << fixed << setprecision(2) << (processes[i].attempts[3] * 100.0 / processes[i].page_replacements) << "%)" 
            << endl;
        
        total_page_accesses += processes[i].accesses;
        total_replacements += processes[i].page_replacements;
        total_attempts[0] += processes[i].attempts[0];
        total_attempts[1] += processes[i].attempts[1];
        total_attempts[2] += processes[i].attempts[2];
        total_attempts[3] += processes[i].attempts[3];
    }

    cout << "\tTotal" << setw(13) << total_page_accesses 
        << setw(7) << total_page_faults << " (" << fixed << setprecision(2) 
        << (total_page_faults * 100.0 / total_page_accesses) << "%) " 
        << setw(7) << total_replacements << " (" << fixed << setprecision(2) 
        << (total_replacements * 100.0 / total_page_accesses) << "%) " 
        << setw(7) << total_attempts[0] << " + " << setw(3) << total_attempts[1] << " + " 
        << setw(3) << total_attempts[2] << " + " << setw(3) << total_attempts[3] << "  (" 
        << fixed << setprecision(2) << (total_attempts[0] * 100.0 / total_replacements) << "% + " 
        << fixed << setprecision(2) << (total_attempts[1] * 100.0 / total_replacements) << "% + " 
        << fixed << setprecision(2) << (total_attempts[2] * 100.0 / total_replacements) << "% + " 
        << fixed << setprecision(2) << (total_attempts[3] * 100.0 / total_replacements) << "%)" 
        << endl;

}