#include "stla_object_pipe.h"

#include "stla_allocator.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

struct stla_object_pipe_s {
  uv_poll_t read_poll;
  int read_fd;
  int write_fd;
  stla_object_pipe_f cb;
  stla_object_pipe_close_f close_cb;
  void *cb_arg;
};

static void setnonblock(int fd) {
  int flags = fcntl(fd, F_GETFL);
  if (flags < 0)
    abort();

  flags |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) < 0)
    abort();
}

static void _destroy_object_pipe(uv_handle_t *h) {
  stla_object_pipe_t *op = (stla_object_pipe_t *)(h->data);
  if (op->close_cb)
    op->close_cb(op->cb_arg);
  stla_free(op);
}

static void on_poll_receive(uv_poll_t *p, int status, int events) {
  stla_object_pipe_t *h = (stla_object_pipe_t *)p->data;
  char buffer[sizeof(void *) * 1];
  int n;
  while ((n = read(h->read_fd, buffer, sizeof(void *) * 1)) > 0) {
    void **ptr = (void **)buffer;
    void **ep = (void **)(buffer + n);
    while (ptr < ep) {
      if ((ssize_t)(*ptr) != -1) {
        h->cb(h->cb_arg, *ptr);
        ptr++;
      } else {
        uv_poll_stop(p);
        close(h->read_fd);
        close(h->write_fd);
        h->write_fd = -1;
        uv_close((uv_handle_t *)p, _destroy_object_pipe);
        return;
      }
    }
  }
}

#ifdef _STLA_DEBUG_MEMORY_
stla_object_pipe_t *_stla_object_pipe_open(uv_loop_t *loop,
                                           stla_object_pipe_f cb, void *arg,
                                           const char *caller) {
  stla_object_pipe_t *h = (stla_object_pipe_t *)_stla_malloc_d(
      NULL, caller, sizeof(stla_object_pipe_t), false);
#else
stla_object_pipe_t *_stla_object_pipe_open(uv_loop_t *loop,
                                           stla_object_pipe_f cb, void *arg) {
  stla_object_pipe_t *h =
      (stla_object_pipe_t *)stla_malloc(sizeof(stla_object_pipe_t));
#endif
  int fds[2];
  int n = pipe(fds);
  if (n < 0)
    abort();

  setnonblock(fds[0]);
  setnonblock(fds[1]);

  h->read_fd = fds[0];
  h->write_fd = fds[1];
  h->cb = cb;
  h->cb_arg = arg;
  h->close_cb = NULL;

  uv_poll_init(loop, &h->read_poll, fds[0]);
  h->read_poll.data = h;
  uv_poll_start(&h->read_poll, UV_READABLE, on_poll_receive);
  return h;
}

void stla_object_pipe_set_close_cb(stla_object_pipe_t *h,
                                   stla_object_pipe_close_f cb) {
  h->close_cb = cb;
}

static void _object_pipe_write(stla_object_pipe_t *h, ssize_t object) {
  if (h->write_fd == -1)
    abort();
  int n = write(h->write_fd, &object, sizeof(object));
  (void)n;
}

void stla_object_pipe_write(stla_object_pipe_t *h, void *object) {
  if ((ssize_t)(object) == -1) // protect -1 from getting through
    return;
  _object_pipe_write(h, (ssize_t)(object));
}

void stla_object_pipe_close(stla_object_pipe_t *h) {
  _object_pipe_write(h, -1);
}
