#include "volc_errno.h"
#include <sys/errno.h>

int volc_errno(int fd) {
    return errno;
}