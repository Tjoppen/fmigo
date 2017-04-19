//for some reason copying a string safely on all platforms is still a problem in the year 2017
//this is an implementation of https://www.sudo.ws/todd/papers/strlcpy.html
//it's not the most efficient, but it gets the job done
#ifndef HAVE_STRLCPY  //try to play nice with a hypothetical future build system
#define HAVE_STRLCPY
#include <string.h> //for strlen() and memcpy()

static size_t strlcpy(char *dst, const char *src, size_t size) {
    size_t ret = strlen(src);
    size_t bytes = ret < size-1 ? ret : size-1;
    memcpy(dst, src, bytes); //assume strings don't overlap, else we'd use memmove()
    dst[bytes] = 0;
    return ret;
}
#endif

#ifndef HAVE_STRLCAT
#define HAVE_STRLCAT
#include <string.h>

static size_t strlcat(char *dst, const char *src, size_t size) {
    size_t a = strlen(dst);
    size_t b = strlen(src);
    size_t ret = a + b;
    size_t maxbytes = size-1 - a;
    size_t bytes = b > maxbytes ? maxbytes : b;
    memcpy(dst + a, src, bytes);
    dst[a+bytes] = 0;
    return ret;
}
#endif
