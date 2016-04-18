/* Similar to previous test but now there are multiple downloader threads, so should be able to parse all links and put it in buffer as the buffer gets consumed by multiple threads*/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "crawler.h"
#include <pthread.h>
#include <semaphore.h>
#include "helper.h"


pthread_mutex_t buffer_mutex, bounded_queue_mutex;
int buffer_consumed = 0;
int fill = 0, pages_downloaded = 0;
char buffer[100][50];
char *fetch(char *link) {
  pthread_mutex_lock(&bounded_queue_mutex);
  if(pages_downloaded == 1)
  {
  	pthread_mutex_unlock(&bounded_queue_mutex);
 	sleep(5);
 }
  else
  	pthread_mutex_unlock(&bounded_queue_mutex);
  int fd = open(link, O_RDONLY);
  if (fd < 0) {
        fprintf(stderr, "failed to open file: %s", link);
    	return NULL;
  }
  int size = lseek(fd, 0, SEEK_END);
  assert(size >= 0);
  char *buf = Malloc(size+1);
  buf[size] = '\0';
  assert(buf);
  lseek(fd, 0, SEEK_SET);
  char *pos = buf;
  while(pos < buf+size) {
    int rv = read(fd, pos, buf+size-pos);
    assert(rv > 0);
    pos += rv;
  }
  pthread_mutex_lock(&bounded_queue_mutex);
  pages_downloaded++;
  pthread_mutex_unlock(&bounded_queue_mutex);
  close(fd);
  return buf;
}

void edge(char *from, char *to) {
	if(!from || !to)
		return;
	char temp[50];
	temp[0] = '\0';
	char *fromPage = parseURL(from);
	char *toPage = parseURL(to);
	strcpy(temp, fromPage);
	strcat(temp, "->");
	strcat(temp, toPage);
	strcat(temp, "\n");
	pthread_mutex_lock(&buffer_mutex);
	strcpy(buffer[fill++], temp);	
	pthread_mutex_unlock(&buffer_mutex);	
  	pthread_mutex_lock(&bounded_queue_mutex);
  	if(pages_downloaded == 1 && fill > 4)
		buffer_consumed = 1;		
	pthread_mutex_unlock(&bounded_queue_mutex);
     
}

int main(int argc, char *argv[]) {
  pthread_mutex_init(&buffer_mutex, NULL);
  pthread_mutex_init(&bounded_queue_mutex, NULL);
  int rc = crawl("/u/c/s/cs537-1/ta/tests/4a/tests/files/small_buffer/pagea", 5, 1, 1, fetch, edge);
  assert(rc == 0);
  check(buffer_consumed == 1, "All the links should have been parsed as multiple downloader threads should have consumed the buffer\n");
  return compareOutput(buffer, fill, "/u/c/s/cs537-1/ta/tests/4a/tests/files/output/small_buffer.out");
}
