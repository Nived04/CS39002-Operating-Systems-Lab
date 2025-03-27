#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <pthread.h>
#include <vector>
#include <inttypes.h>
#include <unistd.h>
#include <algorithm>
#include <map>
#include <cassert>
using namespace std;

#define ADDITIONAL 1
#define RELEASE 2
#define QUIT 3

vector<int> available;
vector<vector<int>> need, alloc;
vector<bool> finished;
pthread_barrier_t BOS, REQB;
vector<pthread_barrier_t> ACKB;
pthread_mutex_t pmtx, rmtx;

typedef struct {
    int user_id;
    int type;
    vector<int> REQ;
}request;

request global_req;

typedef struct {
    int value;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
}semaphore;

vector<semaphore> user_sem;

void down(semaphore *s) {
    pthread_mutex_lock(&(s->mtx));
    while(s->value == 0) pthread_cond_wait(&(s->cv), &(s->mtx));
    s->value = 0;
    pthread_mutex_unlock(&(s->mtx));
}

void up(semaphore* s) {
    pthread_mutex_lock(&(s->mtx));
    s->value = 1;
    pthread_cond_signal(&(s->cv));
    pthread_mutex_unlock(&(s->mtx));
}

void safe_print(string msg) {
    pthread_mutex_lock(&pmtx);
    cout << msg << endl;
    pthread_mutex_unlock(&pmtx);
}

void vector_op(vector<int> &a, vector<int> &b, char op) {
    int n = a.size();
    for(int i=0; i<n; i++) {
        if(op == '+') a[i] += b[i];
        else if(op == '-') a[i] -= b[i];
    }
}

bool vector_comp(vector<int> &a, vector<int> &b, string op) {
    int n = a.size();
    for(int i=0; i<n; i++) {
        if(op == "<=" && a[i] > b[i]) return false;
        else if(op == "<" && a[i] >= b[i]) return false;
        else if(op == ">=" && a[i] < b[i]) return false;
        else if(op == ">" && a[i] <= b[i]) return false;
    }
    return true;
}

bool isSafe(vector<int> &available, vector<vector<int>> &alloc, vector<vector<int>> &need){
    int n = alloc.size();
    vector<int> work = available;
    vector<bool> fin = finished;

    while(true){
        bool flag = false;
        for(int i=0; i<n; i++){
            if(!fin[i] && vector_comp(need[i], work, "<=")){
                flag = true;
                fin[i] = true;
                vector_op(work, alloc[i], '+');
            }
        }
        if(!flag) break;
    }

    bool ret;
    for(int i=0; i<n; i++){
        if(!fin[i]){
            ret = false;
            break;
        }
        ret = true;
    }

    return ret;
}

void* user(void * arg) {
    int user_id = *((int*)arg);
    safe_print("\tThread " + to_string(user_id) + " born");
    pthread_barrier_wait(&BOS);

    string user_id_str;

    // get 2 digit user id
    if(user_id < 10) {
        user_id_str = "0" + to_string(user_id);
    } else {
        user_id_str = to_string(user_id);
    }

    string filename = "./input/thread" + user_id_str + ".txt";
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Error: Unable to open file" << endl;
        exit(1);
    }

    
    int m = need[user_id].size();
    for(int i=0; i<m; i++) {
        file >> need[user_id][i];
    }

    user_id_str = to_string(user_id);
    
    while(1) {
        int delay; file >> delay;
        usleep(delay * 10000);

        char req_type; file >> req_type;
        int type;

        if(req_type == 'R') {
            vector<int> REQ(m);
            int is_pos = 0;
            for(int i=0; i<m; i++) {
                file >> REQ[i];
                if(REQ[i] > 0) {
                    is_pos = 1;
                }
            }
            type = (is_pos) ? ADDITIONAL : RELEASE;
            string type_str = (type == ADDITIONAL) ? "ADDITIONAL" : "RELEASE";
            pthread_mutex_lock(&rmtx);
            global_req = {user_id, type, REQ};

            safe_print("\tThread " + user_id_str + " sends resource request: type = " + type_str);
        }
        else if(req_type == 'Q') {
            type = QUIT;
            pthread_mutex_lock(&rmtx);
            global_req = {user_id, type, {}};
        }

        pthread_barrier_wait(&REQB);
        pthread_barrier_wait(&ACKB[user_id]);
        pthread_mutex_unlock(&rmtx);

        if(type == ADDITIONAL) {
            down(&user_sem[user_id]);
            safe_print("\tThread " + user_id_str + " is granted its last resource request");
        }
        else if(type == RELEASE) {
            safe_print("\tThread " + user_id_str + " is done with its resource release request");
        }
        else {
            safe_print("\tThread " +  user_id_str + " going to quit");
            break;
        }
    }

    file.close();
    return NULL;
}

