#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define READY 0
#define FINISH 1
#define RUN 2
#define MAX_LEN 9
#define COMMAND_LEN 7
#define PHYSICAL_MEM 1
#define ALLOCATE 1
#define NON_ALLOCATE -1
#define TOTALMEM 2048
#define TOTALPAGES 512
#define PAGE_SIZE 4
#define FRAMES_NEED 4
#define HUNDRED 100
#define LINE_LEN 4
//process creator
typedef struct node node_t;
struct node {
    int arrive_time;
    char pid[MAX_LEN];
    int remain_time;
    int memory;
    int total_time;
    int state;
    int allocated;
    int page;
    int reference_time;
    int in_mem;
    int page_left;
    int frames_in;
    node_t* next;
};

//linked list structure --- from FOA Lecture
typedef struct{
    node_t* head;
    node_t* foot;
}list_t;

typedef struct{
    int allocate;
    char pid[MAX_LEN];
}mem_frame;
//empty list creator --- from FOA Lecture 
list_t* make_empty_list(){
    list_t* list = malloc(sizeof(list_t));
    assert(list != NULL);
    list->head = NULL;
    list->foot = NULL;
    return list;
}

// to determine the list is empty or not
int is_empty(list_t* list){
    if(list->head == NULL || list == NULL){
        return 1;
    }
    return 0;
}

//create a new node into the linked list
node_t* create_node(char* pid, int arrive_time, int remain_time, int memory){
    node_t* new = malloc(sizeof(node_t));
    new->arrive_time = arrive_time;
    new->remain_time = remain_time;
    new->memory = memory;
    new->allocated = NON_ALLOCATE;
    new->reference_time = 0;
    new->in_mem = NON_ALLOCATE;
    new->page = 0;
    new->frames_in = 0;
    strcpy(new->pid,pid);
    new->state = READY;
    new->next = NULL;
    new->page_left = 0;
    new->total_time = remain_time;
    return new;
}

//insert at list foot creator --- from FOA Lecture
list_t* insert_at_foot(list_t* list, node_t* new){
    //if there is no process in the list
    if(list->head == NULL){
        //head and foot both be the new process
        list->head = new;
        list->foot = new;
    //if there are at least one process in the list, current foot's next will be the new process
    }else{
        list->foot->next = new;
        list->foot = new;
    }
    return list;
}

//delete the node when translating the node from one queue to another queue
void delete_node(list_t* list, node_t* node){
    if(list->head == node){
        list->head = list->head->next;
        return;
    }else{
        node_t* curr = list->head;
        node_t* pre = list->head;
        while(curr != NULL){
            pre = curr;
            curr = curr->next;
            if(curr == node){
                break;
            }
        }
        pre->next = curr->next;
        if(curr == list->foot){
            list->foot = pre;
        }
    }
}

//take out the first process in the linked list
node_t* take_first(list_t* list){
    //if the list is empty
    if(is_empty(list) == 1){
        return NULL;
    }else{
        node_t* first = list->head;
        list->head = list->head->next;
        first->next = NULL;
        return first;
    }
}



// to read input by opening the file and put the process into input queue
void read_input(list_t* input_queue, char* filename){
    FILE* fp = fopen(filename,"r");
    //if the file is failed to open
    if(fp == NULL){
        printf("fail");
    }else{
        int arrive_time;
        int remain_time;
        char pid[MAX_LEN];
        int memory;
        //when the line of file contain arrive time, process id, remaining time and memory
        while(fscanf(fp,"%d %s %d %d\n",&arrive_time, pid, &remain_time, &memory) == LINE_LEN){
            //put these information into one node
            node_t* new_node = create_node(pid, arrive_time, remain_time, memory);
            //put this node into the queue
            input_queue = insert_at_foot(input_queue, new_node);
        }
    }
    fclose(fp);
}
// put the ready state process into ready queue
void get_arrive_list(list_t* input_queue, list_t* ready_queue, int time){
    node_t* now = input_queue->head;
    //loop the all process in input queue
    while(now != NULL){
        node_t* next = now->next;
        // if the process's is arrived
        if(now->arrive_time <= time){
            // delete the process into input queue and put the process into the end of the ready queue
            delete_node(input_queue, now);
            now->next = NULL;
            ready_queue = insert_at_foot(ready_queue, now);
        }
        now = next;
    }
}

