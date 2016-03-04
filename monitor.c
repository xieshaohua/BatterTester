#include "tester.h"

static char logbuf[MONITOR_LOGBUF_SIZE];

static size_t monitor_readfile(const char *path, char *buf, size_t size)
{
	char *cp = NULL;
	int fd;
	ssize_t count = 0;

	if (path == NULL)
		return -1;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		TESTER_DEBUG("Could not open\n");
		return -1;
	}

	count = TEMP_FAILURE_RETRY(read(fd, buf, size));
	if (count > 0)
		cp = (char *)memrchr(buf, '\n', count);

	if (cp)
		*(cp + 1) = '\0';
	else
		buf[0] = '\0';

	close(fd);
	return count;
}

static size_t monitor_writefile(const char *path, char *buf, size_t size)
{
	int fd;
	size_t count = 0;
	
	if (path == NULL)
		return -1;

	fd = open(path, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
	if (fd < 0) {
		printf("Could not open:%s\n", strerror(fd));
		return -1;
	}

	count = TEMP_FAILURE_RETRY(write(fd, buf, size));
	if (count != size) {
		TESTER_DEBUG("write fail\n");
	}

	close(fd);
	return count;
}

static char *set_timestamp(char *buf)
{
	time_t now;
	struct tm *tt;

	time(&now);
	tt = localtime(&now);
	snprintf(buf, TIME_TIMESTAMP_LEN + 1, "%04d-%02d-%02d %02d:%02d:%02d\n",
				tt->tm_year + 1900, tt->tm_mon, tt->tm_mday, tt->tm_hour, tt->tm_min, tt->tm_sec);
	return buf;
}

void monitor_init(void)
{
	
}

void monitor_updatelog(void)
{
	char *pos = logbuf;
	
	memset(pos, 0, MONITOR_LOGBUF_SIZE);
	set_timestamp(pos);
	monitor_readfile(POWER_SUPPLY_SYSFS_PATH, pos + TIME_TIMESTAMP_LEN, MONITOR_LOGBUF_SIZE - TIME_TIMESTAMP_LEN);
	//monitor_writefile(POWER_SUPPLY_LOGFILE_PATH, pos, MONITOR_LOGBUF_SIZE);
	printf("%s", pos);
}

