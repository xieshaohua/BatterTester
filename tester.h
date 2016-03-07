#ifndef _TESTER_H_
#define _TESTER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
//#include <io.h>
#include <time.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cutils/klog.h>

#define LOG_TAG				"battary_test"
#define TEST_LOGS_DIR_PATH		"/data/batt_logs"

#define TEST_STATE_ENABLE		TEST_LOGS_DIR_PATH"/enable"
#define TEST_STATE_ITEM			TEST_LOGS_DIR_PATH"/items"
#define TEST_STATE_STEP			TEST_LOGS_DIR_PATH"/step"

/* logs */
#define TEST_PON_CHARGING_LOG		TEST_LOGS_DIR_PATH"/poweron_charging.log"
#define TEST_PON_DISCHARGING_LOG	TEST_LOGS_DIR_PATH"/poweron_discharging.log"
#define TEST_POFF_CHARGING_LOG		TEST_LOGS_DIR_PATH"/poweroff_charging.log"
#define TEST_POFF_DISCHARGING_LOG	TEST_LOGS_DIR_PATH"/poweroff_discharging.log"



#define POWER_SUPPLY_SYSFS_PATH 	"/sys/class/power_supply/battery/uevent"
#define MONITOR_LOGBUF_SIZE		(1 * 1024)
#define TIME_TIMESTAMP_LEN		20


#define TESTER_DEBUG(fmt)	printf(fmt)
//#define TESTER_DEBUG(fmt)	KLOG_ERROR(LOG_TAG, fmt)

#define KLOG_LEVEL	6

#define MAX_EPOLL_EVENTS			20
#define DEFAULT_PERIODIC_CHORES_INTERVAL	5

/* items */
#define TESTER_ITEM_NULL			-1
#define TESTER_ITEM_PON_CHARGING		0
#define TESTER_ITEM_PON_DISCHARGING		1
#define TESTER_ITEM_POFF_CHARGING		2
#define TESTER_ITEM_POFF_DISCHARGING		3

/* steps */
#define TESTER_STEP_NULL			-1
#define TESTER_STEP_CHARGING			0
#define TESTER_STEP_DISCHARGING			1
#define TESTER_STEP_REBOOT			2

#define BATTERY_PROPS_ITEM_LEN			64
enum proptype{
	POWER_SUPPLY_STATUS = 0,
	POWER_SUPPLY_PRESENT,
	POWER_SUPPLY_BATTERY_CHARGING_ENABLED,
	POWER_SUPPLY_CHARGING_ENABLED,
	POWER_SUPPLY_CHARGE_TYPE,
	POWER_SUPPLY_CAPACITY,
	POWER_SUPPLY_HEALTH,
	POWER_SUPPLY_CURRENT_NOW,
	POWER_SUPPLY_VOLTAGE_NOW,
	POWER_SUPPLY_TEMP,
};

struct convert_items {
	char *name;
	int item_id;
};

struct convert_steps {
	char *name;
	int step_id;
};

#define MAX_ITEM_STEPS	10
struct item_steps {
	int step_id[MAX_ITEM_STEPS];
};

struct battery_props {
	int status;
	int present;
	int battery_charging_enabled;
	int charging_enabled;
	int charging_type;
	int capacity;
	int health;
	int current_now;
	int voltage_now;
	int temp;
};

struct tester_status {
	int item_id;
	int step_id;
	int log_enable;
	char *logfile;
	struct battery_props batt_props;
};

struct tester_mode_ops {
	int (*init)(struct tester_status *status);
	int (*preparetowait)(struct tester_status *status);
	void (*heartbeat)(struct tester_status *status);
	void (*update)(struct tester_status *status);
};

extern struct tester_status tester_status;


extern int get_next_content(char *str, char *content);

/* tester */
extern int tester_register_event(int fd, void (*handler)(uint32_t));
extern void tester_finish(void);

/* wakealarm */
extern void wakealarm_init(void);

/* monitor */
extern int monitor_init(struct tester_status *status);
extern void monitor_updatelog(void);

/* pon charging */
extern int pon_charging_init(struct tester_status *status);
extern int pon_charging_preparetowait(struct tester_status *status);
extern void pon_charging_heartbeat(struct tester_status *status);
extern void pon_charging_update(struct tester_status *status);


#endif
