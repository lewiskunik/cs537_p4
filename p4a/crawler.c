#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>

#include <time.h>

#define HASH_TABLE_SIZE 193

//Global definitions for the hash table
typedef struct HashNode_t{
	struct HashNode_t *next;
	char *link;
	//int done;
}HashNode;

//193 is a prime number! That's the size of our array.
HashNode *HashTable[HASH_TABLE_SIZE];

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
//pthread_cond_t done = PTHREAD_COND_INITIALIZER;

char** buff_download;
int dfillptr = 0;
int duseptr = 0;
int dq_size;
int downldItems = 0;

int num_awake = 0;

Queue *buff_parse;

//global fetch function
char * (*_fetch_function)(char *);
//global edge function
void (*_edge_function)(char *, char *);


void pthread_initialize(){
	pthread_mutex_init(&p_lock, NULL);
	pthread_mutex_init(&d_lock, NULL);

}

////////////////////////
// Hash Table Function
////////////////////////

//fletcher32 function taken from  https://en.wikipedia.org/wiki/Fletcher%27s_checksum
uint32_t fletcher32( uint16_t const *data, size_t words )
{
        uint32_t sum1 = 0xffff, sum2 = 0xffff;
        size_t tlen;
 
        while (words) {
                tlen = words >= 359 ? 359 : words;
                words -= tlen;
                do {
                        sum2 += sum1 += *data++;
                } while (--tlen);
                sum1 = (sum1 & 0xffff) + (sum1 >> 16);
                sum2 = (sum2 & 0xffff) + (sum2 >> 16);
        }
        /* Second reduction step to reduce sums to 16 bits */
        sum1 = (sum1 & 0xffff) + (sum1 >> 16);
        sum2 = (sum2 & 0xffff) + (sum2 >> 16);
        return sum2 << 16 | sum1;
}

//TODO: may need to initialize hash table so each node is null
/*void hash_init(){
	HashTable = (*HashNode[])malloc(HASH_TABLE_SIZE * sizeof(HashNode*));
}*/


//returns 1 if link already exists in hash table, 0 if link does not yet exist
int check_hash(char *link){

	
	int index = fletcher32((uint16_t const *)link, strlen(link)) % HASH_TABLE_SIZE;
	printf("Link is: %s and Hash index is: %d\n", link, index);
	int first_time = 1;
	HashNode *check_node, *prev_node;
	prev_node = NULL;
	check_node = HashTable[index];
	while(check_node != NULL){
		first_time = 0;		
		if((strcmp(link, check_node->link)) == 0){
			printf("found used link\n");
			return 1;
		}
		else{
			prev_node = check_node;
			check_node = check_node->next;
		}	
	}
	HashNode *new_node = malloc(sizeof(HashNode *));
	new_node->next = NULL;
	//new_node->done = 0;
	new_node->link = link;
	if(first_time)
		HashTable[index] = new_node;
	else
		prev_node->next = new_node;
	
	return 0;
}

//sets the hash node of the link in the table to the done state.
/*
void set_hash_done(char *link){

	int index = fletcher32((uint16_t const *)link, strlen(link)) % HASH_TABLE_SIZE;
	HashNode *check_node;
	check_node = HashTable[index];
	//should return the node because it has already been downloaded
	while(check_node != NULL){		
		if((strcmp(link, check_node->link)) == 0){
			printf("found used link\n");
			continue;
		}
		else
			check_node = check_node->next;
	}
	if(check_node != NULL)
		check_node->done = 1;
	//else: do nothing. return from function. should never actually leave without setting done to 1
}
*/


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
	char *tmp, *link, *copy;
	char *saveptr = NULL;
	copy = strdup(page);
	tmp = strtok_r(page, ":", &saveptr);//problem child
	printf("file is: %s\n", prev_link);
	if(strcmp(tmp, copy) == 0)
		return;
	while(tmp != NULL){
		
		//link = strdup(strtok_r(NULL, " \n", &saveptr));
		link = strtok_r(NULL, " \n", &saveptr);
		printf("link found: %s\n", link);
		pthread_mutex_lock(&d_lock);
		//wait on condition variable if download queue is full
		if(downldItems == dq_size)
			num_awake--;
		while(downldItems == dq_size)
			pthread_cond_wait(&dfull, &d_lock);
		num_awake++;
		if(!check_hash(link)){
			dfill(link);//means that the link has not been downloaded before
			pthread_cond_signal(&dempty);
		}

		pthread_mutex_unlock(&d_lock);
		//send link via edge. use link and prev_link
		_edge_function(prev_link, link);

		copy = strdup(tmp);
		tmp = strtok_r(NULL, ":", &saveptr);

		if(tmp != NULL){//
		/// this compares strings to see if we've reached the end of the file
		if(strcmp(tmp, copy) == 0){
			tmp = NULL;
		}
		}//
	}
	
}



//producer sequence
void *parse_thread(){
	
	Node *page_content;
	while(1){
		//printf("parser\n");
		pthread_mutex_lock(&p_lock);
		if(buff_parse->parseItems == 0)
			num_awake--;
		//if parse queue is empty, wait until dowloader gives something
		while(buff_parse->parseItems == 0)
			pthread_cond_wait(&pempty, &p_lock);
		num_awake++;
		page_content = pget();
		pthread_mutex_unlock(&p_lock);
		parse(page_content->parent_link, page_content->data); //will implement the pthread_wait within
		free(page_content);
	}
	return NULL;
}

//consumer sequence
void *download_thread(){
	
	while(1){
		//printf("downloader\n");
		//is download queue empty? If yes, pull from parse queue and get a link!
		pthread_mutex_lock(&d_lock);
		//if download queue is empty, wait until parser gives it something
		//printf("%d\n", downldItems);
		if(downldItems == 0)
			num_awake--;
		while(downldItems == 0)
			pthread_cond_wait(&dempty, &d_lock);
		num_awake++;
		//pull link off of download queue
		char *link = dget();
		pthread_cond_signal(&dfull);
		pthread_mutex_unlock(&d_lock);
		

		char *page = _fetch_function(link);
		//add content to parse queue
		padd(link, page);
		//set_hash_done(link);
		pthread_cond_signal(&pempty);

	}
	return NULL;
}




///Main function! Kind of.
int crawl(char *start_url,
	  int download_workers,
	  int parse_workers,
	  int queue_size,
	  char * (*_fetch_fn)(char *url),
	  void (*_edge_fn)(char *from, char *to)) {

  	//start code here:
	pthread_initialize();
  	//assign queue_size to global var
  	dq_size = queue_size;
	
	_fetch_function = _fetch_fn;
	_edge_function = _edge_fn;

	//hash_init();
	p_init();
	d_init();
//printf("start url = %s\n", start_url);
	//char *tmp = strtok(start_url," \n"); 


	dfill(start_url);
	check_hash(start_url);
	pthread_t pid[download_workers], cid[parse_workers];
	int i;
	
	for(i = 0; i < download_workers; i++){
		num_awake++;
		pthread_create(&pid[i], NULL, download_thread, NULL);
	}
	for(i = 0; i < parse_workers; i++){
		num_awake++;
		pthread_create(&cid[i], NULL, parse_thread, NULL);
	}
		
	while(num_awake > 0)//TODO
  		sleep(1);


  	return 0;
}



