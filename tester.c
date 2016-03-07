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
//	[TESTER_ITEM_PON_DISCHARGING] = {
//	},
};

static struct tester_mode_ops pon_charging_ops = {
	.init = pon_charging_init,
	.preparetowait = pon_charging_preparetowait,
	.heartbeat = pon_charging_heartbeat,
	.update = pon_charging_update,
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
	
	return tester_mode_ops->init(&tester_status);
}

int get_next_content(char *str, char *content)
{
	int i = 0, st = -1;

	/* find next content begining with displayable characters and end with '\n' or '\0' */
	do {
		if (st != -1) {
			if ((str[i] == '\n') || (str[i] == '\0')) {
				strncpy(content, str + st, i - st);
				content[i] = '\0';
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

static int tester_verify_item(char *item)
{
	int i;

	for (i = 0; i < (int)(sizeof(items_tbl)/sizeof(items_tbl[0])); i++) {
		if (strcmp(items_tbl[i].name, item) == 0)
			return items_tbl[i].item_id;
	}
	return TESTER_ITEM_NULL;
}

static int tester_verify_step(char *step)
{
	int i;

	for (i = 0; i < (int)(sizeof(steps_tbl)/sizeof(steps_tbl[0])); i++) {
		if (strcmp(steps_tbl[i].name, step) == 0)
			return steps_tbl[i].step_id;
	}
	return TESTER_STEP_NULL;
}

static void tester_poweron_init(void)
{
	int fd, pos, count = 0;
	char buf[1024];
	char item[32];
	char step[32];

	if (access(TEST_LOGS_DIR_PATH, R_OK | W_OK) == -1) {
		printf("%s don't exist:%s\n", TEST_LOGS_DIR_PATH, strerror(errno));
		if (mkdir(TEST_LOGS_DIR_PATH, 0666) == -1) {
			printf("can't creat %s:%s", TEST_LOGS_DIR_PATH, strerror(errno));
			exit(-1);
		}
		if (creat(TEST_STATE_ENABLE, 0666) == -1) {
			printf("can't creat %s:%s", TEST_STATE_ENABLE, strerror(errno));
			exit(-1);
		}
		if (creat(TEST_STATE_ITEM, 0666) == -1) {
			printf("can't creat %s:%s", TEST_STATE_ITEM, strerror(errno));
			exit(-1);
		}
		if (creat(TEST_STATE_STEP, 0666) == -1) {
			printf("can't creat %s:%s", TEST_STATE_STEP, strerror(errno));
			exit(-1);
		}
		printf("Creat files success\n");
		exit(-1);
	}
	
	fd = open(TEST_STATE_ENABLE, O_RDONLY);
	if (fd < 0) {
		printf("Could not open %s:%s\n", TEST_STATE_ENABLE, strerror(errno));
		exit(-1);
	}
	memset(buf, 0, sizeof(buf));
	count = read(fd, buf, 1);
	if ((count < 1) || buf[0] != '1') {
		printf("enable=%s\n", buf);
		exit(-1);
	}
	close(fd);

	if (buf[0] == '1') {
		/* read out and verify item info */
		fd = open(TEST_STATE_ITEM, O_RDONLY);
		if (fd < 0) {
			printf("Could not open %s:%s\n", TEST_STATE_ITEM, strerror(errno));
			exit(-1);
		}
		memset(buf, 0, sizeof(buf));
		count = read(fd, buf, sizeof(buf));
		close(fd);
		if ((count < 1)) {
			printf("%s is empty:%s\n", TEST_STATE_ITEM, strerror(errno));
			exit(-1);
		}
		printf("items:\n%s", buf);
		if (get_next_content(buf, item) < 0) {
			printf("Can't find first item\n");
			exit(-1);
		}
		printf("item:%s\n", item);
		tester_status.item_id = tester_verify_item(item);
		if (tester_status.item_id < 0) {
			printf("invalid item name:%s\n", item);
			exit(-1);
		}
		tester_status.step_id = item_steps_tbl[tester_status.item_id].step_id[0];
		
		/* read out step info if it exist */
		fd = open(TEST_STATE_STEP, O_RDONLY);
		if (fd < 0) {
			printf("Could not open %s:%s\n", TEST_STATE_STEP, strerror(errno));
			exit(-1);
		}
		memset(step, 0, sizeof(step));
		count = read(fd, step, sizeof(step) - 1);
		close(fd);
		if ((count < 1)) {
			printf("%s is empty:%s\n", TEST_STATE_STEP, strerror(errno));
			return;
		}
		step[count] = '\0';
		tester_status.step_id = tester_verify_step(step);
		if (tester_status.step_id < 0) {
			printf("invalid step name:%s\n", step);
			return;
		}
	}
}

static void tester_mainloop(void)
{
	int i;

	while (1) {
		struct epoll_event events[eventct];
		int nevents;
		int timeout = -1;

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

		if (!nevents)
			tester_mode_ops->update(&tester_status);

		tester_mode_ops->heartbeat(&tester_status);
	}
	return;
}

int main(void)
{
	int ret;

	tester_poweron_init();
	printf("tester start: item=%s, step=%s\n", items_tbl[tester_status.item_id].name,
						steps_tbl[tester_status.step_id].name);
	switch (tester_status.item_id) {
	case TESTER_ITEM_PON_CHARGING:
		tester_mode_ops = &pon_charging_ops;
		break;
	default:
		printf("error: tester_mode_ops is NULL\n");
		break;
	};
	
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

