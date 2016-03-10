#include "tester.h"

static int epollfd;
static int eventct;
static struct tester_mode_ops *tester_mode_ops = NULL;

struct tester_status tester_status = {
	.log_enable = 0,
	.log_path = NULL,
	.alarm_interval = DEFAULT_WAKEALARM_INTERVAL,
};

static const struct items_convert items_tbl[] = {
	{"pon_charging",	TESTER_ITEM_PON_CHARGING},
	{"pon_discharging",	TESTER_ITEM_PON_DISCHARGING},
	{"poff_charging",	TESTER_ITEM_POFF_CHARGING},
	{"poff_discharging",	TESTER_ITEM_POFF_DISCHARGING},
	{"item_null",		TESTER_ITEM_NULL},
};

static const struct items_desc *items_desc_tbl[] = {
	[TESTER_ITEM_PON_CHARGING]	= &pon_charging_desc,
	[TESTER_ITEM_PON_DISCHARGING]	= &pon_charging_desc,
	[TESTER_ITEM_POFF_CHARGING]	= &pon_charging_desc,
	[TESTER_ITEM_POFF_DISCHARGING]	= &pon_charging_desc,
};

static struct tester_mode_ops pon_charging_ops = {
	.heartbeat = pon_charging_heartbeat,
};

static struct tester_mode_ops *tester_mode_ops_tbl[] = {
	[TESTER_ITEM_PON_CHARGING]	= &pon_charging_ops,
	[TESTER_ITEM_PON_DISCHARGING]	= &pon_charging_ops,
	[TESTER_ITEM_POFF_CHARGING]	= &pon_charging_ops,
	[TESTER_ITEM_POFF_DISCHARGING]	= &pon_charging_ops,
};

int tester_register_event(int fd, void (*handler)(uint32_t))
{
	struct epoll_event ev;

	ev.events = EPOLLIN | EPOLLWAKEUP;
	ev.data.ptr = (void *)handler;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		TESTER_DEBUG("epoll_ctl failed; errno\n");
		return -1;
	}
	eventct++;
	return 0;
}

static int tester_init(void)
{
	epollfd = epoll_create(MAX_EPOLL_EVENTS);
	if (epollfd == -1) {
		TESTER_DEBUG("epoll_create failed; errno=\n");
		return -1;
	}
	monitor_init(&tester_status);
	wakealarm_init(&tester_status);

	if (tester_mode_ops->init)
		return tester_mode_ops->init(&tester_status);
	else
		return 0;
}

int get_next_content(char *str, char *content)
{
	int i = 0, st = -1;

	if ((str == NULL) || (content == NULL))
		return -1;

	/* find next content begining with displayable characters and end with '\n' or '\0' */
	do {
		if (st != -1) {
			if ((str[i] == '\n') || (str[i] == '\0')) {
				strncpy(content, str + st, i - st);
				content[i - st] = '\0';
				return (i + 1);
			}
		} else {
			/* if the ascii characters which can display */
			if ((str[i] >= 32) && (str[i] <= 126))
				st = i;
		}
	} while (str[i++] != '\0');
	return -1;
}

int first_ready_item(char *str, char *item, char *step)
{
	int i, len, cnt, pos = 0;
	char buf[TESTER_CONTENT_SIZE + 1];
	
	if (str == NULL)
		return -1;

	while ((cnt = get_next_content(str + pos, buf)) != -1) {
		len = strlen(buf);
		for (i = 0; i < len; i++) {
			if (buf[i] == '=')
				break;
		}
		if (i == len)
			return -1;
		if (buf[i + 1] != 'y') {
			if ((item != NULL) && (step != NULL)) {
				strncpy(item, buf, i);
				item[i] = '\0';
				strncpy(step, buf + i + 1, len - i - 1);
				step[len - i - 1] = '\0';
			}
			return (pos + i + 1);	/* step_id position */
		}
		pos += cnt;
	}
	return -1;
}

int get_items_id(char *item)
{
	int i;

	for (i = 0; i < (int)(sizeof(items_tbl)/sizeof(items_tbl[0])); i++) {
		if (strcmp(items_tbl[i].name, item) == 0)
			return items_tbl[i].item_id;
	}
	return TESTER_ITEM_NULL;
}

char *get_items_name(int item_id)
{
	int i;

	for (i = 0; i < (int)(sizeof(items_tbl)/sizeof(items_tbl[0])); i++) {
		if (items_tbl[i].item_id == item_id)
			return items_tbl[i].name;
	}
	return NULL;
}

char *get_timestamp(char *buf)
{
	time_t now;
	struct tm *tt;

	time(&now);
	tt = localtime(&now);
	snprintf(buf, TIME_TIMESTAMP_SIZE, "%04d-%02d-%02d %02d:%02d:%02d",
				tt->tm_year + 1900, tt->tm_mon, tt->tm_mday,
				tt->tm_hour, tt->tm_min, tt->tm_sec);
	return buf;
}

