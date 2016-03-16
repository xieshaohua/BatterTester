#include "tester.h"

static const char *pon_charging_msg[] = {
	"<0> discharging battery capacity to 1%",
	"<1> charging until charge current under 50mA",
};

const struct items_desc pon_charging_desc = {
	.step_id_limit = sizeof(pon_charging_msg)/sizeof(pon_charging_msg[0]),
	.step_msg = pon_charging_msg,
};

#if 0
int pon_charging_init(struct tester_status *status)
{
	if (status->step_id == 2) {
		status->log_enable = 0;
	}
	return 0;
}
#endif
void pon_charging_heartbeat(struct tester_status *status)
{
	static int count = 0;
	char timestamp[TIME_TIMESTAMP_SIZE];

	if (status->step_id == 0) {
		status->log_enable = 0;
		status->alarm_interval = 2;
		if (status->batt_props.capacity > 1) {
			if (status->batt_props.charging_enabled)
				charging_disable();
			discharge_start(DISCHARGE_METHOD_VIBRATE);
		} else {
			tester_finish();
			//android_reboot(ANDROID_RB_RESTART, 0, 0);
		}
	} else if (status->step_id == 1) {
		status->log_enable = 1;
		status->log_path = TESTER_PON_CHARGING_LOG;
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
	
	DEBUG("%s %s: %s\n", get_timestamp(timestamp), get_items_name(status->item_id),
				tester_status.items_desc->step_msg[tester_status.step_id]);
}
