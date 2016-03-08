#include "tester.h"

#if 0
int pon_charging_init(struct tester_status *status)
{
	printf("pon_charging_init\n");
	return 0;
}
#endif

int pon_charging_preparetowait(struct tester_status *status)
{
	if (status->step_id == TESTER_STEP_DISCHARGING)
		return 1000;
	else
		return 5000;
}

void pon_charging_heartbeat(struct tester_status *status)
{
	if (status->step_id == TESTER_STEP_DISCHARGING) {
		/* discharging to %1 and reboot */
		if (status->batt_props.capacity > 1) {
			if (status->batt_props.charging_enabled)
				charging_disable();
			discharge_start(DISCHARGE_METHOD_VIBRATE |
					DISCHARGE_METHOD_FLASH |
					DISCHARGE_METHOD_CPU);
		} else {
			tester_finish();
			android_reboot(ANDROID_RB_RESTART, 0, 0);
		}
	}
}
#if 0
void pon_charging_update(struct tester_status *status)
{

}
#endif
