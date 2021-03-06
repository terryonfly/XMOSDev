#include <string.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#ifdef ANDROID
#include <cutils/sockets.h>
#include <cutils/log.h>
#define printf ALOGV
#else
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#endif

#include "xmosd.h"
#include "rokid_xmos.h"

int running = 1;

char* join_char(char *s1, char *s2)
{
	char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
	//in real code you would check for errors in malloc here
	if (result == NULL) exit (1);

	strcpy(result, s1);
	strcat(result, s2);

	return result;
}

void find_bcd_device() {
	DIR *dirp;
	struct dirent *dp;
	dirp = opendir("/sys/bus/usb/devices/");
	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_type != DT_LNK) continue;
		struct dirent *sub_dp;
		DIR *sub_dirp;
		char *dir_path = join_char(join_char("/sys/bus/usb/devices/", dp->d_name), "/");
		sub_dirp = opendir(dir_path);
		int pid_ok = 0;
		int vid_ok = 0;
		while ((sub_dp = readdir(sub_dirp)) != NULL) {
			if (strcmp(sub_dp->d_name,"idProduct") == 0) {
				int file = open(join_char(dir_path, sub_dp->d_name), O_RDONLY);
				char spid[4];
				read(file, spid, 4);
				if (spid[0] == '0' && spid[1] == '0' && spid[2] == '0' && spid[3] == '8') {
					pid_ok = 1;
				}
				close(file);
			}
			if (strcmp(sub_dp->d_name,"idVendor") == 0) {
				int file = open(join_char(dir_path, sub_dp->d_name), O_RDONLY);
				char svid[4];
				read(file, svid, 4);
				if (svid[0] == '2' && svid[1] == '0' && svid[2] == 'b' && svid[3] == '1') {
					vid_ok = 1;
				}
				close(file);
			}
		}
		if (pid_ok && vid_ok) {
			int file = open(join_char(dir_path, "bcdDevice"), O_RDONLY);
			char bcdD[5];
			read(file, bcdD, 4);
			bcdDevice = atoi(bcdD);
			close(file);
			break;
		}
	}
	printf("bcdDevice = %d\n", bcdDevice);
	closedir(dirp);
}

extern struct sock_func *sock_funcs;
extern struct hub_func hub_func;

/* TODO: Handler sock_func */
/* Internal members */

#define MAX_EVENTS (3*16)
struct listen_conns {
	int listenfd;
	int connsfd;
};

int watch_listen(int epollfd, int listenfd)
{
	int flag;
	struct epoll_event ev;

	flag = fcntl(listenfd, F_GETFL, 0);
	fcntl(listenfd, F_SETFL, flag | O_NONBLOCK);
	ev.events = EPOLLIN;
	ev.data.fd = listenfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
		perror("epoll_ctl: socket_led");
		exit(-1);
	}

	return 0;
}

int watch_conn(int epollfd, int listenfd)
{
	struct sockaddr local;
	socklen_t addrlen;
	int conn;
	struct epoll_event ev;
	int flag;


	memset(&local, 0, sizeof(local));
	memset(&addrlen, 0, sizeof(socklen_t));
	conn = accept(listenfd, (struct sockaddr *)&local, &addrlen);
	if (conn == -1) {
		perror("accept");
		exit(-1);
	}

	ev.events = EPOLLIN;
	ev.data.fd = conn;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn, &ev) == -1) {
		perror("epoll_ctl");
		exit(-1);
	}
	flag = fcntl(conn, F_GETFL, 0);
	fcntl(conn, F_SETFL, flag | O_NONBLOCK);

	return conn;
}

/* event loop */
int event_handler(struct sock_func *sfs)
{
	int efd,  nfds;
	int conn;
	struct sock_func *sf;
	struct epoll_event events[MAX_EVENTS];
	struct listen_conns  lcs[MAX_EVENTS];


	memset(lcs, 0, sizeof(lcs));
	efd = epoll_create(256);
	if (efd == -1) {
		perror("epoll_create1");
		exit(-1);
	}

	if (hub_func.hubfd)
		watch_listen(efd, hub_func.hubfd);

	sf = sfs;
	while (sf->fd > 0) {
		watch_listen(efd, sf->fd);
		sf++;
	}


	while (running) {

		int n;
		nfds = epoll_wait(efd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			perror("epoll_wait");
			continue;
			/* avoid strace exist */
			/* exit(-1); */
		}

		for (n = 0; n < nfds; ++n) {
			struct listen_conns *lc;

			if(events[n].events & EPOLLHUP) { /* connection HUP */
				struct epoll_event ev;

				epoll_ctl(efd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
				close(events[n].data.fd);
				printf("colse -> %d\n", lc->connsfd);

				lc = (struct listen_conns *)&lcs;
				while (lc->connsfd != events[n].data.fd)
					lc++;

				/* delete a record */
				lc->listenfd = -1;
				lc->connsfd = -1;
				continue;
			}

			sf = sfs;
			while (sf->fd > 0) {
				if (events[n].data.fd == sf->fd)
					break;
				sf++;
			}

			if (sf->fd > 0) { /* listen */
				conn = watch_conn(efd, sf->fd);

				/* add a record */
				lc = (struct listen_conns *)&lcs;
				while (lc->listenfd > 0)
					lc++;
				lc->listenfd = sf->fd;
				lc->connsfd = conn;
			} else { /* connection */
				int status;

				if (events[n].data.fd == hub_func.hubfd) {
					hub_func.func(hub_func.hubfd, sfs);
					continue;
				} else {
					lc = (struct listen_conns *)&lcs;
					while (lc->connsfd != events[n].data.fd)
						lc++;

					sf = sfs;
					while (sf->fd != lc->listenfd)
						sf++;

					status = sf->func(lc->connsfd, hub_func.hubfd);

					if (status == 0) { /* file descriptor closed */
						printf("status %d\n", status);
					} else if (status < 0) { /* if EAGAIN as non-block */
						printf("%d status %d\n", lc->connsfd, status);
					}
				}
			}
		}
	}

	return 0;
}

#ifdef ANDROID
int main(void)
{
	find_bcd_device();

	struct sock_func *sf;

	ALOGI("Xmosd started");

	hub_func.hubfd = preprocess();
	if (hub_func.hubfd < 0) {
		ALOGE("preprocess failed");
		return -1;
	}

	sf = &sock_funcs;
	while (sf->socket) {
		sf->fd = android_get_control_socket(sf->socket);
		if (listen(sf->fd, 4) < 0) {
			perror("listen");
			return -1;
		}
		sf++;
	}

	event_handler((struct sock_func *)&sock_funcs);

	return 0;
}

#else
void cs(int n)
{
	running = 0;
	printf("now dowm!\n");
	exit(0);
}

int create_socket(char *name)
{
	struct sockaddr_un addr;
	int fd;

	fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", name);
	unlink(addr.sun_path);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return -1;
	}

	return fd;
}

int main(void)
{
	printf("now alive!\n");

	signal(SIGINT, cs);  //ctrl+c
	signal(SIGTERM, cs);  //kill

	find_bcd_device();

	char spath[1024] = {0};
	struct sock_func *sf;

	hub_func.hubfd = preprocess();
	if (hub_func.hubfd < 0) {
		return -1;
	}

	sf = (struct sock_func *)&sock_funcs;
	while (sf->socket) {
		snprintf(spath, sizeof(spath), "/tmp/%s", sf->socket);
		sf->fd = create_socket(spath);
		if (listen(sf->fd, 4) < 0) {
			perror("listen");
			return -1;
		}
		sf++;
	}

	event_handler((struct sock_func *)&sock_funcs);

	return 0;
}
#endif
