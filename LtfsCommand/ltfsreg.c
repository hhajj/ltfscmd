/*
 *   File:   ltfsreg.c
 *   Author: Matthew Millman (inaxeon@hotmail.com)
 *
 *   Command line LTFS Configurator for Windows
 *
 *   This is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *   This software is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   You should have received a copy of the GNU General Public License
 *   along with this software.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pch.h"
#include "ltfsreg.h"

static BOOL LtfsRegGetInstallDir(LPSTR buffer, USHORT bufferLen);

BOOL LtfsRegCreateMapping(CHAR driveLetter, LPCSTR tapeDrive, BYTE tapeIndex, LPCSTR serialNumber, LPCSTR logDir, LPCSTR workDir, BOOL showOffline)
{
	HKEY key;
	DWORD disposition;
	char regKey[128];
	BOOL success = FALSE;
	size_t convertedChars = 0;

	_snprintf_s(regKey, _countof(regKey), _TRUNCATE, "Software\\Hewlett-Packard\\LTFS\\Mappings\\%c", driveLetter);

	// These registry values are read directly by FUSE4WinSvc.exe
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, regKey, 0, NULL, 0, KEY_READ | KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL, &key, &disposition) == ERROR_SUCCESS)
	{
		success = RegSetKeyValue(key, NULL, "SerialNumber", REG_SZ, serialNumber, (DWORD)(strlen(serialNumber) + 1)) == ERROR_SUCCESS;

		if (success)
		{
			success = RegSetKeyValue(key, NULL, "DeviceName", REG_SZ, tapeDrive, (DWORD)(strlen(tapeDrive) + 1)) == ERROR_SUCCESS;
		}

		if (success)
		{
			CHAR installDir[1024];

			success = LtfsRegGetInstallDir(installDir, _countof(installDir));

			if (success)
			{
				char commandLine[1024];
				_snprintf_s(commandLine, _countof(commandLine), _TRUNCATE,
					"%s%sltfs.exe T: -o devname=%s -d -o log_directory=%s -o work_directory=%s%s",
					installDir, installDir[strlen(installDir) - 1] == '\\' ? "" : "\\", tapeDrive, logDir, workDir, showOffline ? " -o show_offline" : "");
				success = RegSetKeyValue(key, NULL, "CommandLine", REG_SZ, commandLine, (DWORD)(strlen(commandLine) + 1)) == ERROR_SUCCESS;
			}
		}

		if (success)
		{
			char traceTarget[64];
			_snprintf_s(traceTarget, _countof(traceTarget), _TRUNCATE, "\\\\.\\pipe\\%c", driveLetter);
			success = RegSetKeyValue(key, NULL, "TraceTarget", REG_SZ, traceTarget, (DWORD)(strlen(traceTarget) + 1)) == ERROR_SUCCESS;
		}

		if (success)
		{
			DWORD traceType = 0x00000101;
			success = RegSetKeyValue(key, NULL, "TraceType", REG_DWORD, &traceType, sizeof(traceType)) == ERROR_SUCCESS;
		}

		RegCloseKey(key);
	}

	return TRUE;
}

BOOL LtfsRegRemoveMapping(CHAR driveLetter)
{
	char regKey[128];

	_snprintf_s(regKey, _countof(regKey), _TRUNCATE, "Software\\Hewlett-Packard\\LTFS\\Mappings\\%c", driveLetter);

	return RegDeleteKey(HKEY_LOCAL_MACHINE, regKey) == ERROR_SUCCESS;
}

BOOL LtfsRegGetMappingCount(BYTE *numMappings)
{
	HKEY key;
	BYTE count = 0;
	char driveLetter;
	char regKey[128];

	for (driveLetter = MIN_DRIVE_LETTER; driveLetter <= MAX_DRIVE_LETTER; driveLetter++)
	{
		_snprintf_s(regKey, _countof(regKey), _TRUNCATE, "Software\\Hewlett-Packard\\LTFS\\Mappings\\%c", driveLetter);

		LRESULT result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, regKey, 0, KEY_READ, &key);

		if (result == ERROR_SUCCESS)
		{
			count++;
			RegCloseKey(key);
		}
		else if (result != ERROR_FILE_NOT_FOUND)
		{
			return FALSE;
		}
	}

	*numMappings = count;
	return TRUE;
}

BOOL LtfsRegGetMappingProperties(CHAR driveLetter, LPSTR deviceName, USHORT deviceNameLength, LPSTR serialNumber, USHORT serialNumberLength)
{
	HKEY key;
	BYTE count = 0;
	BOOL result = FALSE;
	char regKey[128];

	_snprintf_s(regKey, _countof(regKey), _TRUNCATE, "Software\\Hewlett-Packard\\LTFS\\Mappings\\%c", driveLetter);

	if ((result = (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regKey, 0, KEY_READ, &key) == ERROR_SUCCESS)))
	{
		if (deviceName && deviceNameLength)
		{
			DWORD value = deviceNameLength;
			DWORD type = REG_SZ;
			result = RegQueryValueEx(key, "DeviceName", NULL, &type, deviceName, &value) == ERROR_SUCCESS;
		}

		if (serialNumber && serialNumberLength)
		{
			DWORD value = serialNumberLength;
			DWORD type = REG_SZ;
			result = RegQueryValueEx(key, "SerialNumber", NULL, &type, serialNumber, &value) == ERROR_SUCCESS;
		}

		RegCloseKey(key);
	}

	return result;
}

static BOOL LtfsRegGetInstallDir(LPSTR buffer, USHORT bufferLen)
{
	HKEY key;
	BYTE count = 0;
	BOOL result = FALSE;
	char regKey[128];

	strcpy_s(regKey, _countof(regKey), "Software\\Hewlett-Packard\\LTFS");

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regKey, 0, KEY_READ, &key) == ERROR_SUCCESS)
	{
		DWORD value = bufferLen;
		DWORD type = REG_SZ;
		result = RegQueryValueEx(key, "InstallDir", NULL, &type, buffer, &value) == ERROR_SUCCESS;

		RegCloseKey(key);
	}

	return result;
}
