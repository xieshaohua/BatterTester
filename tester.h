#ifndef _BATTST_H_
#define _BATTST_H_

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

#include "lua51/lauxlib.h"
#include "lua51/lualib.h"
#include "lua51/lua.h"

/* debug */
#define KLOG_LEVEL			6
#define KLOG_TAG			"battst"
//#define DEBUG(fmt, ...)			printf(fmt, ##__VA_ARGS__)
#define DEBUG(fmt, ...)			KLOG_INFO(KLOG_TAG, fmt, ##__VA_ARGS__)

/* log file dir */
#define DIR_LOGS_ROOT			"/data/batt_logs"
#define DIR_LOGS_FILE		"/data/batt_logs/logs"

/* status files */
#define STATUS_FILE_ENABLE		DIR_LOGS_ROOT"/enable"
#define STATUS_FILE_ITEMS		DIR_LOGS_ROOT"/items"
#define STATUS_FILE_CASES		DIR_LOGS_ROOT"/cases"

/* log files */
#define LOG_PON_CHARGING		DIR_LOGS_FILE"/pon_charging.log"
#define LOG_PON_DISCHARGING		DIR_LOGS_FILE"/pon_discharging.log"
#define LOG_POFF_CHARGING		DIR_LOGS_FILE"/poff_charging.log"
#define LOG_POFF_DISCHARGING		DIR_LOGS_FILE"/poff_discharging.log"

/* power supply sys node */
#define POWER_SUPPLY_SYSFS 		"/sys/class/power_supply/battery/uevent"

#define MAX_EPOLL_EVENTS			20
#define DEFAULT_WAKEALARM_INTERVAL		1
#define TIME_TIMESTAMP_SIZE			20

#define ITEMS_BUF_SIZE				(4 * 1024)
#define CONTENT_BUF_SIZE			64

/* item id */
#define ID_NULL					-1
#define ID_PON_CHARGING				0
#define ID_PON_DISCHARGING			1
#define ID_POFF_CHARGING			2
#define ID_POFF_DISCHARGING			3

/* steps */
enum {
	ITEM_STEP_0 = 0,
	ITEM_STEP_1,
	ITEM_STEP_2,
	ITEM_STEP_3,
	ITEM_STEP_4,
	ITEM_STEP_5,
	ITEM_STEP_6,
	ITEM_STEP_7,
	ITEM_STEP_8,
	ITEM_STEP_9,
};

struct step_desc {
	int step_id_limit;
	const char **step_msg;
};

struct item_desc {
	char *name;
	int item_id;
	const struct step_desc *step_desc;
	struct tester_mode_ops *ops;
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
struct battst_status {
	int step_id;
	int log_enable;
	char *log_path;
	int alarm_interval;
	const struct item_desc *item_desc;
	struct battery_props batt_props;
};

struct battst_mode_ops {
	int (*init)(struct battst_status *status);
	int (*preparetowait)(struct battst_status *status);
	void (*heartbeat)(struct battst_status *status);
	void (*update)(struct battst_status *status);
};

extern struct battst_status battst_status;

/* utils */
extern char *get_timestamp(char *buf);
extern int read_file(const char *path, char *buf, int size);
extern int write_file(const char *path, const char *buf, int size);
extern int get_next_content(const char *str, char *content);
extern int first_ready_item(const char *str, char *item, char *step);

/* tester */
extern int tester_register_event(int fd, void (*handler)(uint32_t));
extern void tester_finish(void);
extern int get_item_id(char *item);
//extern char *get_item_name(int item_id);

/* wakealarm */
extern void wakealarm_init(struct battst_status *status);

/* monitor */
extern int monitor_init(struct battst_status *status);
extern void monitor_updatelog(void);

/* discharge */
extern void discharge_start(enum discharge_method method);
extern void discharge_stop(void);
extern void charging_enable(void);
extern void charging_disable(void);


/* pon charging */

//extern int pon_charging_init(struct tester_status *status);
//extern void pon_charging_heartbeat(struct tester_status *status);


#endif	/* _BATTST_H_ */
