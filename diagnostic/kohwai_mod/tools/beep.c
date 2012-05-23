#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>

#else

#include <stdlib.h>
#include <fcntl.h>
#include <linux/kd.h>

#endif

int beep(int freq, int ms)
{
#ifdef WIN32
    Beep(freq, ms);
    return 0;
#else
    int fd = open("/dev/tty10", O_RDONLY);
    if (fd == -1)
        return -1;    
    return ioctl(fd, KDMKTONE, (ms << 16) + (1193180 / freq));
#endif
}

