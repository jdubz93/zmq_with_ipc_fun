#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include "shared_state.h"

int lock(int semid)
{
  struct sembuf sb = {0, -1, 0};
  if (semop(semid, &sb, 1) == -1)
  {
    perror("Failed to lock semaphore");
    return -1;
  }
  return 0;
}

int unlock(int semid)
{
  struct sembuf sb = {0, 1, 0};
  if (semop(semid, &sb, 1) == -1)
  {
    perror("Failed to unlock semaphore");
    return -1;
  }
  return 0;
}

int main()
{
  int shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
  if (shmid == -1)
  {
    perror("Failed to access shared memory");
    exit(1);
  }

  char *data = (char *)shmat(shmid, NULL, 0);
  if (data == (char *)-1)
  {
    perror("Failed to attach shared memory segment");
    exit(1);
  }

  int semid = semget(SEM_KEY, 1, 0666);
  if (semid == -1)
  {
    perror("Failed to access semaphore");
    exit(1);
  }

  char last_read[SHM_SIZE] = {0}; // stores last read value

  /* note:
  reader semaphore order: lock --> read --> unlock
  writer semaphore order: lock --> write --> unlock
  */
  while (1)
  {
    // wait (block) semaphore
    if (lock(semid) == -1)
    {
      break;
    }

    // check if data is different from the last read value
    if (strcmp(data, last_read) != 0)
    {
      printf("Read from shared memory: %s\n", data);
      strcpy(last_read, data);
    }

    if (unlock(semid) == -1)
    {
      break;
    }
  }

  return 0;
}
