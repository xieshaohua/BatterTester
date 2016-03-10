#include "tester.h"

#define MONITOR_LOGBUF_SIZE		(1 * 1024)

static char logbuf[MONITOR_LOGBUF_SIZE + 1];

const char proptype_tbl[][TESTER_CONTENT_SIZE] = {
	{"POWER_SUPPLY_STATUS"},
	{"POWER_SUPPLY_PRESENT"},
	{"POWER_SUPPLY_BATTERY_CHARGING_ENABLED"},
	{"POWER_SUPPLY_CHARGING_ENABLED"},
	{"POWER_SUPPLY_CHARGE_TYPE"},
	{"POWER_SUPPLY_CAPACITY"},
	{"POWER_SUPPLY_HEALTH"},
	{"POWER_SUPPLY_CURRENT_NOW"},
	{"POWER_SUPPLY_VOLTAGE_NOW"},
	{"POWER_SUPPLY_TEMP"}
};

static size_t read_battinfo(const char *path, char *buf, size_t size)
{
	int fd;
	ssize_t count = 0;

	if (path == NULL)
		return -1;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("Could not open %s\n", path);
		return -1;
	}

	count = TEMP_FAILURE_RETRY(read(fd, buf, size));
	if (count > 0)
		buf[count] = '\0';

	close(fd);
	return count;
}

static int write_batterylog(const char *path, char *buf, int size)
{
	int fd;
	int count = 0;
	
	if ((path == NULL) || (buf == NULL))
		return -1;

	/* check and create batt_logs/logs if not exist */
	if (access(TESTER_LOG_FILE_LOG_DIR, R_OK | W_OK) == -1) {
		if (mkdir(TESTER_LOG_FILE_LOG_DIR, 0666) == -1) {
			printf("can't create %s:%s", TESTER_LOG_FILE_LOG_DIR, strerror(errno));
			return -1;
		}
	}

	fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0666);
	if (fd < 0) {
		printf("could not open:%s\n", strerror(fd));
		return -1;
	}

	count = TEMP_FAILURE_RETRY(write(fd, buf, size));
	if (count != size) {
		TESTER_DEBUG("write fail\n");
	}

	close(fd);
	return count;
}

int monitor_init(struct tester_status *status)
{
	return 0;
}

static int get_propnameval(char *content, char *name, char *val)
{
	int i = 0;
	int len = strlen(content);

	for (i = 0; i < len; i++) {
		if (content[i] == '=')
			break;
	}

	if (i == len) {
		printf("get_propnameval error: Could not find '='\n");
		return -1;
	}
	strncpy(name, content, i);
	name[i] = '\0';
	strncpy(val, content + i + 1, len - i - 1);
	val[len - i - 1] = '\0';
	return i;
}

static int battery_getprop(char *str, enum proptype type)
{
	int value = -1, pos = 0, cnt = 0;
	char content[TESTER_CONTENT_SIZE];
	char propname[TESTER_CONTENT_SIZE];
	char propval[TESTER_CONTENT_SIZE];

	//printf("str:\n%s", str);
	while ((cnt = get_next_content(str + pos, content)) != -1) {
		pos += cnt;
		//printf("content:%s\n", content);
		get_propnameval(content, propname, propval);
		//printf("%s:%s\n", propname, propval);
		if (strcmp(proptype_tbl[type], propname) == 0) {
			switch (type) {
			case POWER_SUPPLY_PRESENT:
			case POWER_SUPPLY_BATTERY_CHARGING_ENABLED:
			case POWER_SUPPLY_CHARGING_ENABLED:
			case POWER_SUPPLY_CAPACITY:
			case POWER_SUPPLY_CURRENT_NOW:
			case POWER_SUPPLY_VOLTAGE_NOW:
			case POWER_SUPPLY_TEMP:
				value = atoi(propval);
				break;
			case POWER_SUPPLY_STATUS:
			case POWER_SUPPLY_CHARGE_TYPE:
			case POWER_SUPPLY_HEALTH:
			default:
				value = -1;
				break;
			}
		}
	}
	return value;
}
	
static void parse_battprops(struct battery_props *props, char *str)
{
	int pos = 0, cnt = 0;

	props->status = battery_getprop(str, POWER_SUPPLY_STATUS);
	props->present = battery_getprop(str, POWER_SUPPLY_PRESENT);
	props->battery_charging_enabled = battery_getprop(str, POWER_SUPPLY_BATTERY_CHARGING_ENABLED);
	props->charging_enabled = battery_getprop(str, POWER_SUPPLY_CHARGING_ENABLED);
	props->charging_type = battery_getprop(str, POWER_SUPPLY_CHARGE_TYPE);
	props->capacity = battery_getprop(str, POWER_SUPPLY_CAPACITY);
	props->health = battery_getprop(str, POWER_SUPPLY_HEALTH);
	props->current_now = battery_getprop(str, POWER_SUPPLY_CURRENT_NOW);
	props->voltage_now = battery_getprop(str, POWER_SUPPLY_VOLTAGE_NOW);
	props->temp = battery_getprop(str, POWER_SUPPLY_TEMP);
	#if 0
	printf("props->present=%d\n", props->present);
	printf("props->battery_charging_enabled=%d\n", props->battery_charging_enabled);
	printf("props->charging_enabled=%d\n", props->charging_enabled);
	printf("props->capacity=%d\n", props->capacity);
	printf("props->current_now=%d\n", props->current_now);
	printf("props->voltage_now=%d\n", props->voltage_now);
	printf("props->temp=%d\n", props->temp);
	#endif
}

void monitor_updatelog(void)
{
	char *cp = logbuf;

	get_timestamp(cp);
	cp[TIME_TIMESTAMP_SIZE] = '\n';
	cp += TIME_TIMESTAMP_SIZE + 1;
	read_battinfo(POWER_SUPPLY_SYSFS, cp, MONITOR_LOGBUF_SIZE - TIME_TIMESTAMP_SIZE);
	parse_battprops(&tester_status.batt_props, cp);
	if (tester_status.log_enable && tester_status.log_path)
		write_batterylog(tester_status.log_path, logbuf, MONITOR_LOGBUF_SIZE);
	//printf("%s", logbuf);
}

