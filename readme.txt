logs �ļ�·����

/data/batt_logs/*

/data/batt_logs/enable
/data/batt_logs/steps
/data/batt_logs/cur_step
/data/batt_logs/




items:
<poweron_charging>
	1.ʹ��battery���ԣ�enable == 1������������
	2.�жϵ�ǰ������poweron_charging�������ص���>%1���Ͽ���س�磬��ʼ�ŵ磬
          ����ص���<=1%ʱ��ʼ��磬��log
	3.���жϵ�������Ϊ<100mAʱ��������ֹ��������һ������

<poweron_discharging>

<poweroff_charging>

<poweroff_dischargind>


struct tester_mode_ops {
	void (*init)(struct tester_status *status);		// ������ʼ��ִ��
	int (*preparetowait)(struct tester_status *status);	// mainloop timeout, ������ϵͳ����״̬����������ִ�е�ʱ��
	void (*heartbeat)(struct tester_status *status);	// ���ں��¼�����ִ��
	void (*update)(struct tester_status *status);		// ����ִ��
};


