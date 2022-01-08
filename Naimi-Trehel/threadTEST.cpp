#include <stdio.h>
#include <pthread.h>
#define BUFFER_SIZE 2
#define OVER (-1)

// Compilation : 
// g++ -c processus.cpp && g++ calculCC.o processus.o -o processus -lpthread
// g++ -c threadTEST.cpp && g++ threadTEST.o -o threadTEST -lpthread

/* circular buffer of integers */
struct prodcons {
	int buffer [BUFFER_SIZE ]; /* the actual data */
	int read_pos, write_pos; /* positions for read and write */
	pthread_mutex_t lock; /* mutex ensuring exclusive */
	/* access to buffer */
	pthread_cond_t notempty; /* signaled when buffer is not empty */
	pthread_cond_t notfull; /* signaled when buffer is not full */
} buffer;

/* Initialize a buffer */
void init(struct prodcons * b){
	pthread_mutex_init(&(b->lock), NULL);
	pthread_cond_init(&(b->notempty), NULL);
	pthread_cond_init(&(b->notfull), NULL);
	b->read_pos = 0;
	b->write_pos = 0;
}

int get(struct prodcons *b){
	int data;
	pthread_mutex_lock(&b->lock);

	while (b->write_pos == b->read_pos) {
		/* Wait until buffer is not empty */
		pthread_cond_wait(&b->notempty, &b->lock);
	}

	data = b->buffer[b->read_pos];
	b->read_pos++;
	if (b->read_pos >= BUFFER_SIZE)
		b->read_pos = 0;

	// signal that the buffer is now not full
	pthread_cond_signal(&b->notfull);
	pthread_mutex_unlock(&b->lock);

	return data;
}

void put(struct prodcons *b, int data){
	pthread_mutex_lock(&b->lock);
	while ((b->write_pos + 1) % BUFFER_SIZE == b->read_pos){
		/* Wait until buffer is not empty */
		pthread_cond_wait(&b->notfull, &b->lock);
	}

	b->buffer[b->write_pos] = data;
	b->write_pos++;
	if (b->write_pos >= BUFFER_SIZE)
		b->write_pos = 0;
	// signal that the buffer is now not empty
	pthread_cond_signal(&b->notempty);
	pthread_mutex_unlock(&b->lock);
}

void * producer (void * par){
	int n;
	for (int n = 0; n < 33; n++){
		printf ("prod --> %d\n", n);
		put(&buffer, n);
	}
	put(&buffer, OVER);
	pthread_exit(NULL);
}

void * consumer (void * p){
	int d;
	while (1){
		d = get (&buffer);
		if (d == OVER) break;
		printf ("cons --> %d\n", d);
	}
	pthread_exit(NULL);
}

/** MAIN **/
int main (){
	pthread_t th_p, th_c;
	void * retval;
	init (&buffer);

	/* Create the threads */
	pthread_create (&th_p, NULL, producer, NULL);
	pthread_create (&th_c, NULL, consumer, NULL);

	/* Wait until producer and consumer finish */
	pthread_join (th_p, &retval);
	pthread_join (th_c, &retval);


	return 0;
}