//calculate the process need how many pages to store
int calculate_page_num(node_t* head){
        int page_num = 0;
        if((head->memory) % PAGE_SIZE == 0){
            //会需要几个page
            page_num = (head->memory) / PAGE_SIZE ;
        }else{
            page_num = (int)((head->memory) / PAGE_SIZE) + 1;
        }
        head->page = page_num;
    return page_num;
}

//calculate numbers of the process after one process is finished
int process_remaining(list_t* queue){
    int total = 0;
    node_t* now = queue->head;
    if(queue->head == NULL){
        return 0;
    }else{
        while(now != NULL){
            now = now->next;
            total+= 1;
        }
        return total;
    }
}

//calculate the percentage of the memory use
int get_mem_use(int total_memory,int left_memory){
    if(((total_memory-left_memory)*100)%total_memory ==0){
        return ((total_memory-left_memory)*100/total_memory);
    }else{
        return  ((total_memory-left_memory)*100/total_memory)+1;
    }
}


double** addRow(double **arr, int *rows, int cols) {
    (*rows)++; 

    arr = (double **)realloc(arr, (*rows) * sizeof(double *));
    if (arr == NULL) {
        printf("Fail\n");
        exit(1);
    }
    arr[*rows - 1] = (double *) malloc(cols * sizeof(double));
    if (arr[*rows - 1] == NULL) {
        printf("Fail\n");
        exit(1);
    }

    return arr;
}


void evicted_memory(int *frame_left, node_t* head, mem_frame* frame, int quantum, list_t* ready_queue,int time, int *memory_left, int evict_tnum){
    int total_evicted = 0;
    int numofd = 0;
    int to_evicted[TOTALPAGES];
    //initalize the evict array to record the evict frames
    for(int i = 0; i < TOTALPAGES; i++){
        to_evicted[i] = NON_ALLOCATE;
    }
    printf("%d,EVICTED,evicted-frames=[", time);
    while(total_evicted < evict_tnum){
        node_t *node = ready_queue->head;
        node_t *min_node;
        int min_use_time = -1;
        //if there is a process in ready state
        while (node != NULL && node != head) {
            //if the process is in memory
            if(node->in_mem == ALLOCATE){
                //if the process is first searched in the memory,then record
                if(min_use_time == -1){
                    min_use_time = node->reference_time;
                    min_node = node;
                }else{
                    //record the least referenced node
                    if(min_use_time > node->reference_time){
                        min_use_time = node->reference_time;
                        min_node = node;
                    }
                }
            }
            node = node->next;
        }
        total_evicted += min_node->page;
        //move the process to disk and free the allocated frame 
        for(int i = 0; i < TOTALPAGES; i++){
            if(strcmp(frame[i].pid, min_node->pid) == 0){
                frame[i].allocate = NON_ALLOCATE;
                to_evicted[i] = ALLOCATE;
                strcpy(frame[i].pid,"NULL");
                *frame_left += 1;
            } 
        }
        //allocate this node into disk
        node_t* loop_node = ready_queue->head;
        while (loop_node != NULL) {
            if(strcmp(min_node->pid, loop_node->pid) == 0){
                //put it in disk
                loop_node->in_mem = NON_ALLOCATE;
                //should calculate memory been left
                *memory_left += calculate_page_num(loop_node) * PAGE_SIZE;
            }
            loop_node = loop_node->next;
        }
    }
    // record the evicted page frame
    for(int i = 0; i < TOTALPAGES; i++){
        if(to_evicted[i] == ALLOCATE){
            numofd++;
            printf("%d", i);
            if(numofd < total_evicted){
                printf(",");
            }
        }    
    }
    printf("]\n");
}
//free the memory when the process is finished
void free_memory(node_t* head, mem_frame* frame, int time, int* left_memory, int* frame_left){
    printf("%d,EVICTED,evicted-frames=[", time);
    int count = 0;
    for(int i = 0; i < TOTALPAGES; i++){
        if(strcmp(head->pid, frame[i].pid) == 0){
            count++;
            printf("%d",i);
            strcpy(frame[i].pid, "NULL");
            frame[i].allocate = NON_ALLOCATE;
            if(count < head->page){
                printf(",");
            }else{
                break;
            }
        }
    }
    printf("]\n");
    //add the free memory and free frame
    *left_memory += head->page * PAGE_SIZE;
    *frame_left += head->page;
}


