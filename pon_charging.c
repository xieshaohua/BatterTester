#include "tester.h"

static void heartbeat(struct battst_status *status);

static const char *step_msg[] = {
	"<0> discharging battery capacity to 1%",
	"<1> charging until charge current under 50mA",
};

static const struct step_desc step_desc = {
	.step_id_limit = sizeof(step_msg)/sizeof(step_msg[0]),
	.step_msg = step_msg,
};

static struct tester_mode_ops ops = {
	.heartbeat = heartbeat,
};

const struct item_desc pon_charging_item_desc = {
	.name = "pon_charging",
	.step_desc = &step_desc,
	.ops = &ops,
};

static void heartbeat(struct battst_status *status)
{
	static int count = 0;
	char timestamp[TIME_TIMESTAMP_SIZE];

	if (status->step_id == ITEM_STEP_1) {
		status->log_enable = 0;
		status->alarm_interval = 2;
		if (status->batt_props.capacity > 1) {
			if (status->batt_props.charging_enabled)
				charging_disable();
			discharge_start(DISCHARGE_METHOD_VIBRATE);
			if (++count == 10) {
				android_reboot(ANDROID_RB_POWEROFF, 0, 0);
			}
		} else {
			tester_finish();
			//android_reboot(ANDROID_RB_RESTART, 0, 0);
		}
	} else if (status->step_id == ITEM_STEP_2) {
		status->log_enable = 1;
		status->log_path = LOG_PON_CHARGING;
		status->alarm_interval = 10;
		if (status->batt_props.capacity == 100) {
			if (status->batt_props.current_now > -50) {
				if (++count > 10)
					tester_finish();
			} else {
				count = 0;
			}
		}
	}

	DEBUG("%s %s: %s\n", get_timestamp(timestamp), status->item_desc->name,
				status->item_desc->step_desc->step_msg[status->step_id]);
}
