#include <string.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#ifdef ANDROID
#include <cutils/sockets.h>
#include <cutils/log.h>
#define printf ALOGV
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <signal.h>
#endif

#include "rokid_xmos.h"

struct xmos_dev *xmos_dd;

int running = 1;

int preprocess(void)
{
	return 0;
}

/* process xmosd_amp connection */
int process_amp(int fd)
{
	char buf[64*1024];
	int num;

	memset(buf, 0, sizeof(buf));

	do {
		num = read(fd, buf, sizeof(buf));
		if (num < 0) {
			if (errno != EAGAIN) {
				close(fd);
				perror("read");
				return -1;
			}
			break;
		} else if (num == 0) {
			printf("%d closed", fd);
			close(fd);
		} else {	/* Process data */
			printf("amp: %s %d\n", buf, num);
		}
	} while(0);


	return 0;

}

/* process xmosd_len connection */
int process_led(int fd)
{
	char buf[64*1024];
	int num;

	memset(buf, 0, sizeof(buf));

	do {
		num = read(fd, buf, sizeof(buf));
		if (num < 0) {
			if (errno != EAGAIN) {
				close(fd);
				perror("read");
				return -1;
			}
			break;
		} else if (num == 0) {
			printf("%d closed", fd);
			close(fd);
		} else {	/* Process data */
			printf("%s %d\n", buf, num);
		}
	} while(0);

	return 0;
}


/* TODO: Handler sock_func */
/* Internal members */
int socket_led, socket_ammeter, socket_amp;
#define MAX_EVENTS (3*16)
struct fd_func {
	int fd;
	int (*func)(int fd);
};

int watch_listen(int epollfd, int listenfd)
{
	int flag;
	struct epoll_event ev;

	flag = fcntl(socket_led, F_GETFL, 0);
	fcntl(socket_led, F_SETFL, flag | O_NONBLOCK);
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
int event_handler(void)
{
	int efd,  nfds;
	int conn;
	struct epoll_event events[MAX_EVENTS];
	struct fd_func fd_func[MAX_EVENTS];


	memset(fd_func, 0, sizeof(fd_func));
	efd = epoll_create(256);
	if (efd == -1) {
		perror("epoll_create1");
		exit(-1);
	}

	watch_listen(efd, socket_led);
	watch_listen(efd, socket_amp);

	while (1) {
		int n, i;

		nfds = epoll_wait(efd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			perror("epoll_wait");
			continue;
			/* exit(-1); */ /* avoid strace exist */
		}

		for (n = 0; n < nfds; ++n) {
			if (events[n].data.fd == socket_led) {
				conn = watch_conn(efd, socket_led);
				i = 0;
				while (fd_func[i].fd)
					i++;

				fd_func[i].fd = conn;
				fd_func[i].func = process_led;
			} else if (events[n].data.fd == socket_amp) {
				conn = watch_conn(efd, socket_amp);
				i = 0;
				while (fd_func[i].fd)
					i++;

				fd_func[i].fd = conn;
				fd_func[i].func = process_amp;
			} else {
				/* process connection */
				int i;

				for (i=0; i<MAX_EVENTS; i++)
					if (fd_func[i].fd == events[n].data.fd)
						fd_func[i].func(fd_func[i].fd);
				/* something else */
				/* TODO: Should callback close fd */
			}
		}
	}

	return 0;
}

//int main(int argc, char **argv)
//{
//	printf("now alive!\n");
//
//	int r;
//	xmos_dd = xmos_dev_open(&r);
//	printf("dev open success.\n");
//
//	int k = 0;
//	while (running) {
//		unsigned char *one_frame = (unsigned char *)malloc(LED_COUNT * 3);
//		int i;
//		for (i = 0; i < LED_COUNT * 3; i ++) {
//			if (i / 3 == k) {
//				if (i % 3 == 0) one_frame[i] = 0xff;
//				if (i % 3 == 1) one_frame[i] = 0xff;
//				if (i % 3 == 2) one_frame[i] = 0x00;
//			} else {
//				if (i % 3 == 0) one_frame[i] = 0x00;
//				if (i % 3 == 1) one_frame[i] = 0x00;
//				if (i % 3 == 2) one_frame[i] = 0x00;
//
//			}
//		}
//		k ++;
//		if (k >= LED_COUNT) k = 0;
//
//		r = xmos_dev_led_flush_frame(xmos_dd, one_frame, LED_COUNT * 3);
//		usleep(17*1000);
//	}
//
//	return 0;
//}

#ifdef ANDROID
int main(void)
{
	ALOGI("Xmosd started");

	socket_led = android_get_control_socket("xmosd_led");
	socket_ammeter = android_get_control_socket("xmosd_ammeter");
	socket_amp = android_get_control_socket("xmosd_amp");

	if (listen(socket_led, 4) < 0 ||
	    listen(socket_ammeter, 4) < 0 ||
	    listen(socket_amp, 4) < 0) {
		ALOGE("Failed to listen xmosd_devs");
		return -1;
	}

	event_handler();

	return 0;
}

#else
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
	socket_led = create_socket("/tmp/xmosd_led");
	socket_ammeter =  create_socket("/tmp/xmosd_ammeter");
	socket_amp = create_socket("/tmp/xmosd_amp");

	if (listen(socket_led, 4) < 0 ||
		listen(socket_ammeter, 4) < 0 ||
		listen(socket_amp, 4) < 0) {
		perror("Failed listen xmosd_devs");
		return -1;
	}

	event_handler();

	return 0;

}
#endif
