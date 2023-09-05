#include <zmq.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "shared_state.h"

// Semaphore operations
/*
Mutex = thread sync
Semaphore = ipc sync
note: Mutex: Has ownership. The thread that locks the mutex should be the one to unlock it.
Semaphore: Doesn't have the concept of ownership.
Any process can increment (release or "signal") or decrement (acquire or "wait on") a semaphore.
*/
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
  void *context = zmq_ctx_new();
  void *publisher = zmq_socket(context, ZMQ_PUB);

  if (!context || !publisher)
  {
    fprintf(stderr, "failed to set up zmq. exiting...\n");
    return 1;
  }

  if (zmq_bind(publisher, "tcp://*:5556") != 0)
  {
    fprintf(stderr, "failed to bind zmq socket. exiting...\n");
    zmq_close(publisher);
    zmq_ctx_destroy(context);
    return 1;
  }

  // IPC setup
  int shmid = shmget(SHM_KEY, sizeof(shared_state_t), 0666 | IPC_CREAT);
  if (shmid == -1)
  {
    perror("failed to get shared memory segment");
    zmq_close(publisher);
    zmq_ctx_destroy(context);
    return 1;
  }

  shared_state_t *state = (shared_state_t *)shmat(shmid, NULL, 0);
  if (state == (void *)-1)
  {
    perror("failed to attach shared memory segment");
    shmctl(shmid, IPC_RMID, NULL);
    zmq_close(publisher);
    zmq_ctx_destroy(context);
    return 1;
  }

  int semid = semget(SEM_KEY, 1, 0666 | IPC_CREAT);
  if (semid == -1)
  {
    perror("failed to get semaphore");
    shmdt(state);
    shmctl(shmid, IPC_RMID, NULL);
    zmq_close(publisher);
    zmq_ctx_destroy(context);
    return 1;
  }

  if (semctl(semid, 0, SETVAL, 1) == -1)
  {
    perror("failed to initialize semaphore");
    shmdt(state);
    shmctl(shmid, IPC_RMID, NULL);
    zmq_close(publisher);
    zmq_ctx_destroy(context);
    return 1;
  }
  int i = 0;
  while (1)
  {
    char new_message[SHM_SIZE];
    sprintf(new_message, "herro from pub %d, %d!", getpid(), i++);

    // update shared state
    if (lock(semid) == -1 || unlock(semid) == -1)
    {
      break;
    }
    strncpy(state->message, new_message, sizeof(state->message) - 1);
    state->message[sizeof(state->message) - 1] = '\0'; // add null-terminator
    // unlocks semaphore & signal to the reader that new data is available
    if (unlock(semid) == -1)
    {
      break;
    }
    zmq_send(publisher, state->message, strlen(state->message), 0); // publish message over zmq
    sleep(1);                                                       // slows down messaging
  }

  // cleanup
  shmdt(state);
  shmctl(shmid, IPC_RMID, NULL);
  semctl(semid, 0, IPC_RMID, 0);
  zmq_close(publisher);
  zmq_ctx_destroy(context);

  return 0;
}