void* master(void *) {
    ifstream file("./input/system.txt");
    if (!file.is_open()) {
        cout << "Error: Unable to open file" << endl;
        exit(1);
    }

    int m, n;
    file >> m >> n;
    available.resize(m);

    for (int i = 0; i < m; ++i) {
        file >> available[i];
    }

    need.resize(n, vector<int>(m));
    alloc.resize(n, vector<int>(m));
    user_sem.resize(n);
    finished.resize(n, false);
    ACKB.resize(n);

    for(int i=0; i<n; i++) {
        user_sem[i] = (semaphore){0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
    }

    file.close();

    vector<pthread_t> threads(n);

    pthread_barrier_init(&BOS, NULL, n + 1);
    pthread_barrier_init(&REQB, NULL, 2);
    pthread_mutex_init(&pmtx, NULL);
    pthread_mutex_init(&rmtx, NULL);

    vector<int> waiting_users;
    for(int i=0; i<n; i++) {
        pthread_barrier_init(&ACKB[i], NULL, 2);
        int *arg = (int*)malloc(sizeof(int));
        *arg = i;
        pthread_create(&threads[i], NULL, user, arg);
        waiting_users.push_back(i);
    }

    pthread_barrier_wait(&BOS); 

    vector<request> Q;
    while(1) {
        pthread_barrier_wait(&REQB);
        request req = global_req;
        pthread_barrier_wait(&ACKB[req.user_id]);

        if(req.type == QUIT) {
            for(int i=0; i<m; i++) {
                available[i] += alloc[req.user_id][i];
                alloc[req.user_id][i] = 0;
                need[req.user_id][i] += alloc[req.user_id][i];
            }
            auto it = find(waiting_users.begin(), waiting_users.end(), req.user_id);
            waiting_users.erase(it);
            safe_print("Master thread releases resources of thread " + to_string(req.user_id));
            finished[req.user_id] = true;

            string print_waiting = "\t\tWaiting threads:";
            for(auto it = Q.begin(); it != Q.end(); it++) {
                print_waiting += (" " + to_string(it->user_id));
            }
            safe_print(print_waiting);

            string print_left = to_string(waiting_users.size()) + " threads left:";
            for(auto it = waiting_users.begin(); it != waiting_users.end(); it++) {
                print_left += (" " + to_string(*it));
            }
            safe_print(print_left);

            string avail = "Available resources:";
            for(int i=0; i<m; i++) {
                avail += (" " + to_string(available[i]));
            }
            safe_print(avail);

            if(waiting_users.size() == 0) break;
        }
        else if(req.type == RELEASE) {
            vector_op(available, req.REQ, '-');
            vector_op(alloc[req.user_id], req.REQ, '+');
            vector_op(need[req.user_id], req.REQ, '-');
        }
        else {
            for(int i=0; i<m; i++) {
                if(req.REQ[i] < 0) {
                    available[i] -= req.REQ[i];
                    alloc[req.user_id][i] += req.REQ[i];
                    need[req.user_id][i] -= req.REQ[i];
                    req.REQ[i] = 0;
                }
            }
            Q.push_back(req);
            safe_print("Master thread stores resource request of thread " + to_string(req.user_id));

            string print_waiting = "\t\tWaiting threads:";
            for(auto it = Q.begin(); it != Q.end(); it++) {
                print_waiting += (" " + to_string(it->user_id));
            }
            safe_print(print_waiting);
        }

        safe_print("Master thread tries to grant pending requests");
        
        for(auto it = Q.begin(); it != Q.end();) {
            if(vector_comp(it->REQ, available, "<=")) {

            #ifdef _DLAVOID
                vector<int> temp_av = available;
                vector<vector<int>> temp_ne = need;
                vector<vector<int>> temp_al = alloc;

                vector_op(temp_av, it->REQ, '-');
                vector_op(temp_al[it->user_id], it->REQ, '+');
                vector_op(temp_ne[it->user_id], it->REQ, '-');

                if(!isSafe(temp_av, temp_al, temp_ne)) {
                    safe_print("\t+++ Unsafe to grant request of thread " + to_string(it->user_id));
                    ++it;
                    continue;
                }
            #endif

                vector_op(available, it->REQ, '-');
                vector_op(alloc[it->user_id], it->REQ, '+');
                vector_op(need[it->user_id], it->REQ, '-');

                up(&user_sem[it->user_id]);

                safe_print("Master thread grants resource request for thread " + to_string(it->user_id));
                it = Q.erase(it);
            }
            else {
                safe_print("\t+++ Insufficient resources to grant request of thread " + to_string(it->user_id));
                ++it;
            }
        }

        string print_waiting = "\t\tWaiting threads:";
        for(auto it = Q.begin(); it != Q.end(); it++) {
            print_waiting += (" " + to_string(it->user_id));
        }
        safe_print(print_waiting);
    }

    for(int i=0; i<n; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&BOS);
    pthread_barrier_destroy(&REQB);
    pthread_mutex_destroy(&pmtx);
    pthread_mutex_destroy(&rmtx);

    for(int i=0; i<n; i++) {
        pthread_barrier_destroy(&ACKB[i]);
        pthread_mutex_destroy(&user_sem[i].mtx);
        pthread_cond_destroy(&user_sem[i].cv);
    }

    return NULL;
}

int main() {
    pthread_t master_thread;
    pthread_create(&master_thread, NULL, master, NULL);
    pthread_join(master_thread, NULL);
}
