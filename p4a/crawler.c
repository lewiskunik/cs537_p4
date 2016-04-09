#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>

#include <time.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pfill = PTHREAD_COND_INITIALIZER;
pthread_cond_t pempty = PTHREAD_COND_INITIALIZER;
char* buff_parse;
int pfillptr = 0;
int puseptr = 0;
int pq_size;
int parseItems = 0;

char* buff_download;
int dfillptr = 0;
int duseptr = 0;
int dq_size = 16;
int downldItems = 0;

///////////////////////
//PARSE QUEUE FUNCTIONS
///////////////////////

//queue init
void p_init(){
	if((buff_parse = malloc(sizeof(char*)*q_size)) == NULL){
		fprintf(stderr, "cannot allocate parse-buffer");
		exit(-1);
	}
}

//queue fill
void pfill(char* link){
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

	int i;
	pthread_mutex_lock(&lock);
	//if parse queue is full, wait until consumer opens it up
	while(parseItems == pq_size)
		pthread_cond_wait(&pempty, &lock);
	pfill(link);
	pthread_cond_signal(&pfill);
	pthread_mutex_unlock(&lock);
}

//consumer sequence
char *pconsumer(){
	char *link;
	pthread_mutex_lock(&lock);
	//if parse queue is empy, wait until producer gives it something
	while(parseItems == 0)
		pthread_cond_wait(&pfill, &lock);
	link = pget();
	pthread_cond_signal(&pempty);
	pthread_mutex_unlock(&lock);
	return link;
}

//////////////////
//DONWLOAD QUEUE FUNCTIONS
//////////////////

//queue init
void d_init(){
	if((buff_download = malloc(sizeof(char*)*dq_size)) == NULL){
		fprintf(stderr, "error allocating download-buffer");
		exit(-1);
	}
}

void dfill(char* link){
	buff_download[dfillptr] = link;
	//check if queue is full
	if(downldItems == dq_size){
		//expand queue
	}
	dfillptr = dfillptr + 1;
	downldItems++;
}

char *dget(){
	char *tmp = buff_download[duseptr];
	puseptr++;
	downldItems--;
	return tmp;
}


//////////////////
// parsing function
//////////////////
char *parse(char* page){

	


}

///Main function! Kind of.
int crawl(char *start_url,
	  int download_workers,
	  int parse_workers,
	  int queue_size,
	  char * (*_fetch_fn)(char *url),
	  void (*_edge_fn)(char *from, char *to)) {

  //start code here:
	
  	//assign queue_size to global var
  	q_size = queue_size;
	
	pthread_t pid[download_workers], cid[parse_workers];
	int i;
	for(i = 0; i < download_workers; i++)
		pthread_create(&pid[i], NULL, pproducer, NULL);

	for(i = 0; i < parse_workers; i++)
		pthread_create(&cid[i], NULL, pconsumer, NULL);
	
	
		
	
  


  	return -1;
}



