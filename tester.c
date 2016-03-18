#include "tester.h"

static int epollfd;
static int eventct;
static struct battst_mode_ops *battst_mode_ops = NULL;

struct battst_status battst_status = {
	.log_enable = 0,
	.log_path = NULL,
	.alarm_interval = DEFAULT_WAKEALARM_INTERVAL,
};

extern const struct item_desc pon_charging_item_desc;

static const struct item_desc *item_desc_tbl[] = {
	&pon_charging_item_desc,
};


int battst_register_event(int fd, void (*handler)(uint32_t))
{
	struct epoll_event ev;

	ev.events = EPOLLIN | EPOLLWAKEUP;
	ev.data.ptr = (void *)handler;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		DEBUG("epoll_ctl failed; errno\n");
		return -1;
	}
	eventct++;
	return 0;
}

static int battst_init(void)
{
	epollfd = epoll_create(MAX_EPOLL_EVENTS);
	if (epollfd == -1) {
		DEBUG("epoll_create failed; errno=\n");
		return -1;
	}
	monitor_init(&battst_status);
	wakealarm_init(&battst_status);

	if (battst_mode_ops->init)
		return battst_mode_ops->init(&battst_status);
	else
		return 0;
}

const struct item_desc *get_item_desc(const char *item)
{
	int i;

	for (i = 0; i < (int)(sizeof(item_desc_tbl)/sizeof(item_desc_tbl[0])); i++) {
		if (strcmp(item_desc_tbl[i]->name, item) == 0)
			return item_desc_tbl[i];
	}
	return NULL;
}

int get_item_id(char *item)
{
	int i;

	for (i = 0; i < (int)(sizeof(item_desc_tbl)/sizeof(item_desc_tbl[0])); i++) {
		if (strcmp(item_desc_tbl[i]->name, item) == 0)
			return item_desc_tbl[i]->item_id;
	}
	return ID_NULL;
}

