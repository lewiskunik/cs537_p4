#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>

#include <time.h>

typedef struct Node_t{
	struct Node_t *next;
	char *parent_link;
	char *data;
}Node;

typedef struct Parse_queue_t{
	Node *head;
	Node *tail;
	int parseItems;
}Queue;

pthread_mutex_t p_lock;
pthread_mutex_t d_lock;
pthread_cond_t dfull = PTHREAD_COND_INITIALIZER;
pthread_cond_t dempty = PTHREAD_COND_INITIALIZER;
pthread_cond_t pempty = PTHREAD_COND_INITIALIZER;

char** buff_download;
int dfillptr = 0;
int duseptr = 0;
int dq_size;
int downldItems = 0;

Queue *buff_parse;

void pthread_initialize(){
	pthread_mutex_init(&p_lock, NULL);
	pthread_mutex_init(&d_lock, NULL);

}


///////////////////////
//DOWNLOAD QUEUE FUNCTIONS
///////////////////////

//queue init
void d_init(){
	if((buff_download = malloc(sizeof(char*)*dq_size)) == NULL){
		fprintf(stderr, "cannot allocate download-buffer");
		exit(-1);
	}
}

//queue fill
void dfill(char* content){
	buff_download[dfillptr] = content;
	dfillptr = (dfillptr + 1) % dq_size;
	downldItems++;
}

//queue get
char* dget(){
	char*  tmp = buff_download[duseptr];
	duseptr = (duseptr + 1) % dq_size;
	downldItems--;
	return tmp;
}

//////////////////
//PARSE QUEUE FUNCTIONS
//////////////////

//queue init
void p_init(){
	if((buff_parse = malloc(sizeof(Queue))) == NULL){
		fprintf(stderr, "error allocating parse-buffer");
		exit(-1);
	}
	buff_parse->head = NULL;
	buff_parse->tail = NULL;
	buff_parse->parseItems = 0;
}

void padd(char *prev_link, char* content){
	//Make the node that we will insert
	Node *tmp;
	if((tmp = malloc(sizeof(Node))) == NULL){
		fprintf(stderr, "error allocating parse node");
		exit(-1);
	}
	//fill node with data and stuff!
	tmp->next = NULL;
	tmp->data = content;
	tmp->parent_link = prev_link;
	
	//if queue is currently empty
	if(buff_parse->head == NULL){
		buff_parse->head = tmp;
		buff_parse->tail = tmp;
	}
	else{
		(buff_parse->tail)->next = tmp;
		buff_parse->tail = tmp;
	}
	buff_parse->parseItems++;
}

Node *pget(){
	Node *tmp = buff_parse->head;
	buff_parse->head = tmp->next;
	buff_parse->parseItems--;
	
	return tmp;
}


//////////////////
// parsing function
//////////////////
void parse(char* prev_link, char* page){

	char *tmp, *link;
	char **saveptr = NULL;
	tmp = strtok_r(page, "link:", saveptr);
	
	while(tmp != NULL){
		link = strtok_r(NULL, " ", saveptr);
		pthread_mutex_lock(&d_lock);
		//TODO: wait on condition variable if download queue is full
		while(downldItems == dq_size)
			pthread_cond_wait(&dfull, &d_lock);
		dfill(link);
		pthread_cond_signal(&dempty);
		pthread_mutex_unlock(&d_lock);
		//TODO: send link via edge. use link and prev_link
		tmp = strtok_r(NULL, "link:", saveptr);
	}
}


//producer sequence
void *parse_thread(){
	Node *page_content;
	while(1){
		pthread_mutex_lock(&p_lock);
		//if parse queue is full, wait until consumer opens it up
		while(buff_parse->parseItems == 0)
			pthread_cond_wait(&pempty, &p_lock);
		page_content = pget();
		
		pthread_mutex_unlock(&p_lock);
	
		parse(page_content->parent_link, page_content->data); //will implement the pthread_wait within
		free(page_content);
	}
	return NULL;
}

//consumer sequence
void *download_thread(){
	char *link;
	char *content = NULL; //TODO:actually assign this later
	while(1){
		//is download queue empty? If yes, pull from parse queue and get a link!
		pthread_mutex_lock(&d_lock);
		//if download queue is empty, wait until parser gives it something
		while(downldItems == 0)
			pthread_cond_wait(&dempty, &d_lock);
		link = dget();
		//TODO: give the link to the fetch function. ***content variable assigned here***
		pthread_cond_signal(&dfull);

		
		//give it that content!
		padd(link, content);
		pthread_cond_signal(&pempty);
		pthread_mutex_unlock(&d_lock); //TODO: ask about this.
		
	}
	return NULL;
}


/////////////////
// Downloading function
/////////////////
void download(char * (*_fetch_fn)(char *url)){
	
	

}


///Main function! Kind of.
int crawl(char *start_url,
	  int download_workers,
	  int parse_workers,
	  int queue_size,
	  char * (*_fetch_fn)(char *url),
	  void (*_edge_fn)(char *from, char *to)) {

	printf("begin\n");
  //start code here:
	pthread_initialize();
  	//assign queue_size to global var
  	dq_size = queue_size;
	
	/*
	pthread_t pid[download_workers], cid[parse_workers];
	int i;
	
	for(i = 0; i < download_workers; i++)
		pthread_create(&pid[i], NULL, pproducer, NULL);

	for(i = 0; i < parse_workers; i++)
		pthread_create(&cid[i], NULL, pconsumer, NULL);
	
	
		
	*/
	
  	p_init();
	padd("test","test\n");
	char *to_print;
	Node *tmp_node = pget();
	to_print = tmp_node->data;
	printf("%s", to_print);


  	return 0;
}



