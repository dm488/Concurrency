#include "3140_concur.h"
#include <stdlib.h>	


struct process_state{
	unsigned int sp; //stack pointer (SP) for process
};//Note process_sate already typdef as process_t in 3140_concur.h

typedef struct queue{
	process_t *p; //pointer to a process_t structure
	struct queue *next;//pointer to next in queue
} queue_t;

//Initialie current_process and queue_head to NULL
process_t *current_process = NULL;  
queue_t *queue_head = NULL; 

/* Helper methods */

/*Adds a node to the end of the queue */
void add_queue(queue_t *queue_tail){
	
	//Add node to end of queue
	if (queue_head == NULL){//Set head node
		queue_head = queue_tail;
		queue_head->next = NULL; 
	}
	else{//Add node to end of existing queue
		queue_t *tmp = queue_head; 
		while(tmp->next!=NULL){
			tmp = tmp->next;
		}
		tmp->next = queue_tail;
		queue_tail->next = NULL;  	
	}    
	
}

//Removes the current headnode from queue and updates it
//Note: Implementation handles updating current_process for use in process_select
void remove_queue(){
	//There are more processes in the queue
	if(queue_head != NULL){
		queue_head = queue_head->next;
		current_process = queue_head->p;
}
	//There are no processes left in queue
	else current_process = NULL;
}

/* Remaining methods */

int process_create(void(*f)(void), int n){
	unsigned int sp_create;//Stack pointer (SP) for process
	
	/*Create queue_t and process_t (allocate memory)*/
	queue_t *elm = (queue_t *)malloc(sizeof(queue_t));
	elm->p = (process_t *)malloc(sizeof(process_t)); 
		
	//Initialize memory allocation for running the process
	_disable_interrupt();
	sp_create = process_init(f,n);
	_enable_interrupt(); 
	
	
	//If memory allocation failed 
	if (sp_create == 0)return -1; 
	else{ //Memory Allocation Success. Set process's SP and Add elm to queue
	elm->p->sp = sp_create;
	add_queue(elm); 
	return 0;
	} 
}	


void process_start(void){
	//Set up timer A interrupt
	TACCR0 = 0x00EE;//Period (Timer count value)
	TACTL = TASSEL_2 + ID_3 + MC_1;//SMCLK clock, Input Divider of 8, Up-mode (count)
	TACCTL0 = CCIE; // Enable Timer interrupts
	
	//Call process_begin()to start execution of processes in queue 
	_disable_interrupt();
	process_begin(); 
}

/* Selects process to run and returns its SP value */
unsigned int process_select (unsigned int cursp){
	queue_t *temp;

	if (cursp == 0){ 
		//No process was running
		if (current_process == NULL){ //first process
			if (queue_head != NULL){ //Queue is non-empty
			current_process = queue_head->p;
			return current_process->sp;
			}
			return 0;  //Queue is empty
		}
		//Current Process terminated
		if (queue_head->next != NULL){ //Select next process and update current process 
		current_process = queue_head->next->p;
		queue_head = queue_head->next;
		return current_process->sp; 
		}
		//No more processes to run 
		queue_head = NULL; 
		current_process = NULL; 
		return 0;
	}
	else if (queue_head->next == NULL){
		//Current Process is only process (and not terminated), update SP to cursp
		current_process->sp = cursp; 
		return queue_head->p->sp; 
	} 
	else {
		//Current Process is not terminated but there is a ready process in queue
		//Update and Must switch to give other process a chance
		current_process->sp = cursp;
		temp = queue_head;  
		remove_queue(); //Switch to next process (at head) 
		add_queue(temp); //Add prev process back to queue since it did not terminate yet
		return queue_head->p->sp; 	
	}
}

