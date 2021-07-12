#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include "offsets.h"
#include <windows.h>
#include "LocalPlayer.h"
#include <math.h>
#include <stdio.h>


typedef struct vector
{
	float x;
	float y;
	float z;
} VECTOR ;

DWORD getClosestPlayer(DWORD & LocalPlayer, DWORD & pEntityList, float* x)
{
	DWORD closestPlayer = NULL;
	double lowestDist = 9999;
	for (int i = 1; i < 64; i++)
	{
		DWORD entity = *(DWORD*)(pEntityList + i * 0x10);
		if (LocalPlayer == NULL) continue;
		if (entity == NULL) continue;
		if (*(bool*)(entity + offsets::m_bDormant)) continue;

		int entHealth = *(int*)(entity + offsets::m_iHealth);
		if (entHealth <= 0) continue;

		bool entImunity = *(int*)(entity + offsets::m_bGunGameImmunity);
		if (entImunity) continue;

		int myTeamNum = *(int*)(LocalPlayer + offsets::m_iTeamNum);
		int entTeamNum = *(int*)(entity + offsets::m_iTeamNum);

		if (myTeamNum == entTeamNum)
			continue;

		VECTOR localPlayerPos = *(VECTOR*)(LocalPlayer + offsets::m_vecOrigin);
		VECTOR entityPos = *(VECTOR*)(entity + offsets::m_vecOrigin);
		

		double distance = sqrt(pow(localPlayerPos.x - entityPos.x, 2) + pow(localPlayerPos.y - entityPos.y, 2) + pow(localPlayerPos.z - entityPos.z, 2));
		if (distance < lowestDist)
		{
			lowestDist = distance;
			closestPlayer = entity;
		}
	}
	*x = (float)lowestDist;
	return closestPlayer;
}

int WINAPI MainThread(HMODULE hMod)
{
	AllocConsole();
	freopen("CONOUT$", "w", stdout);

	DWORD client = (DWORD)GetModuleHandle("client.dll");
	DWORD pLocalPlayer = client + offsets::dwLocalPlayer;
	DWORD pEntityList = client + offsets::dwEntityList;
	DWORD pGlowObjectManager = client + offsets::dwGlowObjectManager;

	VECTOR localPlayerPos;
	bool run = true;

	HANDLE hComm;
	DCB dcb;
	BOOL fSuccess;

	hComm = CreateFileA("\\\\.\\COM4",                //port name
		GENERIC_READ | GENERIC_WRITE, //Read/Write
		0,                            // No Sharing
		NULL,                         // No Security
		OPEN_EXISTING,// Open existing port only
		0,            // Non Overlapped I/O
		NULL);

	if (hComm == INVALID_HANDLE_VALUE)
	{
		std::cout << "Error in opening serial port" << std::endl;
		CloseHandle(hComm);//Closing the Serial Port
		return 1;
	}

	char buffer[20];
	float x = 0;

	while (run)
	{
		DWORD LocalPlayer = *(DWORD*)pLocalPlayer;
		DWORD EntityList = *(DWORD*)pEntityList;
		DWORD GlowObjectManager = *(DWORD*)pGlowObjectManager;


		if (GetAsyncKeyState(VK_END) & 0x01)
			run = false;

		if (LocalPlayer && GlowObjectManager)
		{
			DWORD closestEntity = getClosestPlayer(LocalPlayer, pEntityList, &x);
			if (closestEntity == NULL)
			{
				std::cout << "Nothing close!" << std::endl;
				continue;
			}
			int glowIndex = *(int*)(closestEntity + offsets::m_iGlowIndex);
			std::cout << "Closest Entity: " << closestEntity << std::endl;
			std::cout << "Distance: " << x << std::endl;

			int dist = (int)x;
			sprintf(buffer, "%d", dist);
			DWORD bytes_written, total_bytes_written = 0;

			if (!WriteFile(hComm, buffer, 20, &bytes_written, NULL))
			{
				std::cout << "Failed to send bytes!" << std::endl;
				CloseHandle(hComm);
				return 2;
			}

			system("cls");
			/**(float*)(GlowObjectManager + (glowIndex * 0x38) + 0x4) = 1.0f;
			*(float*)(GlowObjectManager + (glowIndex * 0x38) + 0x8) = 0.0f;
			*(float*)(GlowObjectManager + (glowIndex * 0x38) + 0xC) = 0.0f;
			*(float*)(GlowObjectManager + (glowIndex * 0x38) + 0x10) = 1.0f;
			*(bool*)(GlowObjectManager + (glowIndex * 0x38) + 0x24) = true;
			*(bool*)(GlowObjectManager + (glowIndex * 0x38) + 0x25) = false;
			localPlayerPos = *(VECTOR*)(LocalPlayer + offsets::m_vecOrigin);
			system("cls");
			std::cout << "X: " << localPlayerPos.x << std::endl;
			std::cout << "Y: " << localPlayerPos.y << std::endl;
			std::cout << "Z: " << localPlayerPos.z << std::endl;*/
		}

	}

	CloseHandle(hComm);
	FreeConsole();
	FreeLibraryAndExitThread(hMod, NULL);
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}