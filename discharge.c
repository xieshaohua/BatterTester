#include <pthread.h>
#include "tester.h"

#define CPU_PTHREAD_MAX		8	/* total cpu number */

static struct discharge_status_type {
	pthread_t cpu_tid[CPU_PTHREAD_MAX];
	pthread_t flash_tid;
	pthread_t vibrate_tid;
	enum discharge_method discharge_method;
	int cpu_discharge_enable;
	int flash_discharge_enable;
	int vibrate_discharge_enable;
} discharge_status = {
	.discharge_method = 0,
	.cpu_discharge_enable = 0,
	.flash_discharge_enable = 0,
	.vibrate_discharge_enable = 0,
};

void charging_enable(void)
{
	if (system("echo 1 > /sys/class/power_supply/battery/charging_enabled") != 0)
		DEBUG("ERROR: charging_enable fail!\n");
}

void charging_disable(void)
{
	if (system("echo 0 > /sys/class/power_supply/battery/charging_enabled") != 0)
		DEBUG("ERROR: charging_disable fail!\n");
}

static void discharge_wake_lock(void)
{
	if (system("echo 'battst_discharge' > /sys/power/wake_lock") != 0)
		DEBUG("ERROR: discharge_wake_lock fail!\n");
}

static void discharge_wake_unlock(void)
{
	if (system("echo 'battst_discharge' > /sys/power/wake_unlock") != 0)
		DEBUG("ERROR: discharge_wake_unlock fail!\n");
}

static void *cpu_discharge_thread(void *arg)
{
	int i;
	double seed = 1.01d;
	DEBUG("INFO:[%ul] cpu_discharge_thread created\n", *((int *)arg));
	while(1) {
		double sum = 0;
		double *p = malloc(1024 * sizeof(double));
		if (!p) {
			DEBUG("ERROR: malloc error");
			return NULL;
		}
		for(i = 0; i < 1024; i++) {
			p[i] = p[i] * seed;
			sum = sum + p[i];
		}
		sum = sum / 1024.0;
		seed = seed * sum;
		free(p);
		if (!discharge_status.cpu_discharge_enable) {
			DEBUG("INFO:[%u] cpu_discharge_thread\n", *((int *)arg));
			discharge_status.discharge_method &= ~DISCHARGE_METHOD_CPU;
			return NULL;
		}
	};
}

static void *flash_discharge_thread(void *arg)
{
	while (1) {
		if (discharge_status.flash_discharge_enable) {
			if (system("echo 510 > /sys/class/leds/torch-light0/brightness") != 0)
				DEBUG("ERROR: flash_discharge_thread fail!\n");
			if (system("echo 510 > /sys/class/leds/torch-light1/brightness") != 0)
				DEBUG("ERROR: flash_discharge_thread fail!\n");

		} else {
			if (system("echo 0 > /sys/class/leds/torch-light0/brightness") != 0)
				DEBUG("ERROR: flash_discharge_thread fail!\n");
			if (system("echo 0 > /sys/class/leds/torch-light1/brightness") != 0)
				DEBUG("ERROR: flash_discharge_thread fail!\n");
			DEBUG("INFO: flash_discharge_thread exit\n");
			discharge_status.discharge_method &= ~DISCHARGE_METHOD_FLASH;
			return NULL;
		}
		sleep(5);
	}
}

static void *vibrate_discharge_thread(void *arg)
{
	while (1) {
		if (discharge_status.vibrate_discharge_enable) {
			if (system("echo 10000 > /sys/class/timed_output/vibrator/enable") != 0)
				DEBUG("ERROR: vibrate_discharge_thread fail!\n");
		} else {
			DEBUG("INFO: vibrate_discharge_thread exit\n");
			discharge_status.discharge_method &= ~DISCHARGE_METHOD_VIBRATE;
			return NULL;
		}
		sleep(5);
	}
}

void discharge_start(enum discharge_method method)
{
	int i, err;

	if (method == DISCHARGE_METHOD_NULL)
		return;

	discharge_wake_lock();
	discharge_status.discharge_method = method;

	/* cpu discharge method */
	if (method & DISCHARGE_METHOD_CPU) {
		if (!discharge_status.cpu_discharge_enable) {
			DEBUG("INFO: cpu discharge method start\n");
			discharge_status.cpu_discharge_enable = 1;
			for (i = 0; i < CPU_PTHREAD_MAX; i++) {
				err = pthread_create(&discharge_status.cpu_tid[i], NULL,
						cpu_discharge_thread, &discharge_status.cpu_tid[i]);
				if (err != 0) {
					DEBUG("ERROR:[%d] cpu_discharge_thread create error!\n", i);
					discharge_status.cpu_discharge_enable = 0;
					return;
				}
			}
		}
	}

	/* flash discharge method */
	if (method & DISCHARGE_METHOD_FLASH) {
		if (!discharge_status.flash_discharge_enable) {
			DEBUG("INFO: flash discharge method start\n");
			discharge_status.flash_discharge_enable = 1;
			err = pthread_create(&discharge_status.flash_tid, NULL,
						flash_discharge_thread, &discharge_status.flash_tid);
			if (err != 0) {
				DEBUG("ERROR: flash_discharge_thread create error!\n");
				discharge_status.flash_discharge_enable = 0;
				return;
			}
		}
	}

	/* vibrate discharge method */
	if (method & DISCHARGE_METHOD_VIBRATE) {
		if (!discharge_status.vibrate_discharge_enable) {
			DEBUG("INFO: vibrate discharge method start\n");
			discharge_status.vibrate_discharge_enable = 1;
			err = pthread_create(&discharge_status.vibrate_tid, NULL,
						vibrate_discharge_thread, &discharge_status.vibrate_tid);
			if (err != 0) {
				DEBUG("ERROR: vibrate_discharge_thread create error!\n");
				discharge_status.vibrate_discharge_enable = 0;
				return;
			}
		}
	}
}

void discharge_stop(void)
{
	discharge_status.cpu_discharge_enable = 0;
	discharge_status.flash_discharge_enable= 0;
	discharge_status.vibrate_discharge_enable = 0;
	do {
		DEBUG("INFO: waiting for discharge pthread stop...\n");
		sleep(2);
	} while(discharge_status.discharge_method != DISCHARGE_METHOD_NULL);
	discharge_wake_lock();
}
