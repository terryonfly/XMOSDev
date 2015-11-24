#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>


#define offsetof(TYPE, MEMBER) ((int)&((TYPE *)0)->MEMBER)


char *sock_addr = "/tmp/xmosd_led";

int main(int argc, char **argv)
{

    char *msg = "Hello, world\n";
    int addrlen, fd;

    struct sockaddr_un addr_un;

    addr_un.sun_family = AF_UNIX;
    strcpy(addr_un.sun_path, sock_addr);

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    addrlen = offsetof(struct sockaddr_un, sun_path) + strlen(addr_un.sun_path);
    if (connect(fd, (struct sockaddr *)&addr_un,  addrlen) < 0) {
        perror("connect");
        return -1;
    }


    while (1) {
        if (write(fd, argv[1], strlen(argv[1])) < 0)
            perror("write");
        usleep(500*1000);
    }


    close(fd);

    return 0;
}
