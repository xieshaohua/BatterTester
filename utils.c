#include "tester.h"

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

int read_file(const char *path, char *buf, int size)
{
	int fd;
	int count = 0;

	if ((path == NULL) || (buf == NULL))
		return -1;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		DEBUG("UTILS: can't open %s:%s\n", path, strerror(errno));
		return -1;
	}

	count = TEMP_FAILURE_RETRY(read(fd, buf, size));
	if (count > 0)
		buf[count] = '\0';

	close(fd);
	return count;

}

int write_file(const char *path, const char *buf, int size)
{
	int fd;
	int count = 0;
	
	if ((path == NULL) || (buf == NULL))
		return -1;

	fd = open(path, O_RDWR, 0666);
	if (fd < 0) {
		DEBUG("UTILS: can't open %s:%s\n", path, strerror(errno));
		return -1;
	}

	count = TEMP_FAILURE_RETRY(write(fd, buf, size));
	if (count != size) {
		DEBUG("ERROR: need %d but %d written\n", size, count);
	}

	close(fd);
	return count;
}

int get_next_content(const char *str, char *content)
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

int first_ready_item(const char *str, char *item, char *step)
{
	int i, len, cnt, pos = 0;
	char buf[CONTENT_BUF_SIZE + 1];

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


