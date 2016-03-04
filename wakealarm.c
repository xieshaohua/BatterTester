#include "tester.h"

static int wakealarm_fd;

static void wakealarm_event(uint32_t epevents)
{
	unsigned long long wakeups;
	epevents = epevents;

	if (read(wakealarm_fd, &wakeups, sizeof(wakeups)) == -1) {
		TESTER_DEBUG("wakealarm_event: read wakealarm fd failed\n");
		return;
	}
	TESTER_DEBUG("wakealarm_event\n");
	monitor_updatelog();
}

static void wakealarm_set_interval(int interval)
{
	struct itimerspec itval;

	if (wakealarm_fd == -1)
		return;

	if (interval == -1)
		interval = 0;

	itval.it_interval.tv_sec = interval;
	itval.it_interval.tv_nsec = 0;
	itval.it_value.tv_sec = interval;
	itval.it_value.tv_nsec = 0;

	if (timerfd_settime(wakealarm_fd, 0, &itval, NULL) == -1)
		TESTER_DEBUG("wakealarm_set_interval: timerfd_settime failed\n");
}

void wakealarm_init(void)
{
	wakealarm_fd = timerfd_create(CLOCK_BOOTTIME_ALARM, TFD_NONBLOCK);
	if (wakealarm_fd == -1) {
		TESTER_DEBUG("wakealarm_init: timerfd_create failed\n");
		return;
	}

	if (tester_register_event(wakealarm_fd, wakealarm_event))
		TESTER_DEBUG("Registration of wakealarm event failed\n");

	wakealarm_set_interval(DEFAULT_PERIODIC_CHORES_INTERVAL);
}

