// DeleteSvc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <windows.h>


int main(int argc, char* argv[])
{
	SC_HANDLE   schSCManager;
	SC_HANDLE   schService;
	SERVICE_STATUS  serviceStatus;

//	//Debug("[KMS]%s:%d Enter...", __FUNCTION__, __LINE__);

	if (argc < 2)
	{

		printf("[�����]\n");
		printf("RemoveDriver.EXE [SERVICENAME]\n");
		printf("[SERVICENAME] : ������ �����̸�\n");
		return 0;
	}
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!schSCManager)
	{
		//Debug("[KMS]%s:%d No Service Manager exist...", __FUNCTION__, __LINE__);
		printf("���񽺸޴����� �����ϴ� error hex = %x\n", GetLastError());
		return 0;
	}

	//Debug("[KMS]%s:%d unregister service [%s]", __FUNCTION__, __LINE__, argv[1]);
	schService = OpenService(schSCManager, argv[1], SERVICE_ALL_ACCESS);
	if (schService == NULL)
	{
		//Debug("[KMS]%s:%d No prior service registered...[%s]", __FUNCTION__, __LINE__, argv[1]);
		printf("�ش��ϴ� ���񽺰� ��ϵǾ� ���� �ʽ��ϴ� error hex = %x\n", GetLastError());
	}
	else
	{
		ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);
		if (serviceStatus.dwCurrentState != 1) {
			//Debug("[KMS]%s:%d SERVICE_CONTROL_STOP but can't stop service...[%s]", __FUNCTION__, __LINE__);
			printf("������ ������ �� �� ���� ������ �ش��ϴ� ��û�� ������ �� �����ϴ� error hex = %x\n", GetLastError());
		}
		else
		{
			DeleteService(schService);
			//Debug("[KMS]%s:%d Delete service...", __FUNCTION__, __LINE__);
			printf("���񽺸� �����Ͽ����ϴ�. ���񽺸� �����Ͽ����ϴ�\n");
		}
		CloseServiceHandle(schService);
		//Debug("[KMS]%s:%d Service handle closed...", __FUNCTION__, __LINE__);
	}
	CloseServiceHandle(schSCManager);

	return 0;
}

