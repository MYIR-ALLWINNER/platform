/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

#include "autosuspend_ops.h"

#define EARLYSUSPEND_SYS_POWER_STATE "/sys/power/state"

static int sPowerStatefd;
static const char *pwr_state_mem = "mem";
static const char *pwr_state_on = "on";
static pthread_t suspend_thread;
static pthread_mutex_t suspend_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t suspend_cond = PTHREAD_COND_INITIALIZER;
static bool wait_for_suspend;
static enum {
	EARLYSUSPEND_ON,
	EARLYSUSPEND_MEM,
} suspend_state = EARLYSUSPEND_ON;
extern void (*wakeup_func) (bool success);

#ifndef __unused
#define __unused        __attribute__((__unused__))
#endif

int suspend_set_state(unsigned int state)	//0-on, 1-mem
{
	if (state != EARLYSUSPEND_ON && state != EARLYSUSPEND_MEM)
		return -1;
	if (wait_for_suspend) {
		pthread_mutex_lock(&suspend_mutex);
		suspend_state = state;
		pthread_cond_signal(&suspend_cond);
		pthread_mutex_unlock(&suspend_mutex);
	}
	return 0;
}

int suspend_wait_resume(void)
{
	if (wait_for_suspend) {
		pthread_mutex_lock(&suspend_mutex);
		while (suspend_state == EARLYSUSPEND_MEM)
			pthread_cond_wait(&suspend_cond, &suspend_mutex);
		pthread_mutex_unlock(&suspend_mutex);
	}
	return 0;
}

static void *suspend_thread_func(void __unused * arg)
{
	int ret = 0;
	bool success;
	while (1) {

		pthread_mutex_lock(&suspend_mutex);
		while (suspend_state == EARLYSUSPEND_ON) {
			pthread_cond_wait(&suspend_cond, &suspend_mutex);
		}
		pthread_mutex_unlock(&suspend_mutex);
		printf("prepare standy!\n");
		/*state change callback[wake->suspend] */

		printf("%s: write %s to %s\n", __func__, pwr_state_mem, EARLYSUSPEND_SYS_POWER_STATE);
		ret = TEMP_FAILURE_RETRY(write(sPowerStatefd, pwr_state_mem, strlen(pwr_state_mem)));
		if (ret < 0) {
			success = false;
		}
		printf("sleep 2s, wait for enter standby, ret=%d\n", ret);
		sleep(2);
		pthread_mutex_lock(&suspend_mutex);
		suspend_state = EARLYSUSPEND_ON;
		pthread_cond_signal(&suspend_cond);
		pthread_mutex_unlock(&suspend_mutex);
		ret = TEMP_FAILURE_RETRY(write(sPowerStatefd, pwr_state_on, strlen(pwr_state_on)));
		/*state change callback[suspend->wake] */
		printf("resume, perform wakeup function!\n");
		void (*func) (bool success) = wakeup_func;
		if (func != NULL) {
			(*func) (success);
		}
	}

	return NULL;
}

static int autosuspend_suspend_enable(void)
{
	char buf[80];
	int ret = -1;

	printf("autosuspend_suspend_enable\n");
	ret = write(sPowerStatefd, pwr_state_on, strlen(pwr_state_on));
	if (ret < 0) {
		strerror_r(errno, buf, sizeof(buf));
		printf("Error writing to %s: %s\n", EARLYSUSPEND_SYS_POWER_STATE, buf);
		goto err;
	}
	if (wait_for_suspend) {
		pthread_mutex_lock(&suspend_mutex);
		while (suspend_state != EARLYSUSPEND_ON) {
			pthread_cond_wait(&suspend_cond, &suspend_mutex);
		}
		pthread_mutex_unlock(&suspend_mutex);
	}

	printf("autosuspend_suspend_enable done\n");

	return 0;

err:
	return ret;
}

static int autosuspend_suspend_disable(void)
{
	char buf[80];
	int ret = -1;

	printf("autosuspend_suspend_disable\n");

	ret = TEMP_FAILURE_RETRY(write(sPowerStatefd, pwr_state_on, strlen(pwr_state_on)));
	if (ret < 0) {
		strerror_r(errno, buf, sizeof(buf));
		printf("Error writing to %s: %s\n", EARLYSUSPEND_SYS_POWER_STATE, buf);
		goto err;
	}

	if (wait_for_suspend) {
		pthread_mutex_lock(&suspend_mutex);
		while (suspend_state != EARLYSUSPEND_ON) {
			pthread_cond_wait(&suspend_cond, &suspend_mutex);
		}
		pthread_mutex_unlock(&suspend_mutex);
	}

	printf("autosuspend_suspend_disable done\n");

	return 0;

err:
	return ret;
}

struct autosuspend_ops autosuspend_suspend_ops = {
	.enable = autosuspend_suspend_enable,
	.disable = autosuspend_suspend_disable,
};

struct autosuspend_ops *autosuspend_suspend_init(void)
{
	char buf[80];
	int ret = -1;

	sPowerStatefd = TEMP_FAILURE_RETRY(open(EARLYSUSPEND_SYS_POWER_STATE, O_RDWR));
	if (sPowerStatefd < 0) {
		strerror_r(errno, buf, sizeof(buf));
		printf("Error opening %s: %s\n", EARLYSUSPEND_SYS_POWER_STATE, buf);
		return NULL;
	}

	ret = TEMP_FAILURE_RETRY(write(sPowerStatefd, "on", 2));
	if (ret < 0) {
		strerror_r(errno, buf, sizeof(buf));
		printf("Error writing 'on' to %s: %s\n", EARLYSUSPEND_SYS_POWER_STATE, buf);
		goto err_write;
	}

	printf("Selected suspend\n");
	ret = pthread_create(&suspend_thread, NULL, suspend_thread_func, NULL);
	if (ret) {
		strerror_r(errno, buf, sizeof(buf));
		printf("Error creating thread: %s\n", buf);
		goto err_write;
	}

	wait_for_suspend = true;

	return &autosuspend_suspend_ops;

err_write:
	close(sPowerStatefd);
	return NULL;
}