void free_frames(node_t* head, mem_frame* frame,int time, int* left_memory){
    printf("%d,EVICTED,evicted-frames=[", time);
    int count = 0;
    for(int i = 0; i < TOTALPAGES; i++){
        if(strcmp(head->pid, frame[i].pid) == 0){
            count++;
            printf("%d",i);
            strcpy(frame[i].pid, "NULL");
            frame[i].allocate = NON_ALLOCATE;
            if(count < head->frames_in){
                printf(",");
            }else{
                break;
            }
        }
    }
    printf("]\n");
    *left_memory += count * PAGE_SIZE;
}


void evicted_frames(int frame_left, node_t* head, mem_frame* frame, int quantum, list_t* ready_queue,int time, int *memory_left,int frames_need_to_evict){
    int tevicted = 0;
    int numofd = 0;
    int to_evicted[TOTALPAGES];
    for(int i =0;i<TOTALPAGES;i++){
        to_evicted[i]=0;
    }
    printf("%d,EVICTED,evicted-frames=[", time);
    while(tevicted<frames_need_to_evict){
        int count = 0;
        char min_pid[MAX_LEN];
        node_t *node = ready_queue->head;
        int min_use_time = -1;
        //int page_num = 0;
        while (node != NULL) {
            if(node!= head){
                //if the process is in memory
                if(node->frames_in >= 1){
                    //if the process is first searched in the memory,then record
                    if(min_use_time == -1){
                        min_use_time = node->reference_time;
                        //page_num = node->page;
                        strcpy(min_pid, node->pid);
                    }else{
                        if(min_use_time > node->reference_time){
                            min_use_time = node->reference_time;
                            //page_num = node->page;
                            strcpy(min_pid, node->pid);
                        }
                    }
                }
            }
            node = node->next;
        }
        for(int i = 0; i < TOTALPAGES; i++){
            if(strcmp(frame[i].pid, min_pid) == 0 && tevicted < frames_need_to_evict){
                count++;
                tevicted++;
                frame[i].allocate = NON_ALLOCATE;
                //printf("%d",i);
                to_evicted[i]=1;
                strcpy(frame[i].pid,head->pid);
            }
            if(strcmp(frame[i].pid, "NULL") == 0){
                frame[i].allocate = ALLOCATE;
                strcpy(frame[i].pid,head->pid);
            }
        }
        node_t *now = ready_queue->head;
        while(now != NULL){
            if(strcmp(now->pid,min_pid) == 0){
                now->frames_in-=count;
                break;
            }else{
                now = now->next;
            }
        }    
    }
    for(int i = 0; i < TOTALPAGES; i++){
        if(to_evicted[i] == 1){
            numofd++;
            printf("%d", i);
            if(numofd < frames_need_to_evict){
                printf(",");
            }
        }    
    }
    printf("]\n");
}

void free_malloc(list_t* queue){
    node_t* free_node= queue->head;
    while(free_node != NULL){
        node_t* f_node = free_node->next;
        free(free_node);
        free_node = f_node;
    }
    free(queue);
}



