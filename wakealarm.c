#include "tester.h"

static int wakealarm_fd;

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
		DEBUG("wakealarm_set_interval: timerfd_settime failed\n");
}

static void wakealarm_event(uint32_t epevents)
{
	static int wakeup_time = 0;
	unsigned long long wakeups;
	epevents = epevents;

	if (read(wakealarm_fd, &wakeups, sizeof(wakeups)) == -1) {
		DEBUG("wakealarm_event: read wakealarm fd failed\n");
		return;
	}
	//DEBUG("wakealarm_event\n");
	monitor_updatelog();
	
	if (wakeup_time != battst_status.alarm_interval) {
		wakeup_time = battst_status.alarm_interval;
		wakealarm_set_interval(wakeup_time);
	}
}

void wakealarm_init(struct battst_status *status)
{
	wakealarm_fd = timerfd_create(CLOCK_BOOTTIME_ALARM, TFD_NONBLOCK);
	if (wakealarm_fd == -1) {
		DEBUG("wakealarm_init: timerfd_create failed\n");
		return;
	}

	if (tester_register_event(wakealarm_fd, wakealarm_event))
		DEBUG("Registration of wakealarm event failed\n");

	wakealarm_set_interval(status->alarm_interval);
}
