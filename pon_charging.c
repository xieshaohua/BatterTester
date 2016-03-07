#include "tester.h"

int pon_charging_init(struct tester_status *status)
{
	printf("pon_charging_init\n");
	return 0;
}

int pon_charging_preparetowait(struct tester_status *status)
{
	if (status->step_id == TESTER_STEP_DISCHARGING)
		return 1;
	else
		return -1;
}

void pon_charging_heartbeat(struct tester_status *status)
{
	if (status->step_id == TESTER_STEP_DISCHARGING) {
		if (status->batt_props.capacity <= 1) {
			status->step_id == TESTER_STEP_CHARGING;
			status->logfile = TEST_PON_CHARGING_LOG;
			status->log_enable = 1;
		} else {
			//discharging
		}
	}
}

void pon_charging_update(struct tester_status *status)
{
}

