#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

typedef uint8_t u8;

typedef struct thr_state {
  unsigned i;
  pthread_t thread_id;
  unsigned char *secret;
  struct timespec waittime;
  volatile unsigned *all_start;
  float op_sec;
  float total_sec;
  unsigned exited;
  
} thr_state;

extern void curve25519_donna(u8 *output, const u8 *secret, const u8 *bp);

static uint64_t
time_now() {
  struct timeval tv;
  uint64_t ret;

  gettimeofday(&tv, NULL);
  ret = tv.tv_sec;
  ret *= 1000000;
  ret += tv.tv_usec;

  return ret;
}

#define DEFAULT_THREAD_COUNT 4

static const unsigned char basepoint[32] = {9};

static void *run( void *state )  {
  thr_state *st = (thr_state*)state;
  unsigned char mypublic[32];
  uint64_t start, end;
  unsigned i;
  const unsigned iterations = 100000;

  printf("Waiting in thread %d ...\n", st->i+1);
  while( *st->all_start==0 ) ; 		/* spin */
  //printf("Proceed to testing in thread %d\n", st->i);

  // Load the caches
  for (i = 0; i < 1000; ++i) {
    curve25519_donna(mypublic, st->secret, basepoint);
  }

  start = time_now();
  for (i = 0; i < iterations; ++i) {
    curve25519_donna(mypublic, st->secret, basepoint);
  }
  end = time_now();

  st->op_sec = iterations*1000000. / (end - start);
  st->total_sec = (end - start)/1000000.;
  st->exited = 1;
  //printf("Exited thread %d\n", st->i);
  return NULL;
}

int
main() {
  unsigned char mysecret[32];
  unsigned i;
  thr_state *st;
  unsigned n;
  float total_op_sec;
  volatile unsigned all_start=0;
  unsigned ncores;
  unsigned threads_count;

  ncores = sysconf( _SC_NPROCESSORS_ONLN );
  if( ncores == -1 )  {
    ncores = DEFAULT_THREAD_COUNT;
    printf("Cannot determine the number of cores; assume %d\n", ncores);
  }
  threads_count = ncores+1;

  st = malloc( sizeof(thr_state) * threads_count );
  if( st == NULL )  {
    printf("Out of memory\n");
    return 1;
  }

  printf("Will use %d threads\n", threads_count);

  memset(mysecret, 42, 32);
  mysecret[0] &= 248;
  mysecret[31] &= 127;
  mysecret[31] |= 64;

  /* start the threads */
  for( i=0; i<threads_count; i++ )  {
    st[i].i = i;
    st[i].secret = mysecret;
    st[i].all_start = &all_start;
    st[i].exited = 0;

    if(pthread_create( &st[i].thread_id, NULL, run, st+i )) {
      fprintf(stderr, "Error creating thread %d\n", i);
      return 1;
    }
  }

  sleep(1);
  all_start = 1;	/* go! */

  /* wait for all the threads to finish */
  for( n=0; n!=threads_count; usleep(1) )  {
    n=0;
    for( i=0; i<threads_count; i++ )  {
      n += st[i].exited;
    }
  }

  total_op_sec=0;
  for( i=0; i<threads_count; i++ )  {
    printf("thread %d: %g op/sec in %.02f sec\n", i+1, st[i].op_sec, st[i].total_sec);
    total_op_sec += st[i].op_sec;
  }
  printf("Total for CPU: %g op/sec\n", total_op_sec);

  /* don't bother with cleaning resources */

  return 0;
}
