// shell/input/g29/G29EvdevOpen.cpp: open(2) for G29Evdev.cpp, in a file of its own. glibc's <fcntl.h> declares a
// function named tee() (splice's sibling), which clashes with this project's namespace tee wherever both are seen; so
// <fcntl.h> is included here only, outside the namespace, and G29Evdev.cpp calls these. Linux only.
#if defined(__linux__)
#include <fcntl.h>

// A file opened: 0 reading, 1 writing (truncated), 2 the node read and written without blocking (else read only when
// it may not be written); -1 and errno when it cannot be.
int teeG29Open(const char* path, int how)
{
    switch (how)
    {
        case 0: return ::open(path, O_RDONLY | O_CLOEXEC);
        case 1: return ::open(path, O_WRONLY | O_TRUNC | O_CLOEXEC);
        default:
        {
            const int fd = ::open(path, O_RDWR | O_NONBLOCK | O_CLOEXEC);
            if (fd >= 0)
                return fd;
            return ::open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        }
    }
}
#endif
