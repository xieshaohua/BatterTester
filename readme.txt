logs 文件路径：

/data/batt_logs/*

/data/batt_logs/enable
/data/batt_logs/steps
/data/batt_logs/cur_step
/data/batt_logs/




items:
<poweron_charging>
	1.使能battery测试（enable == 1），重启开机
	2.判断当前步骤是poweron_charging，如果电池电量>%1，断开电池充电，开始放电，
          当电池电量<=1%时开始充电，打log
	3.当判断到充电电流为<100mA时，测试终止，进入下一步测试

<poweron_discharging>

<poweroff_charging>

<poweroff_dischargind>