static int tester_parse_items(void)
{
	int fd, cnt, size, pos = 0;
	char buf[TESTER_CONTENT_SIZE + 1];
	char *pitems, *pcases, *str;

	printf("parsing items and updating cases\n");
	pitems = malloc(TESTER_ITEMS_BUF_SIZE + 1);
	pcases = malloc(TESTER_ITEMS_BUF_SIZE + 1);
	if ((pitems == NULL) || (pcases == NULL)) {
		printf("parse items malloc error!\n");
		goto error;
	}
	fd = open(TESTER_STAT_FILE_ITEMS, O_RDONLY);
	if (fd < 0) {
		printf("%s open fail!\n", TESTER_STAT_FILE_ITEMS);
		goto error;
	}
	cnt = read(fd, pitems, TESTER_ITEMS_BUF_SIZE);
	close(fd);
	if (cnt < 1) {
		printf("%s may be empty!\n", TESTER_STAT_FILE_ITEMS);
		goto error;
	}
	pitems[cnt] = '\0';
	printf("items:\n%s", pitems);
	/* verify items and generate cases file */
	str = pcases;
	while ((cnt = get_next_content(pitems + pos, buf)) != -1) {
		pos += cnt;
		if (get_items_id(buf) == TESTER_ITEM_NULL) {
			printf("invalid item: %s\n", buf);
			goto error;
		}
		size = snprintf(str, TESTER_CONTENT_SIZE, "%s=0\n", buf);
		str += size;
	}
	printf("cases:\n%s\n", pcases);
	fd = open(TESTER_STAT_FILE_CASES, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		printf("%s open fail:%s\n", TESTER_STAT_FILE_CASES, strerror(errno));
		goto error;
	}
	size = strlen(pcases);
	cnt = write(fd, pcases, size);
	close(fd);
	if (cnt != size) {
		printf("%s: need %d but write %d\n", strerror(errno), size, cnt);
		goto error;
	}
	free(pitems);
	free(pcases);
	return 0;
error:
	if (pitems) free(pitems);
	if (pcases) free(pcases);
	return -1;
}

static int tester_poweron_init(void)
{
	int fd, err, cnt = 0;
	char *pcases;
	char buf1[TESTER_CONTENT_SIZE + 1];
	char buf2[TESTER_CONTENT_SIZE + 1];
	struct stat items_stat, cases_stat;

	/* check and create batt_logs/enable if not exist */
	if (access(TESTER_LOG_FILE_DIR, R_OK | W_OK) == -1) {
		printf("%s not exist!\n", TESTER_LOG_FILE_DIR);
		if (mkdir(TESTER_LOG_FILE_DIR, 0666) == -1) {
			printf("can't create %s:%s", TESTER_LOG_FILE_DIR, strerror(errno));
			return -1;
		}
		if (creat(TESTER_STAT_FILE_ENABLE, 0666) == -1) {
			printf("can't create %s:%s", TESTER_STAT_FILE_ENABLE, strerror(errno));
			return -1;
		}
		printf("create /batt_logs/enable success\n");
		return -1;
	}

	/* check enable */
	fd = open(TESTER_STAT_FILE_ENABLE, O_RDONLY);
	if (fd < 0) {
		printf("can't open %s:%s\n", TESTER_STAT_FILE_ENABLE, strerror(errno));
		return -1;
	}
	memset(buf1, 0, sizeof(buf1));
	cnt = read(fd, buf1, sizeof(buf1));
	close(fd);
	if ((cnt != 2) || buf1[0] != '1') {
		printf("enable = %s\n", buf1);
		return -1;
	}
	/* check items and cases file */
	if (stat(TESTER_STAT_FILE_ITEMS, &items_stat) != 0) {
		printf("%s stat error:%s\n", TESTER_STAT_FILE_ITEMS, strerror(errno));
		return -1;
	}
	err = stat(TESTER_STAT_FILE_CASES, &cases_stat);
	if ((err != 0) && (errno != ENOENT)) {
		printf("%s stat error!:%s\n", TESTER_STAT_FILE_CASES, strerror(errno));
		return -1;
	}
	if ((errno == ENOENT) || (items_stat.st_mtime > cases_stat.st_mtime)) {
		if (tester_parse_items() != 0)
			return -1;
		/* delete logs directory */
		if (system("rm -rf /data/batt_logs/logs") != 0) {
			printf("can't del %s\n", TESTER_LOG_FILE_LOG_DIR);
			return -1;
		}
	}
	/* readout cases files and set items_id/step_id */
	pcases = malloc(TESTER_ITEMS_BUF_SIZE + 1);
	if (pcases == NULL) {
		printf("pcases malloc error!\n");
		return -1;
	}
	fd = open(TESTER_STAT_FILE_CASES, O_RDONLY);
	if (fd < 0) {
		printf("can't open %s:%s\n", TESTER_STAT_FILE_CASES, strerror(errno));
		goto error;
	}
	cnt = read(fd, pcases, TESTER_ITEMS_BUF_SIZE);
	close(fd);
	if ((cnt < 1)) {
		printf("%s may be empty:%s\n", TESTER_STAT_FILE_CASES, strerror(errno));
		goto error;
	}
	pcases[cnt] = '\0';
	printf("cases:\n%s\n", pcases);
	if (first_ready_item(pcases, buf1, buf2) == -1) {
		printf("can't find first ready item\n");
		goto error;
	}
	printf("first ready item:%s, current step:%s\n", buf1, buf2);
	tester_status.item_id = get_items_id(buf1);
	if (tester_status.item_id == TESTER_ITEM_NULL) {
		printf("invalid item: %s\n", buf1);
		goto error;
	}
	tester_status.items_desc = items_desc_tbl[tester_status.item_id];
	tester_status.step_id = atoi(buf2);
	if ((tester_status.step_id >= tester_status.items_desc->step_id_limit)
	    || (tester_status.step_id < 0)) {
		printf("invalid step id\n");
		goto error;
	}
	free(pcases);
	return 0;
error:
	free(pcases);
	return -1;
}

