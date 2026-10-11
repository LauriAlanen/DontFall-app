#include <cstdio>
#include <sys/utsname.h>

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

int main()
{
    utsname sys {};
    uname(&sys);
    std::printf("dontfall-app %s on %s %s (%s)\n",
                APP_VERSION, sys.sysname, sys.release, sys.machine);
    return 0;
}
