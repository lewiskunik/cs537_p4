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
	char *data;
}Node;

typedef struct Download_queue_t{
	Node *head;
	Node *tail;
	int downldItems;
}Queue;

pthread_mutex_t p_lock;
pthread_mutex_t d_lock;
pthread_cond_t pfill = PTHREAD_COND_INITIALIZER;
pthread_cond_t pempty = PTHREAD_COND_INITIALIZER;
char** buff_parse;
int pfillptr = 0;
int puseptr = 0;
int pq_size;
int parseItems = 0;

Queue *buff_download;

void pthread_initialize(){
	pthread_mutex_init(&p_lock, NULL);
	pthread_mutex_init(&d_lock, NULL);
}


///////////////////////
//PARSE QUEUE FUNCTIONS
///////////////////////

//queue init
void p_init(){
	if((buff_parse = malloc(sizeof(char*)*pq_size)) == NULL){
		fprintf(stderr, "cannot allocate parse-buffer");
		exit(-1);
	}
}

//queue fill
void padd(char* link){
	buff_parse[pfillptr] = link;
	pfillptr = (pfillptr + 1) % pq_size;
	parseItems++;
}

//queue get
char* pget(){
	char*  tmp = buff_parse[puseptr];
	puseptr = (puseptr + 1) % pq_size;
	parseItems--;
	return tmp;
}

//producer sequence
void *pproducer(char *link){

	pthread_mutex_lock(&p_lock);
	//if parse queue is full, wait until consumer opens it up
	while(parseItems == pq_size)
		pthread_cond_wait(&pempty, &p_lock);
	padd(link);
	pthread_cond_signal(&pfill);
	pthread_mutex_unlock(&p_lock);
	return NULL;
}

//consumer sequence
char *pconsumer(){
	char *link;
	pthread_mutex_lock(&p_lock);
	//if parse queue is empy, wait until producer gives it something
	while(parseItems == 0)
		pthread_cond_wait(&pfill, &p_lock);
	link = pget();
	pthread_cond_signal(&pempty);
	pthread_mutex_unlock(&p_lock);
	return link;
}

//////////////////
//DONWLOAD QUEUE FUNCTIONS
//////////////////

//queue init
void d_init(){
	if((buff_download = malloc(sizeof(Queue))) == NULL){
		fprintf(stderr, "error allocating download-buffer");
		exit(-1);
	}
	buff_download->head = NULL;
	buff_download->tail = NULL;
	buff_download->downldItems = 0;
}

void dfill(char* link){
	Node *tmp;
	if((tmp = malloc(sizeof(Node))) == NULL){
		fprintf(stderr, "error allocating download node");
		exit(-1);
	}
	pthread_mutex_lock(&d_lock);
	tmp->next = NULL;
	tmp->data = link;
	(buff_download->tail)->next = tmp;
	buff_download->tail = tmp;
	buff_download->downldItems++;
	pthread_mutex_unlock(&d_lock);//TODO
}

char *dget(){
	pthread_mutex_lock(&d_lock);
	Node *tmp = buff_download->head;
	buff_download->head = tmp->next;
	buff_download->downldItems--;
	pthread_mutex_unlock(&d_lock);//TODO
	char* link_data = tmp->data;
	free(tmp);
	return link_data;
}


//////////////////
// parsing function
//////////////////
char *parse(char* page){
	return page;
}

///Main function! Kind of.
int crawl(char *start_url,
	  int download_workers,
	  int parse_workers,
	  int queue_size,
	  char * (*_fetch_fn)(char *url),
	  void (*_edge_fn)(char *from, char *to)) {

	printf("begin");
  //start code here:
	pthread_initialize();
  	//assign queue_size to global var
  	pq_size = queue_size;
	
	/*
	pthread_t pid[download_workers], cid[parse_workers];
	int i;
	
	for(i = 0; i < download_workers; i++)
		pthread_create(&pid[i], NULL, pproducer, NULL);

	for(i = 0; i < parse_workers; i++)
		pthread_create(&cid[i], NULL, pconsumer, NULL);
	
	
		
	*/
  	d_init();
	dfill("test");
	printf(dget());


  	return 0;
}



