/*
 * Copyright (C) 2016 The AllWinner Project
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <stdio.h>

#include <suspend/autosuspend.h>
#include <power_manager.h>
#include <np_input.h>
#include <scene_manager.h>

#ifdef USE_DBUS
#include <dbus_server.h>
static pthread_t dbus_thread;
#endif

extern int set_sleep_state(unsigned int state);

void wakeup_callback(bool sucess)
{
	set_sleep_state(0);
	acquireWakeLock(NATIVE_POWER_DISPLAY_LOCK);
	invalidateActivityTime();
}

int main(int argc, char **argv)
{
#ifdef USE_DBUS
	int ret = -1;
	char buf[80];
#endif

	acquireWakeLock(NATIVE_POWER_DISPLAY_LOCK);
	autosuspend_enable();
	set_wakeup_callback(wakeup_callback);
	init_awake_timeout();

	np_input_init();
	openlog("NativePower", LOG_NOWAIT, LOG_DAEMON);

#ifdef USE_DBUS
	ret = pthread_create(&dbus_thread, NULL, dbus_server_thread_func, NULL);
	if (ret) {
		strerror_r(ret, buf, sizeof(buf));
		goto end;
	}
#endif
	while (1) {
		if (!isActivityTimeValid()) {
			goToSleep(100, 1, 0);
		}

		sleep(1);
	}

#ifdef USE_DBUS
end:
#endif

	autosuspend_disable();
	printf("EXIT ERROR!!!!");
}
