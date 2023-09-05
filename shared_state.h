#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SHM_KEY 1234 // Shared Memory Key
#define SEM_KEY 5678 // Semaphore Key
#define SHM_SIZE 256 // size of shared memory in bytes

typedef struct
{
  char message[SHM_SIZE];
} shared_state_t;

#endif
