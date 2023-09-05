#include <zmq.h>
#include <stdio.h>

int main()
{
  void *context = zmq_ctx_new();
  void *subscriber = zmq_socket(context, ZMQ_SUB);
  zmq_connect(subscriber, "tcp://localhost:5556");
  zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

  while (1)
  {
    char buffer[256];
    zmq_recv(subscriber, buffer, sizeof(buffer), 0);
    printf("recv: %s\n", buffer);
  }

  // Cleanup
  zmq_close(subscriber);
  zmq_ctx_destroy(context);
  return 0;
}