int main(int argc, char** argv) {
    char* filename;
    char* memory_strategy;
    int quantum = 0;
    int time = 0;
    int rows = 0;
    int cols = 2;
    double **arr = NULL;
    double maxoverhead = 0;
    double totaloverhead = 0;

    if(argc < COMMAND_LEN){
        printf("wrong");
        return 0;
    }
    for(int i = 0; i < COMMAND_LEN; i++){
        // if there is store the filename
        if(strcmp(argv[i], "-f") == 0){
            filename = argv[i + 1];
        }
        // if there is store the memory_strategy
        if(strcmp(argv[i],"-m") == 0){
            memory_strategy = argv[i + 1];
        }
        // if there is store the quantum
        if(strcmp(argv[i],"-q") == 0){
            //translate the string type to int type
            quantum = atoi(argv[i + 1]);
        }
    }
    list_t* input_queue = make_empty_list();
    list_t* ready_queue = make_empty_list();
    list_t* finished_queue = make_empty_list();
    read_input(input_queue, filename);
    char* pre_pid = NULL;
    int turntime = 0;
    int total_process = 0;


    if(strcmp(memory_strategy,"infinite")==0){
        // if input queue or ready queue is not empty, still processing
        while(!is_empty(input_queue)|| !is_empty(ready_queue)){
            get_arrive_list(input_queue,ready_queue,time);
            node_t* head = take_first(ready_queue);

            //if the ready queue doesn't have any process,but the input queue still have processes
            if(head == NULL){
                //time will increase and no operation here
                time += quantum;
                continue;
            }
            //if the  process remaining time is less than or equal to quantum, then the running and finish message will be printed 
            if(head->remain_time <= quantum){

                get_arrive_list(input_queue, ready_queue, time + quantum);
                // if the process is changed status, print the message
                if(pre_pid != head->pid){
                    printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", time, head->pid, head->remain_time);
                }
                time += quantum;
                printf("%d,FINISHED,process-name=%s,proc-remaining=%d\n", time, head->pid, process_remaining(ready_queue));
                turntime += time-head->arrive_time;
                arr = addRow(arr, &rows, cols);
                arr[rows-1][0] = time-head->arrive_time;
                arr[rows-1][1] = head->total_time;
                total_process +=1;
                //set in to finish queue
                pre_pid = head->pid;
                head->remain_time = 0;
                head->state = FINISH;
                insert_at_foot(finished_queue,head);
                //if the process remaining time is larger than quantum
            }else{
                if(pre_pid != head->pid){
                    printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", time, head->pid, head->remain_time);
                }
                head->remain_time -= quantum;
                pre_pid = head->pid;
                time += quantum;
                get_arrive_list(input_queue,ready_queue, time);
                //insert the process at the foot of the ready queue
                insert_at_foot(ready_queue, head);
            }
        }
    }


    //task2
    int memory_array[2048];
    for(int i = 0;i < 2048; i++){
        memory_array[i] = 0;
    }
    int total_memory = 2048;
    int left_memory = 2048;

    if(strcmp(memory_strategy,"first-fit") == 0){
        while(!is_empty(input_queue)|| !is_empty(ready_queue)){
            get_arrive_list(input_queue,ready_queue,time);
            node_t* head = take_first(ready_queue);

            if(head == NULL){
                time+=quantum;
                continue;
            }

            //when a process try to find a place
            if(head->state == READY){
                int longest_continue = 0;
                for(int i = 0;i < total_memory; i++){
                    //find a empty memory and find out how long of the continue empty memory
                    if(memory_array[i] == 0){
                        for(int j = i; j < total_memory; j++){
                            if(memory_array[j] == 0){
                                longest_continue += 1;
                            }
                        }
                        //when find a place to allocate
                        if(longest_continue >= head->memory){
                            head->allocated = i;
                            for(int k = i; k < head->memory + i; k++){
                                memory_array[k] = 1;
                            }
                            break;
                        }
                    }
                }
            }

            //when a process do not have a place in memory
            if(head->allocated == NON_ALLOCATE){
                insert_at_foot(ready_queue, head);
                pre_pid = head->pid;
                continue;
            }

            //set the process already have allocate in memory and can run
            if(head->state == READY){
                left_memory -= head->memory;
                head->state = RUN;
            }

            //
            if(pre_pid != head->pid ){
                printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,allocated-at=%d\n", time, head->pid, head->remain_time, get_mem_use(total_memory,left_memory), head->allocated);
            }

            if(head->remain_time <= quantum){            
                get_arrive_list(input_queue, ready_queue, time + quantum);
                time += quantum;
                printf("%d,FINISHED,process-name=%s,proc-remaining=%d\n", time, head->pid, process_remaining(ready_queue));
                turntime += time-head->arrive_time;
                arr = addRow(arr, &rows, cols);
                arr[rows-1][0] = time-head->arrive_time;
                arr[rows-1][1] = head->total_time;
                total_process += 1;
                pre_pid = head->pid;
                head->remain_time = 0;
                head->state = FINISH;
                //deallocate memory
                for(int i = head->allocated; i < head->allocated + head->memory; i++){
                    memory_array[i] = 0;
                }
                left_memory += head->memory;
                insert_at_foot(finished_queue, head);
            }else{
                head->remain_time -= quantum;
                pre_pid = head->pid;
                time += quantum;
                get_arrive_list(input_queue, ready_queue, time);
                insert_at_foot(ready_queue, head);
            }
        }
    }

    //task3
    if(strcmp(memory_strategy,"paged") == 0){
        mem_frame frame[TOTALPAGES];
        int frame_left = TOTALPAGES;
        int total_mem = TOTALMEM;
        int memory_left = TOTALMEM;
        //initialize frame
        for(int i = 0; i < TOTALPAGES; i++){
            frame[i].allocate = NON_ALLOCATE;
            strcpy(frame[i].pid, "NULL");
        }
        //if there are processes in ready queue or input queue
        while(is_empty(input_queue) == 0 || is_empty(ready_queue) == 0){
            get_arrive_list(input_queue, ready_queue, time);
            node_t* head = take_first(ready_queue);
              //if the ready queue doesn't have any process,but the input queue still have processes
            if(head == NULL){
                //time will increase and no operation here
                time += quantum;
                continue;
            }
            //if this process is in memory, run
            if(head->in_mem == ALLOCATE){
                head->reference_time = time;
            }else{
                //if the process is not in memory
                head->reference_time = time;
                calculate_page_num(head);
                //allocate the memory for process
                if(head->page > frame_left){
                    evicted_memory(&frame_left, head, frame, quantum, ready_queue, time, &memory_left, (head->page - frame_left));
                }

                int count = 0;
                //put this current process in the frame
                for(int i =0;i < TOTALPAGES; i++){
                    if(frame[i].allocate == NON_ALLOCATE && count < head->page){
                        count++;
                        frame[i].allocate = ALLOCATE;
                        strcpy(frame[i].pid, head->pid);
                        frame_left -= 1;
                    }
                }    
                //assign the process in memory
                head->in_mem = ALLOCATE;
                //把process的 memory 加入总memory中
                memory_left -= head->page * PAGE_SIZE;
            }
            if(pre_pid != head->pid){
                printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,mem-frames=[",time,head->pid, head->remain_time, get_mem_use(total_mem,memory_left));
                int count = 0;
                for(int i = 0;i < TOTALPAGES; i++){
                    if(strcmp(head->pid, frame[i].pid) == 0){
                        count++;
                        printf("%d",i);
                        if(count < head->page){
                            printf(",");
                        }else{
                            break;
                        }
                    }
                }
                printf("]\n");
            }
            //if the process will finish at this round
            if(head->remain_time <= quantum){
                get_arrive_list(input_queue, ready_queue, time + quantum);
                time += quantum;
                // more memory and frames can be used
                free_memory(head, frame, time, &memory_left, &frame_left);
                printf("%d,FINISHED,process-name=%s,proc-remaining=%d\n", time, head->pid, process_remaining(ready_queue));
                turntime += time-head->arrive_time;
                arr = addRow(arr, &rows, cols);
                arr[rows-1][0] = time-head->arrive_time;
                arr[rows-1][1] = head->total_time;
                total_process +=1;
                pre_pid = head->pid;
                head->remain_time = 0;
                head->state = FINISH;
                head->in_mem = NON_ALLOCATE;
                insert_at_foot(finished_queue,head);
            }else{
                //the process will stil in ready queue
                head->remain_time -= quantum;
                pre_pid = head->pid;
                time += quantum;
                get_arrive_list(input_queue,ready_queue, time);
                //insert the process at the foot of the ready queue
                insert_at_foot(ready_queue, head);
            }
        }
    }







    //task4
    if(strcmp(memory_strategy,"virtual") == 0){
        mem_frame frame[TOTALPAGES];
        int frame_left = TOTALPAGES;
        int total_mem = TOTALMEM;
        int memory_left = TOTALMEM;
        //initialize frame;
        for(int i = 0; i < TOTALPAGES; i++){
            frame[i].allocate = NON_ALLOCATE;
            strcpy(frame[i].pid, "NULL");
        }
        //when not finish all
        while(is_empty(input_queue) == 0 || is_empty(ready_queue) == 0){
            //get the coming process
            get_arrive_list(input_queue, ready_queue, time);
            node_t* head = take_first(ready_queue);
              //if the ready queue doesn't have any process,but the input queue still have processes
            if(head == NULL){
                //time will increase and no operation here
                time += quantum;
                continue;
            }

            //frames in is enough to run
            if(head->frames_in >= FRAMES_NEED || head->frames_in == calculate_page_num(head)){
                head->reference_time = time;
            }else{
                //try to allocate
                head->reference_time = time;
                int page_not_in = calculate_page_num(head)-head->frames_in;

                //to find how many frames at least of the process need to run
                int frames_need_to_run = FRAMES_NEED;
                if(calculate_page_num(head)<FRAMES_NEED){
                    frames_need_to_run = calculate_page_num(head);
                }
                //the frames left is not enough for all the process pages in, but enough for the process need to run
                if(frame_left <= page_not_in && frame_left>=(frames_need_to_run-head->frames_in)){
                    //fill all the left frame of this process
                    for(int i=0;i<TOTALPAGES;i++){
                        if(frame[i].allocate != ALLOCATE){
                            frame[i].allocate = ALLOCATE;
                            strcpy(frame[i].pid,head->pid);
                            head->frames_in += 1;
                        }
                    }
                    memory_left = 0;
                    frame_left = 0;
                    //there are more frames than the process need
                }else if(frame_left >= page_not_in){
                    //fill all pages in to frame
                    for(int i=0;i<TOTALPAGES;i++){
                        if(frame[i].allocate != ALLOCATE){
                            for(int j=i;j<page_not_in+i;j++){
                                frame[j].allocate = ALLOCATE;
                                strcpy(frame[j].pid, head->pid);
                                head->frames_in += 1;
                                //再记一下head放了几个frame进去
                            }
                            break;
                        }
                    }
                    memory_left -= page_not_in * PAGE_SIZE;
                    frame_left -= page_not_in;
                    //the left frames is not enough for the process to run
                }else if(frame_left<page_not_in && frame_left < (frames_need_to_run-head->frames_in)){
                    //calculate the frames need to evicted
                    int frame_need_to_evict = (frames_need_to_run-head->frames_in)-frame_left;
                    evicted_frames(frame_left, head, frame, quantum, ready_queue, time, &memory_left,frame_need_to_evict);
                    //calculate the pages in the frame
                    if(calculate_page_num(head)>=FRAMES_NEED){
                        head->frames_in=FRAMES_NEED;
                    }else{
                        head->frames_in = calculate_page_num(head);
                    }
                    memory_left = 0;
                    frame_left = 0;
                }
            }

            //print out the running message
            if(pre_pid != head->pid){
                printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,mem-frames=[",time,head->pid, head->remain_time, get_mem_use(total_mem,frame_left*PAGE_SIZE));
                int count = 0;
                //print the frames allocate of the process
                for(int i = 0 ; i < TOTALPAGES; i ++){
                    if(strcmp(frame[i].pid,head->pid)==0){
                        printf("%d",i);
                        count++;
                        if(count<head->frames_in){
                            printf(",");
                        }
                    }
                }
                printf("]\n");
            }

            //process not going to finish
            if(head->remain_time > quantum){
                head->remain_time -= quantum;
                pre_pid = head->pid;
                time += quantum;
                get_arrive_list(input_queue,ready_queue, time);
                //insert the process at the foot of the ready queue
                insert_at_foot(ready_queue, head);
            }else{
                get_arrive_list(input_queue, ready_queue, time + quantum);
                time += quantum;
                //reallocate the frames of the finished process
                free_frames(head, frame, time,&memory_left);
                printf("%d,FINISHED,process-name=%s,proc-remaining=%d\n", time, head->pid, process_remaining(ready_queue));
                turntime += time-head->arrive_time;
                arr = addRow(arr, &rows, cols);
                arr[rows-1][0] = time-head->arrive_time;
                arr[rows-1][1] = head->total_time;
                total_process +=1;
                pre_pid = head->pid;
                head->remain_time = 0;
                head->state = FINISH;
                frame_left += head->frames_in;
                insert_at_foot(finished_queue, head);
            }
        }
    }

    //print out the average turnaround time
    printf("Turnaround time %d\n",(int)(ceil((double)turntime/total_process)));
    for(int i=0;i<rows;i++){
        totaloverhead+=(double)(arr[i][0]/arr[i][1]);
        if((double)(arr[i][0]/arr[i][1])>=maxoverhead){
        maxoverhead = (double)(arr[i][0]/arr[i][1]);
        }
    }
    //print max time overhead and average overhead
    printf("Time overhead %.2f %.2f\n", maxoverhead, round((totaloverhead / rows) * HUNDRED) / HUNDRED);
    //print the makespan time
    printf("Makespan %d\n", time);

    for(int i = 0; i < total_process; i++){
        free(arr[i]);
    }
    free(arr);
    free_malloc(finished_queue);
    free_malloc(input_queue);
    free_malloc(ready_queue);
}
