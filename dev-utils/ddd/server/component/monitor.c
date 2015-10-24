#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "ddd_socket.h"
#include "component.h"
#include "cpu.h"

#define MONITOR_NAME     "monitor"

#define MONITOR_INTERVAL 1


/* monitor socket*/
static int monitor_sock = -1;

/* monitor client addr */
static struct sockaddr_in monitor_addr;

/* infomation collect thread */
static pthread_t collect_tid;

/* flag to conntrol if monitor send data to client */
static int running_flag = 0;


void* collect_thread(void* arg)
{
	char send_buf[BUF_MAX] = {0};
	socklen_t addr_len = (socklen_t)sizeof(monitor_addr);
	struct cpu_info_collection* cpu_data = get_cpu_data();
	time_t now;

	pthread_detach(pthread_self());

	for(;;)
	{
		if (running_flag)
		{
			/* record timestamp*/
			now = time(NULL);

			/* cpu information */
			collect_cpu_info();
			memset(send_buf, 0, BUF_MAX);
			sprintf(send_buf, "cpu$total$%lu$usage:%.2f\n", now,
					cpu_data->total_info.usage);
			sendto(monitor_sock, send_buf, strlen(send_buf), 0,
				   (struct sockaddr*)&monitor_addr, addr_len);

			/* memory info */
		}

		sleep(MONITOR_INTERVAL);
	}

	return NULL;
}


void* monitor_s(void* arg)
{
	char recv_buf[BUF_MAX] = {0};
	socklen_t addr_len = (socklen_t)sizeof(monitor_addr);

	if (0 != pthread_create(&collect_tid, NULL, collect_thread, NULL))
	{
		fprintf(stderr, "[monitor] create collect thread failed\n");
		return NULL;
	}

	monitor_sock = ddd_init_udp_socket(MONITOR_NAME, DDD_PORT_ANY, &monitor_addr);
	if (-1 == monitor_sock)
	{
		fprintf(stderr, "[monitor] create socket failed\n");
		return NULL;
	}

	for (;;)
	{
		memset(recv_buf, 0, BUF_MAX);
		recvfrom(monitor_sock, recv_buf, BUF_MAX, 0,
				 (struct sockaddr *)&monitor_addr, &addr_len);
		{
			printf("[monitor] recv command: %s\n", recv_buf);
			if (0 == strncmp("start", recv_buf, BUF_MAX))
			{
				running_flag = 1;
			}
			else if (0 == strncmp("stop", recv_buf, BUF_MAX))
			{
				running_flag = 0;
			}
			else
			{
				//...
			}
		}
	}

	close(monitor_sock);

	return NULL;
}
