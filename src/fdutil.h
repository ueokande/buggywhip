#ifndef FDUTIL_H
#define FDUTIL_H

#include <unistd.h>

static inline int write_all(int fd, const void *buf, size_t count) {
  while (count) {
    ssize_t tmp;

    errno = 0;
    tmp = write(fd, buf, count);
    if (tmp > 0) {
      count -= tmp;
      if (count) {
        buf = (void *) ((char *) buf + tmp);
      }
    } else if (errno != EINTR && errno != EAGAIN)
      return -1;
    if (errno == EAGAIN) {   /* Try later, *sigh* */
      usleep(250000);
    }
  }
  return 0;
}

#endif