static void tester_mainloop(void)
{
	int i;

	while (1) {
		struct epoll_event events[eventct];
		int nevents;
		int timeout = -1;

		if (tester_mode_ops->preparetowait)
			timeout = tester_mode_ops->preparetowait(&tester_status);
		nevents = epoll_wait(epollfd, events, eventct, timeout);

		if (nevents == -1) {
			if (errno == EINTR)
				continue;
			TESTER_DEBUG("tester_mainloop: epoll_wait failed\n");
			break;
		}

		for (i = 0; i < nevents; i++) {
			if (events[i].data.ptr)
				(*(void (*)(int))events[i].data.ptr)(events[i].events);
		}

		if (!nevents && tester_mode_ops->update)
			tester_mode_ops->update(&tester_status);

		if (tester_mode_ops->heartbeat)
			tester_mode_ops->heartbeat(&tester_status);
	}
	return;
}

int main(void)
{
	int ret;

	ret = tester_poweron_init();
	if (ret) {
		printf("tester_poweron_init fail!\n");
		return ret;
	}
	
	printf("[tester start]: item_id=%d:%s\n", tester_status.item_id,
						  get_items_name(tester_status.item_id));
	printf("[tester start]: step_id=%d:%s\n", tester_status.step_id,
				tester_status.items_desc->step_msg[tester_status.step_id]);

	tester_mode_ops = tester_mode_ops_tbl[tester_status.item_id];
	if (tester_mode_ops == NULL) {
		printf("tester_mode_ops == NULL\n");
		return -1;
	}
	
	ret = tester_init();
	if (ret) {
		TESTER_DEBUG("Initialization failed, exiting\n");
		exit(-1);
	}
	tester_mainloop();
	TESTER_DEBUG("tester run error!\n");
	return ret;
}

void tester_finish(void)
{
	int fd, pos, cnt, size;
	char *pcases, buf[2];

	printf("[tester finish]:%s\n", tester_status.items_desc->step_msg[tester_status.step_id]);
	pcases = malloc(TESTER_ITEMS_BUF_SIZE);
	if (pcases == NULL) {
		printf("tester_finish malloc error!\n");
		return;
	}
	fd = open(TESTER_STAT_FILE_CASES, O_RDONLY);
	if (fd < 0) {
		printf("can't open %s:%s\n", TESTER_STAT_FILE_CASES, strerror(errno));
		goto error;
	}
	cnt = read(fd, pcases, TESTER_ITEMS_BUF_SIZE);
	close(fd);
	if ((cnt < 1)) {
		printf("%s may be empty:%s\n", TESTER_STAT_FILE_CASES, strerror(errno));
		goto error;
	}
	pcases[cnt] = '\0';
	pos = first_ready_item(pcases, NULL, NULL);
	snprintf(buf, sizeof(buf), "%d", tester_status.step_id);
	if (pcases[pos] != buf[0]) {
		printf("match error:step_id=%d, buf=%s\n", tester_status.step_id, buf);
		goto error;
	}
	/* reboot if finished the whole item test or increase step_id and write back */
	if (++tester_status.step_id >= tester_status.items_desc->step_id_limit)
		pcases[pos] = 'y';
	else
		pcases[pos] = ++buf[0];

	fd = open(TESTER_STAT_FILE_CASES, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		printf("%s open fail:%s\n", TESTER_STAT_FILE_CASES, strerror(errno));
		goto error;
	}
	size = strlen(pcases);
	cnt = write(fd, pcases, size);
	if (cnt != size) {
		printf("%s: need %d but write %d\n", strerror(errno), size, cnt);
		goto error;
	}
	if (pcases[pos] == 'y') {
		free(pcases);
		close(fd);
		printf("reboot system...\n");
		android_reboot(ANDROID_RB_RESTART, 0, 0);
		while (1);
	}

error:
	free(pcases);
	close(fd);
}

