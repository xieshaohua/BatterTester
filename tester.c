#include "tester.h"

static int epollfd;
static int eventct;
static struct tester_mode_ops *tester_mode_ops = NULL;

struct tester_status tester_status;

static const struct convert_items items_tbl[] = {
	{"pon_charging",	TESTER_ITEM_PON_CHARGING},
	{"pon_discharging",	TESTER_ITEM_PON_DISCHARGING},
	{"poff_charging",	TESTER_ITEM_POFF_CHARGING},
	{"poff_discharging",	TESTER_ITEM_POFF_DISCHARGING},
	{"item_null",		TESTER_ITEM_NULL},
};

static const struct convert_steps steps_tbl[] = {
	{"charging",	TESTER_STEP_CHARGING},
	{"discharging",	TESTER_STEP_DISCHARGING},
	{"reboot",	TESTER_STEP_REBOOT},
	{"step_null",	TESTER_STEP_NULL},
};

static const struct item_steps item_steps_tbl[] = {
	[TESTER_ITEM_PON_CHARGING] = {
		.step_id = {
			TESTER_STEP_DISCHARGING,
			TESTER_STEP_CHARGING,
			TESTER_STEP_NULL,
		},
	},
	[TESTER_ITEM_PON_DISCHARGING] = {
		.step_id = {
		},
	},
};

static struct tester_mode_ops pon_charging_ops = {
//	.init = NULL,
	.preparetowait = pon_charging_preparetowait,
	.heartbeat = pon_charging_heartbeat,
//	.update = pon_charging_update,
};

static struct tester_mode_ops *tester_mode_ops_tbl[] = {
	[TESTER_ITEM_PON_CHARGING]	= &pon_charging_ops,
	[TESTER_ITEM_PON_DISCHARGING]	= NULL,
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
	wakealarm_init();

	if (tester_mode_ops->init)
		return tester_mode_ops->init(&tester_status);
	else
		return 0;
}

int get_next_content(char *str, char *content)
{
	int i = 0, st = -1;

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

static int get_items_id(char *item)
{
	int i;

	for (i = 0; i < (int)(sizeof(items_tbl)/sizeof(items_tbl[0])); i++) {
		if (strcmp(items_tbl[i].name, item) == 0)
			return items_tbl[i].item_id;
	}
	return TESTER_ITEM_NULL;
}

static int get_steps_id(char *step)
{
	int i;

	for (i = 0; i < (int)(sizeof(steps_tbl)/sizeof(steps_tbl[0])); i++) {
		if (strcmp(steps_tbl[i].name, step) == 0)
			return steps_tbl[i].step_id;
	}
	return TESTER_STEP_NULL;
}

static char *get_items_name(int item_id)
{
	int i;

	for (i = 0; i < (int)(sizeof(items_tbl)/sizeof(items_tbl[0])); i++) {
		if (items_tbl[i].item_id == item_id)
			return items_tbl[i].name;
	}
	return NULL;
}

static char *get_steps_name(int step_id)
{
	int i;

	for (i = 0; i < (int)(sizeof(steps_tbl)/sizeof(steps_tbl[0])); i++) {
		if (steps_tbl[i].step_id == step_id)
			return steps_tbl[i].name;
	}
	return NULL;
}

static int tester_poweron_init(void)
{
	int fd, tmp, count = 0;
	char buf[1024];
	char item[32];
	char step[32];

	/* check and create status files if not exist */
	if (access(TESTER_LOG_FILE_DIR, R_OK | W_OK) == -1) {
		printf("TESTER: %s not exist!\n", TESTER_LOG_FILE_DIR);
		if (mkdir(TESTER_LOG_FILE_DIR, 0666) == -1) {
			printf("TESTER: can't creat %s:%s", TESTER_LOG_FILE_DIR, strerror(errno));
			return -1;
		}
		if (creat(TESTER_STAT_FILE_ENABLE, 0666) == -1) {
			printf("TESTER: can't creat %s:%s", TESTER_STAT_FILE_ENABLE, strerror(errno));
			return -1;
		}
		if (creat(TESTER_STAT_FILE_ITEMS, 0666) == -1) {
			printf("TESTER: can't creat %s:%s", TESTER_STAT_FILE_ITEMS, strerror(errno));
			return -1;
		}
		#if 0
		if (creat(TESTER_STAT_FILE_STEP, 0666) == -1) {
			printf("can't creat %s:%s", TESTER_STAT_FILE_STEP, strerror(errno));
			return -1;
		}
		#endif
		printf("Creat files success\n");
		return -1;
	}

	/* check enable */
	fd = open(TESTER_STAT_FILE_ENABLE, O_RDONLY);
	if (fd < 0) {
		printf("TESTER: can't open %s:%s\n", TESTER_STAT_FILE_ENABLE, strerror(errno));
		return -1;
	}
	count = read(fd, buf, 1);
	close(fd);
	if ((count < 1) || buf[0] != '1') {
		printf("TESTER: enable=%s\n", buf);
		return -1;
	} else {
		/* readout and verify items info */
		fd = open(TESTER_STAT_FILE_ITEMS, O_RDONLY);
		if (fd < 0) {
			printf("TESTER: can't open %s:%s\n", TESTER_STAT_FILE_ITEMS, strerror(errno));
			return -1;
		}
		count = read(fd, buf, sizeof(buf) - 1);
		close(fd);
		if ((count < 1)) {
			printf("%s may be empty:%s\n", TESTER_STAT_FILE_ITEMS, strerror(errno));
			return -1;
		}
		buf[count] = '\0';
		printf("TESTER: items:\n%s", buf);
		#if 0
		{
			int c = 0, p = 0;
			printf("item parse:\n");
			while((c = get_next_content(buf + p, item)) != -1) {
				p += c;
				printf("%s\n", item);
			}
		}
		#endif
		if (get_next_content(buf, item) == -1) {
			printf("TESTER: can't find first item\n");
			return -1;
		}
		printf("TESTER: first item:%s\n", item);
		tester_status.item_id = get_items_id(item);
		if (tester_status.item_id == TESTER_ITEM_NULL) {
			printf("TESTER: invalid item: %s\n", item);
			return -1;
		}
		tester_status.step_id = item_steps_tbl[tester_status.item_id].step_id[0];
		
		/* readout step info if it exist */
		fd = open(TESTER_STAT_FILE_STEP, O_RDONLY);
		if (fd < 0) {
			printf("TESTER: %s not exist:%s\n", TESTER_STAT_FILE_STEP, strerror(errno));
			return 0;
		}
		count = read(fd, step, sizeof(step) - 1);
		close(fd);
		if ((count < 1)) {
			printf("TESTER: %s may be empty:%s\n", TESTER_STAT_FILE_STEP, strerror(errno));
			return 0;
		}
		step[count] = '\0';
		printf("TESTER: step:%s\n", step);
		tmp = get_steps_id(step);
		if (tmp != TESTER_STEP_NULL)
			tester_status.step_id = tmp;
	}
	return 0;
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
		printf("TESTER: tester_poweron_init fail!\n");
		return ret;
	}
	
	printf("[tester start]: item=%s, step=%s\n", get_items_name(tester_status.item_id),
							get_steps_name(tester_status.step_id));

	tester_mode_ops = tester_mode_ops_tbl[tester_status.item_id];
	if (tester_mode_ops == NULL) {
		printf("TESTER: tester_mode_ops == NULL\n");
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

}

