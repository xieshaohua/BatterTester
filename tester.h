#ifndef _TESTER_H_
#define _TESTER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cutils/klog.h>
#include <cutils/android_reboot.h>
#include <cutils/properties.h>


/* debug */
#define KLOG_LEVEL			6
#define KLOG_TAG			"battst"
//#define DEBUG(fmt, ...)			printf(fmt, ##__VA_ARGS__)
#define DEBUG(fmt, ...)			KLOG_INFO(KLOG_TAG, fmt, ##__VA_ARGS__)

/* log file dir */
#define TESTER_LOG_FILE_DIR		"/data/batt_logs"
#define TESTER_LOG_FILE_LOG_DIR		"/data/batt_logs/logs"

/* status files */
#define TESTER_STAT_FILE_ENABLE		TESTER_LOG_FILE_DIR"/enable"
#define TESTER_STAT_FILE_ITEMS		TESTER_LOG_FILE_DIR"/items"
#define TESTER_STAT_FILE_CASES		TESTER_LOG_FILE_DIR"/cases"

/* log files */
#define TESTER_PON_CHARGING_LOG		TESTER_LOG_FILE_LOG_DIR"/pon_charging.log"
#define TESTER_PON_DISCHARGING_LOG	TESTER_LOG_FILE_LOG_DIR"/pon_discharging.log"
#define TESTER_POFF_CHARGING_LOG	TESTER_LOG_FILE_LOG_DIR"/poff_charging.log"
#define TESTER_POFF_DISCHARGING_LOG	TESTER_LOG_FILE_LOG_DIR"/poff_discharging.log"

/* power supply sys node */
#define POWER_SUPPLY_SYSFS 		"/sys/class/power_supply/battery/uevent"

#define MAX_EPOLL_EVENTS			20
#define DEFAULT_WAKEALARM_INTERVAL		1
#define TIME_TIMESTAMP_SIZE			20

#define TESTER_ITEMS_BUF_SIZE			(4 * 1024)
#define TESTER_CONTENT_SIZE			64

/* items */
#define TESTER_ITEM_NULL			-1
#define TESTER_ITEM_PON_CHARGING		0
#define TESTER_ITEM_PON_DISCHARGING		1
#define TESTER_ITEM_POFF_CHARGING		2
#define TESTER_ITEM_POFF_DISCHARGING		3

struct items_convert {
	char *name;
	int item_id;
};

struct items_desc {
	int step_id_limit;
	const char **step_msg;
};

/* battery property */
enum proptype {
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

/* discharge */
enum discharge_method {
	DISCHARGE_METHOD_NULL		= 0x00,
	DISCHARGE_METHOD_CPU		= 0x01,
	DISCHARGE_METHOD_FLASH		= 0x02,
	DISCHARGE_METHOD_VIBRATE	= 0x04,
};

/* tester */
struct tester_status {
	int item_id;
	int step_id;
	const struct items_desc *items_desc;
	int log_enable;
	char *log_path;
	int alarm_interval;
	struct battery_props batt_props;
};

struct tester_mode_ops {
	int (*init)(struct tester_status *status);
	int (*preparetowait)(struct tester_status *status);
	void (*heartbeat)(struct tester_status *status);
	void (*update)(struct tester_status *status);
};

extern struct tester_status tester_status;

extern char *get_timestamp(char *buf);
extern int get_items_id(char *item);
extern char *get_items_name(int item_id);
extern int get_next_content(char *str, char *content);

/* tester */
extern int tester_register_event(int fd, void (*handler)(uint32_t));
extern void tester_finish(void);

/* wakealarm */
extern void wakealarm_init(struct tester_status *status);

/* monitor */
extern int monitor_init(struct tester_status *status);
extern void monitor_updatelog(void);

/* discharge */
extern void discharge_start(enum discharge_method method);
extern void discharge_stop(void);
extern void charging_enable(void);
extern void charging_disable(void);


/* pon charging */
extern const struct items_desc pon_charging_desc;
extern int pon_charging_init(struct tester_status *status);
extern void pon_charging_heartbeat(struct tester_status *status);


#endif