static int battst_parse_items(void)
{
	int fd, cnt, size, pos = 0;
	char buf[CONTENT_BUF_SIZE + 1];
	char *pitems, *pcases, *str;

	DEBUG("parsing items and updating cases\n");
	pitems = malloc(ITEMS_BUF_SIZE + 1);
	pcases = malloc(ITEMS_BUF_SIZE + 1);
	if ((pitems == NULL) || (pcases == NULL)) {
		DEBUG("parse items malloc error!\n");
		goto error;
	}
	fd = open(STATUS_FILE_ITEMS, O_RDONLY);
	if (fd < 0) {
		DEBUG("%s open fail!\n", STATUS_FILE_ITEMS);
		goto error;
	}
	cnt = read(fd, pitems, ITEMS_BUF_SIZE);
	close(fd);
	if (cnt < 1) {
		DEBUG("%s may be empty!\n", STATUS_FILE_ITEMS);
		goto error;
	}
	pitems[cnt] = '\0';
	DEBUG("items:\n%s", pitems);
	/* verify items and generate cases file */
	str = pcases;
	while ((cnt = get_next_content(pitems + pos, buf)) != -1) {
		pos += cnt;
		if (get_item_id(buf) == ID_NULL) {
			DEBUG("invalid item: %s\n", buf);
			goto error;
		}
		size = snprintf(str, CONTENT_BUF_SIZE, "%s=0\n", buf);
		str += size;
	}
	DEBUG("cases:\n%s\n", pcases);
	fd = open(STATUS_FILE_CASES, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		DEBUG("%s open fail:%s\n", STATUS_FILE_CASES, strerror(errno));
		goto error;
	}
	size = strlen(pcases);
	cnt = write(fd, pcases, size);
	close(fd);
	if (cnt != size) {
		DEBUG("%s: need %d but write %d\n", strerror(errno), size, cnt);
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

static void battst_poweroff_check(void)
{
	char property_val[PROP_VALUE_MAX];

	property_get("ro.bootmode", property_val, "");
	DEBUG("ro.bootmode=%s\n", property_val);
	if (!strcmp("charger", property_val)) {
		DEBUG("booting from charger mode!\n");
		#if 0
		if ((tester_status.item_id != TESTER_ITEM_POFF_CHARGING) ||
		    (tester_status.item_id != TESTER_ITEM_POFF_DISCHARGING)) {
		    	DEBUG("reboot system...\n");
			android_reboot(ANDROID_RB_RESTART, 0, 0);
			while (1);
		}
		#endif
	}
}

static int battst_poweron_init(void)
{
	int fd, err, cnt = 0;
	char *pcases;
	char buf1[CONTENT_BUF_SIZE + 1];
	char buf2[CONTENT_BUF_SIZE + 1];
	struct stat items_stat, cases_stat;

	/* check and create batt_logs/enable if not exist */
	if (access(DIR_LOGS_ROOT, R_OK | W_OK) == -1) {
		DEBUG("%s not exist!\n", DIR_LOGS_ROOT);
		if (mkdir(DIR_LOGS_ROOT, 0666) == -1) {
			DEBUG("can't create %s:%s", DIR_LOGS_ROOT, strerror(errno));
			return -1;
		}
		if (creat(STATUS_FILE_ENABLE, 0666) == -1) {
			DEBUG("can't create %s:%s", STATUS_FILE_ENABLE, strerror(errno));
			return -1;
		}
		DEBUG("create /batt_logs/enable success\n");
		return -1;
	}

	/* check enable */
	fd = open(STATUS_FILE_ENABLE, O_RDONLY);
	if (fd < 0) {
		DEBUG("can't open %s:%s\n", STATUS_FILE_ENABLE, strerror(errno));
		return -1;
	}
	memset(buf1, 0, sizeof(buf1));
	cnt = read(fd, buf1, sizeof(buf1));
	close(fd);
	if ((cnt != 2) || buf1[0] != '1') {
		DEBUG("enable = %s\n", buf1);
		return -1;
	}
	/* check items and cases file */
	if (stat(STATUS_FILE_ITEMS, &items_stat) != 0) {
		DEBUG("%s stat error:%s\n", STATUS_FILE_ITEMS, strerror(errno));
		return -1;
	}
	err = stat(STATUS_FILE_CASES, &cases_stat);
	if ((err != 0) && (errno != ENOENT)) {
		DEBUG("%s stat error!:%s\n", STATUS_FILE_CASES, strerror(errno));
		return -1;
	}
	if ((errno == ENOENT) || (items_stat.st_mtime > cases_stat.st_mtime)) {
		if (battst_parse_items() != 0)
			return -1;
		/* delete logs directory */
		if (system("rm -rf /data/batt_logs/logs") != 0) {
			DEBUG("can't del %s\n", DIR_LOGS_FILE);
			return -1;
		}
	}
	/* readout cases files and set items_id/step_id */
	pcases = malloc(ITEMS_BUF_SIZE + 1);
	if (pcases == NULL) {
		DEBUG("pcases malloc error!\n");
		return -1;
	}
	fd = open(STATUS_FILE_CASES, O_RDONLY);
	if (fd < 0) {
		DEBUG("can't open %s:%s\n", STATUS_FILE_CASES, strerror(errno));
		goto error;
	}
	cnt = read(fd, pcases, ITEMS_BUF_SIZE);
	close(fd);
	if ((cnt < 1)) {
		DEBUG("%s may be empty:%s\n", STATUS_FILE_CASES, strerror(errno));
		goto error;
	}
	pcases[cnt] = '\0';
	DEBUG("cases:\n%s\n", pcases);
	if (first_ready_item(pcases, buf1, buf2) == -1) {
		DEBUG("can't find first ready item\n");
		goto error;
	}
	DEBUG("first ready item:%s, current step:%s\n", buf1, buf2);
	battst_status.item_desc = get_item_desc(buf1);
	if (battst_status.item_desc == NULL) {
		DEBUG("invalid item desc: %s\n", buf1);
		goto error;
	}
	//tester_status.items_desc = items_desc_tbl[tester_status.item_id];
	battst_status.step_id = atoi(buf2);
	if ((battst_status.step_id >= battst_status.item_desc->step_desc->step_id_limit)
	    || (battst_status.step_id < 0)) {
		DEBUG("invalid step id\n");
		goto error;
	}
	free(pcases);
	return 0;
error:
	free(pcases);
	return -1;
}

static void battst_mainloop(void)
{
	int i;

	while (1) {
		struct epoll_event events[eventct];
		int nevents;
		int timeout = -1;

		if (battst_mode_ops->preparetowait)
			timeout = battst_mode_ops->preparetowait(&battst_status);
		nevents = epoll_wait(epollfd, events, eventct, timeout);

		if (nevents == -1) {
			if (errno == EINTR)
				continue;
			DEBUG("tester_mainloop: epoll_wait failed\n");
			break;
		}

		for (i = 0; i < nevents; i++) {
			if (events[i].data.ptr)
				(*(void (*)(int))events[i].data.ptr)(events[i].events);
		}

		if (!nevents && battst_mode_ops->update)
			battst_mode_ops->update(&battst_status);

		if (battst_mode_ops->heartbeat)
			battst_mode_ops->heartbeat(&battst_status);
	}
	return;
}

int main(void)
{
	int ret = 0;

	klog_set_level(KLOG_LEVEL);

	ret = battst_poweron_init();
	if (ret) {
		DEBUG("tester_poweron_init fail!\n");
		return ret;
	}

	battst_poweroff_check();

	DEBUG("[tester start]: item_id=%d:%s\n", battst_status.item_desc->item_id,
						battst_status.item_desc->name);
	DEBUG("[tester start]: step_id=%d:%s\n", battst_status.step_id,
			battst_status.item_desc->step_desc->step_msg[battst_status.step_id]);

	battst_mode_ops = battst_status.item_desc->ops;
	if (battst_mode_ops == NULL) {
		DEBUG("tester_mode_ops == NULL\n");
		return -1;
	}
	
	ret = tester_init();
	if (ret) {
		DEBUG("Initialization failed, exiting\n");
		exit(-1);
	}
	battst_mainloop();
	DEBUG("tester run error!\n");
	return ret;
}

void battst_finish(void)
{
	int fd, pos, cnt, size;
	char *pcases, buf[2];

	DEBUG("[tester finish]:%s\n", battst_status.item_desc->step_desc->step_msg[battst_status.step_id]);
	pcases = malloc(ITEMS_BUF_SIZE);
	if (pcases == NULL) {
		DEBUG("tester_finish malloc error!\n");
		return;
	}
	fd = open(STATUS_FILE_CASES, O_RDONLY);
	if (fd < 0) {
		DEBUG("can't open %s:%s\n", STATUS_FILE_CASES, strerror(errno));
		goto error;
	}
	cnt = read(fd, pcases, ITEMS_BUF_SIZE);
	close(fd);
	if ((cnt < 1)) {
		DEBUG("%s may be empty:%s\n", STATUS_FILE_CASES, strerror(errno));
		goto error;
	}
	pcases[cnt] = '\0';
	pos = first_ready_item(pcases, NULL, NULL);
	snprintf(buf, sizeof(buf), "%d", battst_status.step_id);
	if (pcases[pos] != buf[0]) {
		DEBUG("match error:step_id=%d, buf=%s\n", battst_status.step_id, buf);
		goto error;
	}
	/* reboot if finished the whole item test or increase step_id and write back */
	if (++battst_status.step_id >= battst_status.item_desc->step_desc->step_id_limit)
		pcases[pos] = 'y';
	else
		pcases[pos] = ++buf[0];

	fd = open(STATUS_FILE_CASES, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		DEBUG("%s open fail:%s\n", STATUS_FILE_CASES, strerror(errno));
		goto error;
	}
	size = strlen(pcases);
	cnt = write(fd, pcases, size);
	if (cnt != size) {
		DEBUG("%s: need %d but write %d\n", strerror(errno), size, cnt);
		goto error;
	}
	if (pcases[pos] == 'y') {
		free(pcases);
		close(fd);
		DEBUG("reboot system...\n");
		android_reboot(ANDROID_RB_RESTART, 0, 0);
		while (1);
	}

error:
	free(pcases);
	close(fd);
}

