#include "file_utils.h"

#include <errno.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#define A53_MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define A53_MKDIR(path) mkdir(path, 0775)
#endif

static int is_separator(char ch)
{
    return ch == '/' || ch == '\\';
}

static int ensure_parent_directories(const char *path)
{
    char buffer[512];
    size_t length;
    size_t index;

    if (path == NULL) {
        return -1;
    }

    length = strlen(path);
    if (length >= sizeof(buffer)) {
        return -1;
    }

    memcpy(buffer, path, length + 1);
    for (index = 1; index < length; index++) {
        if (!is_separator(buffer[index])) {
            continue;
        }
        buffer[index] = '\0';
        if (strlen(buffer) > 0 && A53_MKDIR(buffer) != 0 && errno != EEXIST) {
            return -1;
        }
        buffer[index] = path[index];
    }

    return 0;
}

FILE *a53_open_append_text(const char *path)
{
    if (ensure_parent_directories(path) != 0) {
        return NULL;
    }
    return fopen(path, "ab");
}
