/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) 2012 Seth <seth@seth-notebook>
 *
 * ListUsbDevice is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ListUsbDevice is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
// #undef   _FILE_OFFSET_BITS
// #define   _FILE_OFFSET_BITS   64

#include "DefineHeader.h"
#include "RKLog.h"
#include "RKDevice.h"
#include "RKUpgrade.h"
#include "RKImage.h"
#ifdef _WIN32
#include "os\win\RKScan.h"
#include "os\win\RKComm.h"
#include "os\win\RKDisk.h"
#elif __APPLE__
#include "os/mac/RKScan.h"
#include "os/mac/RKComm.h"
#include "os/mac/RKDisk.h"
#else
#include "os/linux/RKScan.h"
#include "os/linux/RKComm.h"
#include "os/linux/RKDisk.h"
#endif
extern const char *szManufName[];
CRKLog *g_pLogObject = NULL;

PARAM_ITEM_VECTOR g_ParamItemVec;
CONFIG_ITEM_VECTOR g_ConfigItemVec;
uint_16 g_usDevNewVid = 0, g_usDevNewPid = 0, g_usDevOldVid = 0, g_usDevOldPid = 0, g_usMscVid = 0, g_usMscPid = 0;
bool g_bDirectLBA = false, g_bFirst4Access = false, g_bGpt = false, g_bReadCheckOff = true, g_bReadLBA = false, g_bEnv = false,
	 g_bReadComLog = false, g_bLogOff = false, g_bSwitchStorage = false, g_bNorSingleIDB = false, g_bCBW_parity = false,g_bMcu = false,
	 g_bReadOtpChip = false, g_bSwitchUsb3 = false, g_bUsb3Transfer=false, g_bCloseCheckIDB=false,g_bUsbDriver4m=false;
uint_32 g_uiForceBand = 0;
#define DEFAULT_RW_SECTOR 32
#define DEFAULT_RW_LBA 128

#define APP_VERSION "v2.44"
void usage()
{
	printf("\r\n---------------------Tool Usage ---------------------\r\n");
	printf("Help:             H\r\n");
	printf("Version:          V\r\n");
	printf("Log:              LG\r\n");
	printf("------------------Upgrade Command ------------------\r\n");
	printf("ChooseDevice:		CD\r\n");
	printf("ListDevice:		LD\r\n");
	printf("SwitchDevice:		SD\r\n");
	printf("UpgradeFirmware:	UF <Firmware> [-noreset]\r\n");
	printf("UpgradeLoader:		UL <Loader> [-noreset] [FLASH|EMMC|SPINOR|SPINAND]\r\n");
	printf("DownloadImage:		DI <-p|-b|-k|-s|-r|-m|-u|-t|-re image>\r\n");
	printf("DownloadBoot:		DB <Loader>\r\n");
	printf("EraseFlash:		EF <Loader|firmware>\r\n");
	printf("PartitionList:		PL\r\n");
	printf("WriteSN:		SN <serial number>\r\n");
	printf("ReadSN:			RSN\r\n");
	printf("ReadComLog:		RCL <File>\r\n");
	printf("CreateGPT:		GPT <Input Parameter> <Output Gpt>\r\n");
	printf("SwitchStorage:		SSD \r\n");
	printf("SwitchUSB3:		SU3 \r\n");
	printf("ShowFirmwareInfo:	SFI <Firmware> [entryNo]\r\n");
	printf("ExtractFirmware:	EXF <Firmware|Loader> <Output Dir>\r\n");
	printf("----------------Professional Command -----------------\r\n");
	printf("TestDevice:		TD\r\n");
	printf("ResetDevice:		RD [subcode]\r\n");
	printf("ResetPipe:		RP [pipe]\r\n");
	printf("ReadCapability:		RCB\r\n");
	printf("ReadFlashID:		RID\r\n");
	printf("ReadFlashInfo:		RFI\r\n");
	printf("ReadChipInfo:		RCI\r\n");
	printf("ReadCPUID:		CPU [-out|-textout]\r\n");
	printf("ReadSecureMode:		RSM\r\n");
	// printf("ReadSector:		RS  <BeginSec> <SectorLen> [-decode] [File]\r\n");
	//printf("WriteSector:		WS  <BeginSec> <PageSizeK> <PageSpareB> <File>\r\n");
	printf("ReadLBA:		RL  <BeginSec> <SectorLen> [File]\r\n");
	printf("WriteLBA:		WL  <BeginSec> [SizeSec] <File>\r\n");
	printf("EraseLBA:		EL  <BeginSec> <EraseCount> \r\n");
	printf("EraseBlock:		EB <CS> <BeginBlock> <BlokcLen> [--Force]\r\n");
	printf("RunSystem:		RUN <uboot_addr> <trust_addr> <boot_addr> <uboot> <trust> <boot>\r\n");
	printf("-------------------------------------------------------\r\n\r\n");
}

void ProgressInfoProc(uint_32 deviceLayer, ENUM_PROGRESS_PROMPT promptID, long long totalValue, long long currentValue, ENUM_CALL_STEP emCall)
{
	string strInfoText = "";
	char szText[256];
	int nPercent;

	nPercent = ((double)currentValue / totalValue) * 100;
	switch (promptID)
	{
	case TESTDEVICE_PROGRESS:
		sprintf(szText, "Test Device... (%d%%)", nPercent);
		strInfoText = szText;
		break;
	case LOWERFORMAT_PROGRESS:
		sprintf(szText, "Lowerformat Device... (%d%%)", nPercent);
		strInfoText = szText;
		break;
	case DOWNLOADIMAGE_PROGRESS:
		sprintf(szText, "Download Image... (%d%%)", nPercent);
		strInfoText = szText;
		break;
	case CHECKIMAGE_PROGRESS:
		sprintf(szText, "Check Image... (%d%%)", nPercent);
		strInfoText = szText;
		break;
	case TAGBADBLOCK_PROGRESS:
		sprintf(szText, "Tag Bad Block... (%d%%)", nPercent);
		strInfoText = szText;
		break;
	case TESTBLOCK_PROGRESS:
		sprintf(szText, "Test Block... (%d%%)", nPercent);
		strInfoText = szText;
		break;
	case ERASEFLASH_PROGRESS:
		sprintf(szText, "Erase Flash... (%d%%)", nPercent);
		strInfoText = szText;
		break;
	case ERASESYSTEM_PROGRESS:
		sprintf(szText, "Erase System partition... (%d%%)", nPercent);
		strInfoText = szText;
		break;
	case ERASEUSERDATA_PROGRESS:
		sprintf(szText, " Erase Userdata partition... (%d%%)", nPercent);
		strInfoText = szText;
		break;
	}
	if (strInfoText.size() > 0)
	{
		if (emCall != CALL_FIRST)
		{
			CURSOR_MOVEUP_LINE(1);
			CURSOR_DEL_LINE;
		}
		printf("%s\r\n", strInfoText.c_str());
	}
}
void PromptInfoProc(uint_32 deviceLayer, ENUM_UPGRADE_PROMPT promptID, uint_32 oldDeviceLayer)
{
	string strInfoText;
	bool bError = false;
	strInfoText = "";
	switch (promptID)
	{
	case DOWNLOADBOOT_START:
		strInfoText = "Download Boot Start";
		break;
	case DOWNLOADBOOT_FAIL:
		strInfoText = "Download Boot Fail";
		bError = true;
		break;
	case DOWNLOADBOOT_PASS:
		strInfoText = "Download Boot Success";
		break;
	case DOWNLOADIDBLOCK_START:
		strInfoText = "Download IDB Start";
		break;
	case DOWNLOADIDBLOCK_FAIL:
		strInfoText = "Download IDB Fail";
		bError = true;
		break;
	case DOWNLOADIDBLOCK_PASS:
		strInfoText = "Download IDB Success";
		break;
	case DOWNLOADIMAGE_START:
		strInfoText = "Download Firmware Start";
		break;
	case DOWNLOADIMAGE_FAIL:
		strInfoText = "Download Firmware Fail";
		bError = true;
		break;
	case DOWNLOADIMAGE_PASS:
		strInfoText = "Download Firmware Success";
		break;
	case TESTDEVICE_START:
		strInfoText = "Test Device Start";
		break;
	case TESTDEVICE_FAIL:
		strInfoText = "Test Device Fail";
		bError = true;
		break;
	case TESTDEVICE_PASS:
		strInfoText = "Test Device Success";
		break;
	case LOWERFORMAT_START:
		strInfoText = " Lowerformat Device Start";
		break;
	case LOWERFORMAT_FAIL:
		strInfoText = "Lowerformat Device Fail";
		bError = true;
		break;
	case LOWERFORMAT_PASS:
		strInfoText = "Lowerformat Device Success";
		break;
	case RESETDEVICE_START:
		strInfoText = "Reset Device Start";
		break;
	case RESETDEVICE_FAIL:
		strInfoText = "Reset Device Fail";
		bError = true;
		break;
	case RESETDEVICE_PASS:
		strInfoText = "Reset Device Success";
		break;

	case WAITMSC_START:
		strInfoText = "Wait For MSC Start";
		break;
	case WAITMSC_FAIL:
		strInfoText = "Wait For MSC Fail";
		bError = true;
		break;
	case WAITMSC_PASS:
		strInfoText = "Wait For MSC Success";
		break;
	case WAITLOADER_START:
		strInfoText = "Wait For Loader Start";
		break;
	case WAITLOADER_FAIL:
		strInfoText = "Wait For Loader Fail";
		bError = true;
		break;
	case WAITLOADER_PASS:
		strInfoText = "Wait For Loader Success";
		break;
	case WAITMASKROM_START:
		strInfoText = "Wait For Maskrom Start";
		break;
	case WAITMASKROM_FAIL:
		strInfoText = "Wait For Maskrom Fail";
		bError = true;
		break;
	case WAITMASKROM_PASS:
		strInfoText = "Wait For Maskrom Success";
		break;
	case ERASEIDB_START:
		strInfoText = "Erase IDB Start";
		break;
	case ERASEIDB_FAIL:
		strInfoText = "Erase IDB Fail";
		bError = true;
		break;
	case ERASEIDB_PASS:
		strInfoText = "Erase IDB Success";
		break;
	case SWITCHMSC_START:
		strInfoText = "Switch MSC Start";
		break;
	case SWITCHMSC_FAIL:
		strInfoText = "Switch MSC Fail";
		bError = true;
		break;
	case SWITCHMSC_PASS:
		strInfoText = "Switch MSC Success";
		break;
	case CHECKCHIP_START:
		strInfoText = "Check Chip Start";
		break;
	case CHECKCHIP_FAIL:
		strInfoText = "Check Chip Fail";
		bError = true;
		break;
	case CHECKCHIP_PASS:
		strInfoText = "Check Chip Success";
		break;
	case PREPAREIDB_START:
		strInfoText = "Prepare IDB Start";
		break;
	case PREPAREIDB_FAIL:
		strInfoText = "Prepare IDB Fail";
		bError = true;
		break;
	case PREPAREIDB_PASS:
		strInfoText = "Prepare IDB Success";
		break;
	case GETFLASHINFO_START:
		strInfoText = "Get FlashInfo Start";
		break;
	case GETFLASHINFO_FAIL:
		strInfoText = "Get FlashInfo Fail";
		bError = true;
		break;
	case GETFLASHINFO_PASS:
		strInfoText = "Get FlashInfo Success";
		break;
	case ERASEALLBLOCKS_START:
		strInfoText = "Erase Flash Start";
		break;
	case ERASEALLBLOCKS_FAIL:
		strInfoText = "Erase Flash Fail";
		bError = true;
		break;
	case ERASEALLBLOCKS_PASS:
		strInfoText = "Erase Flash Success";
		break;
	case GETBLOCKSTATE_START:
		strInfoText = "Get Block State Start";
		break;
	case GETBLOCKSTATE_FAIL:
		strInfoText = "Get Block State Fail";
		bError = true;
		break;
	case GETBLOCKSTATE_PASS:
		strInfoText = "Get Block State Success";
		break;
	case SWITCHUSB3_START:
		strInfoText = "Switch Usb3 Start";
		break;
	case SWITCHUSB3_FAIL:
		strInfoText = "Switch Usb3 Fail";
		bError = true;
		break;
	case SWITCHUSB3_PASS:
		strInfoText = "Switch Usb3 Success";
		break;
	default:
		break;
	}
	if (strInfoText.size() > 0)
	{
		// CURSOR_MOVEUP_LINE(1);
		// CURSOR_DEL_LINE;
		if (bError)
		{
			ERROR_COLOR_ATTR;
			printf("%s", strInfoText.c_str());
			NORMAL_COLOR_ATTR;
			printf("\r\n");
		}
		else
			printf("%s\r\n", strInfoText.c_str());
		if (g_pLogObject)
		{
			g_pLogObject->Record(strInfoText.c_str());
		}
	}
}
string GetErrString(uint_32 err)
{
	string strErr = "";
	if (err == ERR_OPEN_IMAGE)
	{
		strErr = "Note:please check firmware if exist and occupied";
	}
	else if (err == ERR_WRONG_TAG)
	{
		strErr = "Note:invalid tag of firmware,please check firmware";
	}
	else if (err == ERR_WRONG_HASH)
	{
		strErr = "Note:wrong hash of firmware,please check firmware";
	}
	else if (err == ERR_BOOT_TAG)
	{
		strErr = "Note:invalid tag of loader,please check loader";
	}
	else if (err == ERR_BOOT_CRC)
	{
		strErr = "Note:wrong hash of loader,please check loader";
	}
	else if (err == ERR_COMM_RW)
	{
		strErr = "Note:Communication issues,Certainly please check usb,Completely please check device";
	}
	else if (err == ERR_WRONG_CHIP)
	{
		strErr = "Note:read chip info by rkdevtool and use new chip to create firmware";
	}
	else if (err == ERR_DEVICE_CREATE)
	{
		strErr = "Note:device did not respond and was disconnected,please reset device and retry";
	}
	else if (err == ERR_RUN_BOOT471)
	{
		strErr = "Note:please check ddr,please reset device and retry";
	}
	else if (err == ERR_RUN_BOOT472)
	{
		strErr = "Note:please check usb,please reset device and retry";
	}
	else if (err == ERR_NO_FLASH_CS)
	{
		strErr = "Note:please check flash if in the support list or welding issue";
	}
	else if (err == ERR_FW_READ)
	{
		strErr = "Note:please update the tool for creating firmware,and regenerate firmware";
	}
	else if (err == ERR_CROSS_IDBLOCK)
	{
		strErr = "Note:Position of writing image is front of idblock";
	}
	else if (err == ERR_PARTITION_NOT_ENOUGH)
	{
		strErr = "Note:Image is larger than partition size in the firmware";
	}
	else if (err == ERR_PARTITION_CROSS_FLASH)
	{
		strErr = "Note:Definition of partition in the firmware is larger than flash";
	}
	return strErr;
}
size_t get_file_size(char *szFile)
{
	FILE *file = NULL;
	file = fopen(szFile, "rb");
	if (!file)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed,err=%d,can't open file: %s\r\n", __func__, errno, szFile);
		return (uint_32)-1;
	}
	size_t iFileSize;
	fseek(file, 0, SEEK_END);
	iFileSize = ftell(file);
	fclose(file);
	return iFileSize;
}
bool GetReadCheckFlag(bool &value)
{
	int i;
	for (i = 0; i < g_ConfigItemVec.size(); i++)
	{
		if (stricmp("rb_check_off", g_ConfigItemVec[i].szItemName) == 0)
		{
			if (stricmp("true", g_ConfigItemVec[i].szItemValue) == 0)
				value = true;
			else
				value = false;
			return true;
		}
	}
	return false;
}
bool GetIdbCheckFlag(bool &value)
{
	int i;
	for (i = 0; i < g_ConfigItemVec.size(); i++)
	{
		if (stricmp("close_check_idb", g_ConfigItemVec[i].szItemName) == 0)
		{
			if (stricmp("true", g_ConfigItemVec[i].szItemValue) == 0)
				value = true;
			else
				value = false;
			return true;
		}
	}
	return false;
}
bool GetUsb3TransferFlag(bool &value)
{
	int i;
	for (i = 0; i < g_ConfigItemVec.size(); i++)
	{
		if (stricmp("usb3_transfer_on", g_ConfigItemVec[i].szItemName) == 0)
		{
			if (stricmp("true", g_ConfigItemVec[i].szItemValue) == 0)
				value = true;
			else
				value = false;
			return true;
		}
	}
	return false;
}

bool GetNorSingleIDBFlag(bool &value)
{
	int i;
	for (i = 0; i < g_ConfigItemVec.size(); i++)
	{
		if (stricmp("nor_single_idb", g_ConfigItemVec[i].szItemName) == 0)
		{
			if (stricmp("true", g_ConfigItemVec[i].szItemValue) == 0)
				value = true;
			else
				value = false;
			return true;
		}
	}
	return false;
}
bool GetForceDataBand(uint_32 &value)
{
	int i;
	for (i = 0; i < g_ConfigItemVec.size(); i++)
	{
		if (stricmp("force_data_band", g_ConfigItemVec[i].szItemName) == 0)
		{
			char *pszEnd;
			value = strtoul(g_ConfigItemVec[i].szItemValue, &pszEnd, 0);
			if (*pszEnd)
				return false;
			else
			{
				if (value > 10)
					value = 6;
				return true;
			}
		}
	}
	return false;
}

bool GetLogFlag(bool &value)
{
	int i;
	for (i = 0; i < g_ConfigItemVec.size(); i++)
	{
		if (stricmp("log_off", g_ConfigItemVec[i].szItemName) == 0)
		{
			if (stricmp("true", g_ConfigItemVec[i].szItemValue) == 0)
				value = true;
			else
				value = false;
			return true;
		}
	}
	return false;
}

bool GetLogDir(string &strDir)
{
	int i;
	for (i = 0; i < g_ConfigItemVec.size(); i++)
	{
		if (stricmp("log_dir", g_ConfigItemVec[i].szItemName) == 0)
		{
			strDir = g_ConfigItemVec[i].szItemValue;
			return true;
		}
	}
	return false;
}

bool GetParityFlag(bool &value)
{
	int i;
	for (i = 0; i < g_ConfigItemVec.size(); i++)
	{
		if (stricmp("lba_parity", g_ConfigItemVec[i].szItemName) == 0)
		{
			if (stricmp("true", g_ConfigItemVec[i].szItemValue) == 0)
				value = true;
			else
				value = false;
			return true;
		}
	}
	return false;
}
void GetConsoleBufferFlag()
{
	int i, ret;
	for (i = 0; i < g_ConfigItemVec.size(); i++)
	{
		if (stricmp("stdout_buffer_off", g_ConfigItemVec[i].szItemName) == 0)
		{
			if (stricmp("true", g_ConfigItemVec[i].szItemValue) == 0)
				ret = setvbuf(stdout, NULL, _IONBF, 0);
			break;
		}
	}
}
void GetDeviceIDFromConfig()
{
	int i, ret;
	uint_16 usDevNewVid, usDevNewPid, usDevOldVid, usDevOldPid, usMscVid, usMscPid;
	for (i = 0; i < g_ConfigItemVec.size(); i++)
	{
		if (stricmp("rockusb_id", g_ConfigItemVec[i].szItemName) == 0)
		{
			ret = sscanf(g_ConfigItemVec[i].szItemValue, "%hx#%hx#%hx#%hx", &usDevNewVid, &usDevNewPid, &usDevOldVid, &usDevOldPid);
			if (ret == 4)
			{
				g_usDevNewVid = usDevNewVid;
				g_usDevNewPid = usDevNewPid;
				g_usDevOldVid = usDevOldVid;
				g_usDevOldPid = usDevOldPid;
				g_ConfigItemVec.erase(g_ConfigItemVec.begin() + i);
				i--;
				continue;
			}
		}
		if (stricmp("msc_id", g_ConfigItemVec[i].szItemName) == 0)
		{
			ret = sscanf(g_ConfigItemVec[i].szItemValue, "%hx#%hx", &usMscVid, &usMscPid);
			if (ret == 2)
			{
				g_usMscVid = usMscVid;
				g_usMscPid = usMscPid;
				g_ConfigItemVec.erase(g_ConfigItemVec.begin() + i);
				i--;
				continue;
			}
		}
	}
}

bool is_update_loader(char *szFile)
{
	FILE *file = NULL;
	STRUCT_RKBOOT_HEAD head;
	uint_32 uiRead;
	file = fopen(szFile, "rb");
	if (!file)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, can't open file: %s\r\n", __func__, errno, szFile);
		return false;
	}
	uiRead = fread(&head, 1, sizeof(head), file);
	if (uiRead != sizeof(head))
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, read=%d, total=%d\r\n", __func__, errno, uiRead, sizeof(head));
		fclose(file);
		return false;
	}
	fclose(file);
	if ((head.uiTag != 0x544F4F42) && (head.uiTag != 0x2052444C))
	{
		return false;
	}
	return true;
}
bool is_update_firmware(char *szImage)
{
	FILE *file = NULL;
	STRUCT_RKIMAGE_HEAD head;
	uint_32 uiRead;
	file = fopen(szImage, "rb");
	if (!file)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, can't open file: %s\r\n", __func__, errno, szImage);
		return false;
	}
	uiRead = fread(&head, 1, sizeof(head), file);
	if (uiRead != sizeof(head))
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, read=%d, total=%d\r\n", __func__, errno, uiRead, sizeof(head));
		fclose(file);
		return false;
	}
	fclose(file);
	if (head.uiTag != 0x57464B52)
	{
		return false;
	}
	return true;
}

bool is_storage_firmware(char *szImage)
{
	FILE *file = NULL;
	STORAGE_FW_HDR head;
	uint_32 uiRead;
	file = fopen(szImage, "rb");
	if (!file)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, can't open file: %s\r\n", __func__, errno, szImage);
		return false;
	}
	uiRead = fread(&head, 1, sizeof(head), file);
	if (uiRead != sizeof(head))
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, read=%d, total=%d\r\n", __func__, errno, uiRead, sizeof(head));
		fclose(file);
		return false;
	}
	fclose(file);
	if (head.tag != SSFW_TAG)
	{
		return false;
	}
	return true;
}

bool is_jffs_image(char *szImage)
{
	FILE *file = NULL;
	uint_32 uiRead, magic;
	file = fopen(szImage, "rb");
	if (!file)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, can't open file: %s\r\n", __func__, errno, szImage);
		return false;
	}
	uiRead = fread(&magic, 1, sizeof(magic), file);
	if (uiRead != sizeof(magic))
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, read=%d, total=%d\r\n", __func__, errno, uiRead, sizeof(magic));
		fclose(file);
		return false;
	}
	fclose(file);
	if ((magic != JFFS_HEADER_MAGIC) && (magic != JFFS_HEADER_MAGIC2))
	{
		return false;
	}
	return true;
}
bool is_sparse_image(char *szImage)
{
	FILE *file = NULL;
	sparse_header head;
	uint_32 uiRead;
	file = fopen(szImage, "rb");
	if (!file)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, can't open file: %s\r\n", __func__, errno, szImage);
		return false;
	}
	uiRead = fread(&head, 1, sizeof(head), file);
	if (uiRead != sizeof(head))
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, read=%d, total=%d\r\n", __func__, errno, uiRead, sizeof(head));
		fclose(file);
		return false;
	}
	fclose(file);
	if (head.magic != SPARSE_HEADER_MAGIC)
	{
		return false;
	}
	return true;
}
bool is_dhara_image(char *szImage)
{
	FILE *file = NULL;
	uint_32 magic;
	uint_32 uiRead;
	size_t file_size;
	bool bret;

	file_size = get_file_size(szImage);
	if (file_size <= 0x7800)
		return false;
	
	file = fopen(szImage, "rb");
	if (!file)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, can't open file: %s\r\n", __func__, errno, szImage);
		return false;
	}
	fseek(file,0x7800, SEEK_SET);
	uiRead = fread(&magic, 1, sizeof(magic), file);
	if (uiRead != sizeof(magic))
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, read=%d, total=%d\r\n", __func__, errno, uiRead, sizeof(magic));
		fclose(file);
		return false;
	}
	if (magic != DHARA_HEADER_MAGIC)
	{
		bret = false;
	}
	else
		bret = true;
	fclose(file);

	return bret;
}
bool is_ubi_image(char *szImage, uint_32 *ubiPageSize = NULL)
{
	FILE *file = NULL;
	uint_32 magic;
	uint_32 uiRead;
	bool bret;
	file = fopen(szImage, "rb");
	if (!file)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, can't open file: %s\r\n", __func__, errno, szImage);
		return false;
	}
	uiRead = fread(&magic, 1, sizeof(magic), file);
	if (uiRead != sizeof(magic))
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed, err=%d, read=%d, total=%d\r\n", __func__, errno, uiRead, sizeof(magic));
		fclose(file);
		return false;
	}
	if (magic != UBI_HEADER_MAGIC)
	{
		bret = false;
	}
	else
		bret = true;
	if (bret && ubiPageSize)
	{
		fseek(file, 0x10, SEEK_SET);
		fread((void *)ubiPageSize, 1, sizeof(uint_32), file);
		*ubiPageSize = EndianU32_BtoL(*ubiPageSize);
	}
	fclose(file);

	return bret;
}
bool erase_ubi_block(CRKUsbComm *pComm, uint_32 uiOffset, uint_32 uiPartSize)
{
	STRUCT_FLASHINFO_CMD info;
	uint_8 flashID[5];
	uint_32 uiReadCount, uiStartBlock, uiEraseBlock, uiBlockCount, uiErasePos;
	int iRet;
	bool bLast = false;

	iRet = pComm->RKU_ReadFlashID(flashID);
	if (iRet != ERR_SUCCESS)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:EraseUBIBlock-->RKU_ReadFlashID failed,RetCode(%d)", iRet);
		}
		return false;
	}
	uint_32 *pID = (uint_32 *)flashID;
	if (*pID == 0x434d4d45) // emmc
	{
		return true;
	}

	iRet = pComm->RKU_ReadFlashInfo((uint_8 *)&info, &uiReadCount);
	if (iRet != ERR_SUCCESS)
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:EraseUBIBlock-->RKU_ReadFlashInfo err=%d", iRet);
		return false;
	}
	if (uiPartSize == 0xFFFFFFFF)
	{
		bLast = true;
		uiPartSize = info.uiFlashSize - uiOffset;
	}

	uiStartBlock = uiOffset / info.usBlockSize;
	if ((uiPartSize % info.usBlockSize) == 0)
		uiEraseBlock = uiPartSize / info.usBlockSize;
	else
		uiEraseBlock = uiPartSize / info.usBlockSize + 1;

	if (bLast)
		uiEraseBlock--; // subtract backup gpt

	uiErasePos = uiStartBlock;
	while (uiEraseBlock > 0)
	{
		uiBlockCount = (uiEraseBlock < MAX_ERASE_BLOCKS) ? uiEraseBlock : MAX_ERASE_BLOCKS;

		iRet = pComm->RKU_EraseBlock(0, uiErasePos, uiBlockCount, ERASE_FORCE);
		if ((iRet != ERR_SUCCESS) && (iRet != ERR_FOUND_BAD_BLOCK))
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:EraseUBIBlock-->RKU_EraseBlock failed,RetCode(%d)", iRet);
			}
			return false;
		}

		uiErasePos += uiBlockCount;
		uiEraseBlock -= uiBlockCount;
	}
	return true;
}
bool erase_partition(CRKUsbComm *pComm, uint_32 uiOffset, uint_32 uiSize, uint_32 uiErase = MAX_ERASE_LBA)
{
	bool bSuccess = true;
	int iRet;
	while (uiSize)
	{
		if (uiSize >= uiErase)
		{
			iRet = pComm->RKU_EraseLBA(uiOffset, uiErase);
			uiSize -= uiErase;
			uiOffset += uiErase;
		}
		else
		{
			iRet = pComm->RKU_EraseLBA(uiOffset, uiSize);
			uiSize = 0;
			uiOffset += uiSize;
		}
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to erase partition,err=%d", iRet);
			}
			bSuccess = false;
			break;
		}
	}
	return bSuccess;
}

bool EatSparseChunk(FILE *file, chunk_header &chunk)
{
	uint_32 uiRead;
	uiRead = fread(&chunk, 1, sizeof(chunk_header), file);
	if (uiRead != sizeof(chunk_header))
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:EatSparseChunk failed,err=%d", errno);
		}
		return false;
	}
	return true;
}
bool EatSparseData(FILE *file, uint_8 *pBuf, uint_32 dwSize)
{
	uint_32 uiRead;
	uiRead = fread(pBuf, 1, dwSize, file);
	if (uiRead != dwSize)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:EatSparseData failed,err=%d", errno);
		}
		return false;
	}
	return true;
}

char *strupr(char *szSrc)
{
	char *p = szSrc;
	while (*p)
	{
		if ((*p >= 'a') && (*p <= 'z'))
			*p = *p - 'a' + 'A';
		p++;
	}
	return szSrc;
}
void PrintData(uint_8 *pData, int nSize)
{
	char szPrint[17] = "\0";
	int i;
	for (i = 0; i < nSize; i++)
	{
		if (i % 16 == 0)
		{
			if (i / 16 > 0)
				printf("     %s\r\n", szPrint);
			printf("%08d ", i / 16);
		}
		printf("%02X ", pData[i]);
		szPrint[i % 16] = isprint(pData[i]) ? pData[i] : '.';
	}
	if (i / 16 > 0)
		printf("     %s\r\n", szPrint);
}
void PrintEfuseData(uint_8 *pData, uint_32 usSize, uint_32 usPos)
{
	string strLine, strIndex, strData;
	char buf[10];
	int i;
	uint_32 usIndex, usOffset = 0;
	strLine = strIndex = strData = "";
	for (i = 0; i < 16; i++)
	{
		sprintf(buf, "%x", i);
		strLine += buf;
		if (i < 15)
		{
			strLine += " ";
		}
	}
	strData = "     " + strLine + "\r\n";

	i = usOffset = 0;
	usIndex = usPos / 16;
	strLine = "";
	while (usSize > 0)
	{
		if ((usIndex * 16 + i) < usPos)
		{
			strLine += "  ";
		}
		else
		{
			if (pData[usOffset] == 0)
			{
				strLine += " 0";
			}
			else
				strLine += " 1";
			usSize--;
			usOffset++;
		}
		i++;

		if ((i % 16) == 0)
		{
			i = 0;
			sprintf(buf, "%04d", usIndex);
			strIndex = buf;
			strData = strData + strIndex + strLine + "\r\n";
			strLine = "";
			usIndex++;
		}
	}
	if (!strLine.empty())
	{
		sprintf(buf, "%04d", usIndex);
		strIndex = buf;
		strData = strData + strIndex + strLine;
	}
	printf("%s\r\n", strData.c_str());
}

bool MakeSectorSpareData(uint_8 *lpIDBlock, uint_32 dwSectorNum, uint_8 *lpSpareBuffer)
{
	int i = 0;
	uint_8 bchOutBuf[512 + 3 + 13];
	uint_8 bchInBuf[512 + 3];

	for (i = 0; i < dwSectorNum; i++)
	{
		memcpy(bchInBuf, lpIDBlock + 512 * i, 512);
		bchInBuf[514] = ((i == 0) ? 'i' : 0xff);
		bchInBuf[512] = 0xff;
		bchInBuf[513] = 0xff;
		bch_encode(bchInBuf, bchOutBuf);
		memcpy(lpSpareBuffer + i * 16 + 3, bchOutBuf + 515, 13);
	}

	lpSpareBuffer[2] = 'i';
	return true;
}

int find_config_item(const char *pszName)
{
	int i;
	for (i = 0; i < g_ConfigItemVec.size(); i++)
	{
		if (stricmp(pszName, g_ConfigItemVec[i].szItemName) == 0)
		{
			if (!exist_file(pszName))
				return -1;
			else
				return i;
		}
	}
	return -1;
}

bool get_download_item(char **argv, int argc, STRING_VECTOR &vecItem, STRING_VECTOR &vecItemPath, int &paramPos)
{
	if (argc <= 0)
		return false;
	int i, pos;
	string strItem, strPath;
	vecItem.clear();
	vecItemPath.clear();
	for (i = 0; i < argc; i++)
	{
		strItem = argv[i];
		if (strItem[0] != '-')
			break;
		if (stricmp(strItem.c_str(), "-p") == 0)
		{
			strItem = "parameter";
			paramPos = vecItem.size();
		}
		else if (stricmp(strItem.c_str(), "-b") == 0)
			strItem = "boot";
		else if (stricmp(strItem.c_str(), "-k") == 0)
			strItem = "kernel";
		else if (stricmp(strItem.c_str(), "-s") == 0)
			strItem = "system";
		else if (stricmp(strItem.c_str(), "-r") == 0)
			strItem = "recovery";
		else if (stricmp(strItem.c_str(), "-m") == 0)
			strItem = "misc";
		else if (stricmp(strItem.c_str(), "-u") == 0)
			strItem = "uboot";
		else if (stricmp(strItem.c_str(), "-t") == 0)
			strItem = "trust";
		else if (stricmp(strItem.c_str(), "-re") == 0)
			strItem = "resource";
		else
			strItem = strItem.substr(1);
		if ((i + 1) == argc)
			strPath = "";
		else
		{
			strPath = argv[i + 1];
			if (strPath[0] == '-')
				strPath = "";
			else
				i++;
		}

		if (strPath.size() == 0)
		{
			pos = find_config_item(strItem.c_str());
			if (pos != -1)
			{
				strPath = g_ConfigItemVec[pos].szItemValue;
			}
			else
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("%s->%s is not existed in the config.ini!", __func__, strItem.c_str());
				}
				return false;
			}
		}
		else
		{
			if (!exist_file(strPath))
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("%s->%s is not existed,err=%d!", __func__, strPath.c_str(), errno);
				}
				return false;
			}
		}
		vecItem.push_back(strItem);
		vecItemPath.push_back(strPath);
	}

	return (vecItem.size() > 0);
}

bool get_parameter_loader(STRUCT_RKDEVICE_DESC &deviceDesc, char *pParameter, int &nParamSize, bool bDirectLBA = false)
{
	if ((nParamSize != -1) && (!pParameter))
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("get_parameter_loader->pParameter is null!");
		}
		return false;
	}
	CRKUsbComm *pComm = NULL;
	uint_8 paramHead[512];
	uint_32 *pParamTag = (uint_32 *)paramHead;
	uint_32 *pParamSize = (uint_32 *)(paramHead + 4);
	int iRet;
	uint_32 uiBegin = 0;
	bool bRet;
	if (bDirectLBA)
		uiBegin = 0x2000;

	pComm = new CRKUsbComm(deviceDesc, g_pLogObject, bRet);
	if (!bRet)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("get_parameter_loader->Creating comm object failed!");
		}
		return false;
	}

	iRet = pComm->RKU_ReadLBA(uiBegin, 1, paramHead);
	if (iRet != ERR_SUCCESS)
	{
		delete pComm;
		pComm = NULL;
		if (g_pLogObject)
		{
			g_pLogObject->Record("get_parameter_loader->Read parameter head failed,err=%d!", iRet);
		}
		return false;
	}
	if (*pParamTag != 0x4D524150)
	{
		delete pComm;
		pComm = NULL;
		if (g_pLogObject)
		{
			g_pLogObject->Record("get_parameter_loader->Check parameter tag failed!");
		}
		return false;
	}
	if (nParamSize == -1)
	{
		nParamSize = *pParamSize;
		delete pComm;
		pComm = NULL;
		return true;
	}
	if (nParamSize < *pParamSize)
	{
		delete pComm;
		pComm = NULL;
		if (g_pLogObject)
		{
			g_pLogObject->Record("get_parameter_loader->Memory is not enough to save parameter!");
		}
		return false;
	}

	nParamSize = *pParamSize;
	int nParamSec;
	nParamSec = (nParamSize + 12 - 1) / 512 + 1;
	uint_8 *pBuffer = NULL;
	pBuffer = new uint_8[nParamSec * 512];
	if (!pBuffer)
	{
		delete pComm;
		pComm = NULL;
		if (g_pLogObject)
		{
			g_pLogObject->Record("get_parameter_loader->New memory failed!");
		}
		return false;
	}
	iRet = pComm->RKU_ReadLBA(uiBegin, nParamSec, pBuffer);
	delete pComm;
	pComm = NULL;
	if (iRet != ERR_SUCCESS)
	{
		delete[] pBuffer;
		pBuffer = NULL;
		if (g_pLogObject)
		{
			g_pLogObject->Record("get_parameter_loader->Read parameter file failed,err=%d!", iRet);
		}
		return false;
	}

	memcpy(pParameter, pBuffer + 8, nParamSize);
	delete[] pBuffer;
	pBuffer = NULL;
	return true;
}
bool get_parameter_device(STRUCT_RKDEVICE_DESC &deviceDesc, PARAM_ITEM_VECTOR &vecItem, bool &bGpt, bool &bEnv, bool &bMcu)
{
	CRKUsbComm *pComm = NULL;
	STRUCT_FLASHINFO_CMD info;
	int iRet,nLBsector, nGptEntryLB, nMasterGptSector;
	bool bRet, bSuccess = false, bDirectLBA = false;
	uint_8 *buf=NULL;
	char *pParam = NULL;
	int nParamSize = -1;
	uint_32 *pParamTag,uiReadCount;
	gpt_header *gptMasterHead;

	bEnv = false; bGpt = false; bMcu = false;

	pComm = new CRKUsbComm(deviceDesc, g_pLogObject, bRet);
	if (!bRet)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		}
		goto exit_get_param_device;
	}
	iRet = pComm->RKU_ReadFlashInfo((uint_8 *)&info, &uiReadCount);
	if (iRet != ERR_SUCCESS)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to read flash info,err=%d", __func__, iRet);
		}
		goto exit_get_param_device;
	}
	if (info.bManufCode > 200) {
		nLBsector = info.bManufCode - 200;
		nGptEntryLB = CALC_UNIT(128, (nLBsector * 4));
		nMasterGptSector = (2 + nGptEntryLB) * nLBsector;
	}
	else {
		nLBsector = 1;
		nGptEntryLB = 32;
		nMasterGptSector = 34;
	}
	buf = new uint_8[nMasterGptSector * SECTOR_SIZE];
	if (!buf)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to new memory,err=%d", __func__, errno);
		}
		goto exit_get_param_device;
	}
	iRet = pComm->RKU_ReadLBA(0, nMasterGptSector, buf);
	if (iRet != ERR_SUCCESS)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to readlba on checking gpt or parameter,err=%d", __func__, iRet);
		}
		goto exit_get_param_device;
	}
	gptMasterHead = (gpt_header *)(buf + nLBsector * 512);
	if (gptMasterHead->signature == le64_to_cpu(GPT_HEADER_SIGNATURE))
	{
		bGpt = true;
		if (!parse_gpt(buf, vecItem, nLBsector))
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to parse gpt", __func__);
			}
			goto exit_get_param_device;
		}
	}
	else
	{
		pParamTag = (uint_32 *)buf;
		if (*pParamTag != 0x4D524150)
		{
			if (*pParamTag == 0x50464B52)
			{
				bMcu = true;
				if (parse_mcu(buf, vecItem))
				{
					bSuccess = true;
					goto exit_get_param_device;
				}
			}
			else if (*pParamTag != 0)
			{
				bEnv = true;
				if (parse_env(buf, vecItem))
				{
					bSuccess = true;
					goto exit_get_param_device;
				}
			}
			bDirectLBA = true;
		}
		if (pComm)
		{
			delete pComm;
			pComm = NULL;
		}
		bRet = get_parameter_loader(deviceDesc, pParam, nParamSize, bDirectLBA);
		if (!bRet)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to get size of parameter from loader", __func__);
			}
			goto exit_get_param_device;
		}
		pParam = new char[nParamSize];
		bRet = get_parameter_loader(deviceDesc, pParam, nParamSize, bDirectLBA);
		if (!bRet)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to get parameter from loader", __func__);
			}
			goto exit_get_param_device;
		}
		if (!parse_parameter(pParam, vecItem))
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to parse parameter", __func__);
			}
			goto exit_get_param_device;
		}
	}
	bSuccess = true;
exit_get_param_device:
	if (pComm)
		delete pComm;
	if (pParam)
		delete []pParam;
	if (buf)
		delete []buf;
	return bSuccess;
}

bool get_parameter_gptflag(char *pParam)
{
	bool bGpt = false;
	string strParamFile = pParam;
	stringstream paramStream(strParamFile);

	string strLine;
	string::size_type pos, line_size;
	while (!paramStream.eof())
	{
		getline(paramStream, strLine);
		line_size = strLine.size();
		if (line_size <= 0)
		{
			continue;
		}
		if (strLine[line_size - 1] == '\r')
		{
			strLine = strLine.substr(0, line_size - 1);
		}
		if (strLine.size() <= 0)
		{
			continue;
		}
		if (strLine[0] == '#')
		{
			continue;
		}
		transform(strLine.begin(), strLine.end(), strLine.begin(), (int (*)(int))toupper);
		pos = strLine.find("TYPE");
		if (pos == string::npos)
		{
			continue;
		}
		if ((pos = strLine.find("GPT")) != string::npos)
		{
			bGpt = true;
			break;
		}
		else
			continue;
	}
	return bGpt;
}
unsigned int get_parameter_gpt_size(char *pParam)
{
	unsigned int uiSize = (unsigned int)-1;
	string strParamFile = pParam;
	stringstream paramStream(strParamFile);

	string strLine, strSize;
	string::size_type pos, line_size;
	while (!paramStream.eof())
	{
		getline(paramStream, strLine);
		line_size = strLine.size();
		if (line_size <= 0)
		{
			continue;
		}
		if (strLine[line_size - 1] == '\r')
		{
			strLine = strLine.substr(0, line_size - 1);
		}
		if (strLine.size() <= 0)
		{
			continue;
		}
		if (strLine[0] == '#')
		{
			continue;
		}
		transform(strLine.begin(), strLine.end(), strLine.begin(), (int (*)(int))toupper);
		pos = strLine.find("DISKSIZE");
		if (pos == string::npos)
		{
			continue;
		}
		if ((pos = strLine.find(":")) != string::npos)
		{
			strSize = strLine.substr(pos + 1);
			strSize.erase(0, strSize.find_first_not_of(" "));
			strSize.erase(strSize.find_last_not_of(" ") + 1);
			sscanf(strSize.c_str(), "0X%x", &uiSize);
			return uiSize;
		}
		else
			continue;
	}
	return uiSize;
}

bool parse_parameter_file(char *pParamFile, PARAM_ITEM_VECTOR &vecItem)
{
	FILE *file = NULL;
	file = fopen(pParamFile, "rb");
	if (!file)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed,err=%d,can't open file: %s\r\n", __func__, errno, pParamFile);
		return false;
	}
	int iFileSize;
	fseek(file, 0, SEEK_END);
	iFileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	char *pParamBuf = NULL;
	pParamBuf = new char[iFileSize];
	if (!pParamBuf)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed,err=%d,can't new memory\r\n", __func__, errno);
		fclose(file);
		return false;
	}
	int iRead;
	iRead = fread(pParamBuf, 1, iFileSize, file);
	if (iRead != iFileSize)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed,err=%d,read=%d,total=%d\r\n", __func__, errno, iRead, iFileSize);
		fclose(file);
		delete[] pParamBuf;
		return false;
	}
	fclose(file);
	bool bRet;
	bRet = parse_parameter(pParamBuf, vecItem);
	if (!bRet)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed,can't find mtdparts in the parameter\r\n", __func__);
		g_bGpt = false;
	}
	else
		g_bGpt = get_parameter_gptflag(pParamBuf);
	delete[] pParamBuf;
	return bRet;
}
bool parse_config(char *pConfig, CONFIG_ITEM_VECTOR &vecItem)
{

	stringstream configStream(pConfig);
	string strLine, strItemName, strItemValue;
	string::size_type line_size, pos;
	STRUCT_CONFIG_ITEM item;
	vecItem.clear();
	while (!configStream.eof())
	{
		getline(configStream, strLine);
		line_size = strLine.size();
		if (line_size == 0)
			continue;
		if (strLine[0] == '#')
		{
			continue;
		}
		if (strLine[line_size - 1] == '\r')
		{
			strLine = strLine.substr(0, line_size - 1);
		}
		pos = strLine.find("=");
		if (pos == string::npos)
		{
			continue;
		}
		strItemName = strLine.substr(0, pos);
		strItemValue = strLine.substr(pos + 1);
		strItemName.erase(0, strItemName.find_first_not_of(" "));
		strItemName.erase(strItemName.find_last_not_of(" ") + 1);
		strItemValue.erase(0, strItemValue.find_first_not_of(" "));
		strItemValue.erase(strItemValue.find_last_not_of(" ") + 1);
		if ((strItemName.size() > 0) && (strItemValue.size() > 0))
		{
			strcpy(item.szItemName, strItemName.c_str());
			strcpy(item.szItemValue, strItemValue.c_str());
			vecItem.push_back(item);
		}
	}
	return true;
}
bool parse_config_file(const char *pConfigFile, CONFIG_ITEM_VECTOR &vecItem)
{
	FILE *file = NULL;
	file = fopen(pConfigFile, "rb");
	if (!file)
	{
		if (g_pLogObject)
			g_pLogObject->Record("parse_config_file failed,err=%d,can't open file: %s\r\n", errno, pConfigFile);
		return false;
	}
	int iFileSize;
	fseek(file, 0, SEEK_END);
	iFileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	char *pConfigBuf = NULL;
	pConfigBuf = new char[iFileSize + 1];
	if (!pConfigBuf)
	{
		fclose(file);
		return false;
	}
	memset(pConfigBuf, 0, iFileSize + 1);
	int iRead;
	iRead = fread(pConfigBuf, 1, iFileSize, file);
	if (iRead != iFileSize)
	{
		if (g_pLogObject)
			g_pLogObject->Record("parse_config_file failed,err=%d,read=%d,total=%d\r\n", errno, iRead, iFileSize);
		fclose(file);
		delete[] pConfigBuf;
		return false;
	}
	fclose(file);
	bool bRet;
	bRet = parse_config(pConfigBuf, vecItem);
	delete[] pConfigBuf;
	return bRet;
}
void list_device(CRKScan *pScan)
{
	STRUCT_RKDEVICE_DESC desc;
	string strDevType;
	int i, cnt;
	cnt = pScan->DEVICE_COUNTS;
	for (i = 0; i < cnt; i++)
	{
		pScan->GetDevice(desc, i);
		if (desc.emUsbType == RKUSB_MASKROM)
			strDevType = "Maskrom";
		else if (desc.emUsbType == RKUSB_LOADER)
			strDevType = "Loader";
		else
			strDevType = "Msc";

		printf("DevNo=%d\tVid=0x%x,Pid=0x%x,LocationID=%x\tMode=%s\tSerialNo=%s\r\n", i + 1, desc.usVid,
			   desc.usPid, desc.uiLocationID, strDevType.c_str(), desc.strSerial.c_str());
	}
}
void list_disk(CmDiskBaseVec vecDisk)
{
	CRKDisk *pDisk = NULL;
	int i, cnt;
	cnt = vecDisk.size();
	bool bRemovable;
	double disk_size_g;
	for (i = 0; i < cnt; i++)
	{
		pDisk = new CRKDisk(vecDisk[i]);
		if (pDisk)
		{
			if (pDisk->OpenDisk())
			{
				if (pDisk->IsRemovedDisk(bRemovable))
				{
					if (bRemovable)
					{
						disk_size_g = (double)pDisk->GetDiskSize() / 1024 / 1024 / 1024;
						printf("No=%d\t\tDiskID=%x\tFriendlyName=%s\tDiskSize=%.1fG\r\n", i + 1, vecDisk[i].diskNum, vecDisk[i].diskDesc.c_str(), disk_size_g);
					}
				}
				pDisk->CloseDisk();
			}
			delete pDisk;
			pDisk = NULL;
		}
	}
}
int choose_device(CRKScan *pScan)
{
	int cnt;
	int nSelect;
	char szUserInput[256];
	do
	{
		printf("List of rockusb connected\r\n");
		cnt = pScan->Search(RKUSB_MASKROM | RKUSB_LOADER | RKUSB_MSC);
		list_device(pScan);
		nSelect = 0;
		do
		{
			if (cnt <= 0)
			{
				printf("No found rockusb,Rescan press <R>,Quit press <Q>:");
			}
			else
			{
				printf("Found %d rockusb,Select input DevNo,Rescan press <R>,Quit press <Q>:", cnt);
			}
			fgets(szUserInput, 256, stdin);
			szUserInput[1] = 0; // only accept one char input
			strupr(szUserInput);
			if (strcmp(szUserInput, "Q") == 0)
			{
				return -1;
			}
			else if (strcmp(szUserInput, "R") == 0)
			{
				break;
			}
			nSelect = atoi(szUserInput);
			if ((nSelect <= 0) || (nSelect > cnt))
			{
				nSelect = 0;
			}
			else
				break;

		} while (1);

		if (nSelect > 0)
		{
			break;
		}

	} while (1);
	return nSelect;
}
bool check_device_exist(STRUCT_RKDEVICE_DESC &dev, CRKScan *pScan)
{
	int cnt;
	bool bRet;
	STRUCT_RKDEVICE_DESC newDev;
	cnt = pScan->Search(RKUSB_MASKROM | RKUSB_LOADER | RKUSB_MSC);
	int i;
	bool bFound = false;
	for (i = 0; i < cnt; i++)
	{
		bRet = pScan->GetDevice(newDev, i);
		if (bRet)
		{
			if (dev.uiLocationID == newDev.uiLocationID)
			{
				dev.emDeviceType = newDev.emDeviceType;
				dev.emUsbType = newDev.emUsbType;
				dev.pUsbHandle = newDev.pUsbHandle;
				dev.usbcdUsb = newDev.usbcdUsb;
				dev.usPid = newDev.usPid;
				dev.usVid = newDev.usVid;
				dev.strDevPath = newDev.strDevPath;
				bFound = true;
				break;
			}
		}
	}
	if (!bFound)
	{
		ERROR_COLOR_ATTR;
		printf("No found match device,please press CD to choose device!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
	}
	return bFound;
}
bool check_device_type(STRUCT_RKDEVICE_DESC &dev, uint_32 uiSupportType)
{
	if ((dev.emUsbType & uiSupportType) == dev.emUsbType)
		return true;
	else
	{
		ERROR_COLOR_ATTR;
		printf("The  Device did not support this operation!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		return false;
	}
}
bool switch_device(STRUCT_RKDEVICE_DESC &dev)
{
	if (!check_device_type(dev, RKUSB_MSC))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	int iRet;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		if (dev.usPid == 0x0016)
		{
			iRet = pComm->RKU_UvcToRockusb();
		}
		else
		{
			iRet = pComm->RKU_MscToRockusb();
		}

		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to switch,err=%d!", __func__, iRet);
			}
			printf("Switch Device Fail!\r\n");
		}
		else
		{
			bSuccess = true;
			printf("Switch Device OK.\r\n");
		}
	}
	else
	{
		printf("Switch Device quit,Creating comm object failed!\r\n");
	}
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	return bSuccess;
}
bool read_capability(STRUCT_RKDEVICE_DESC &dev, bool bPrint = false)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	int iRet;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		uint_8 capability[8];
		iRet = pComm->RKU_ReadCapability(capability);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to read capability,err=%d", __func__, iRet);
			if (bPrint)
				printf("Read capability Fail!\r\n");
		}
		else
		{
			bSuccess = true;
			if (bPrint)
			{
				printf("Capability:%02X %02X %02X %02X %02X %02X %02X %02X \r\n",
					   capability[0], capability[1], capability[2], capability[3],
					   capability[4], capability[5], capability[6], capability[7]);
			}
			if (capability[0] & 1)
			{
				g_bDirectLBA = true;
				if (bPrint)
					printf("Direct LBA:\tenabled\r\n");
			}
			else
				g_bDirectLBA = false;
			if (capability[0] & 2)
			{
				if (bPrint)
					printf("Vendor Storage:\tenabled\r\n");
			}

			if (capability[0] & 4)
			{
				g_bFirst4Access = true;
				if (bPrint)
					printf("First 4m Access:\tenabled\r\n");
			}
			else
				g_bFirst4Access = false;

			if (capability[0] & 8)
			{
				g_bReadLBA = true;
				if (bPrint)
					printf("Read LBA:\tenabled\r\n");
			}
			else
				g_bReadLBA = false;

			if (capability[0] & 20)
			{
				g_bReadComLog = true;
				if (bPrint)
					printf("Read Com Log:\tenabled\r\n");
			}
			else
				g_bReadComLog = false;

			if (capability[0] & 40)
			{
				if (bPrint)
					printf("Read IDB Config:\tenabled\r\n");
			}

			if (capability[0] & 80)
			{
				if (bPrint)
					printf("Read Secure Mode:\tenabled\r\n");
			}

			if (capability[1] & 1)
			{
				if (bPrint)
					printf("New IDB:\tenabled\r\n");
			}

			if (capability[1] & 2)
			{
				g_bSwitchStorage = true;
				if (bPrint)
					printf("Switch Storage:\tenabled\r\n");
			}
			else
				g_bSwitchStorage = false;

			if (capability[1] & 4)
			{
				if (bPrint)
					printf("LBA Parity:\tenabled\r\n");
			}

			if (capability[1] & 8)
			{
				g_bReadOtpChip = true;
				if (bPrint)
					printf("Read OTP Chip:\tenabled\r\n");
			}
			else
				g_bReadOtpChip = false;

			if (capability[1] & 0x10)
			{
				g_bSwitchUsb3 = true;
				if (bPrint)
					printf("Switch USB3:\tenabled\r\n");
			}
			else
				g_bSwitchUsb3 = false;
		}
	}
	else
	{
		if (bPrint)
			printf("Read capability quit,Creating comm object failed!\r\n");
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
	}
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	return bSuccess;
}

void DestroyUpgradeSet(RK_UPGRADE_SET &ctrlUpgradeStruct)
{
	ctrlUpgradeStruct.clear();
}

void DestroyImages(vector<CRKImage *> &vecImages)
{
	int i;
	for (i = 0; i < vecImages.size(); i++)
	{
		if (vecImages[i])
			delete vecImages[i];
	}
	vecImages.clear();
}
uint_8 get_storage_index(uint_32 storage)
{
	if (storage & (1 << 0))
		return 0;
	else if (storage & (1 << 1))
		return 1;
	else if (storage & (1 << 2))
		return 2;
	else if (storage & (1 << 3))
		return 3;
	else if (storage & (1 << 6))
		return 6;
	else if (storage & (1 << 7))
		return 7;
	else if (storage & (1 << 8))
		return 8;
	else if (storage & (1 << 9))
		return 9;
	else if (storage & (1 << 10))
		return 10;
	else if (storage & (1 << 11))
		return 11;
	else if (storage & (1 << 12))
		return 12;
	else if (storage & (1 << 13))
		return 13;
	else
		return 0xFF;
}
bool get_device_storage(CRKComm *pComm, uint_8 *allStorage, STORAGE_CODE &curStorage)
{
	int iTry = 3, iRet;
	uint_8 data[8];
	iRet = pComm->RKU_ReadCapability(data);
	if (iRet == ERR_SUCCESS)
	{
		if (data[1] & (0x1 << 1))
		{
			do
			{
				iRet = pComm->RKU_ReadStorageList(allStorage);
				if (iRet == ERR_SUCCESS)
				{
					curStorage = (*(STORAGE_CODE *)allStorage);
					return true;
				}
				if (iRet == ERR_DEVICE_UNREADY)
				{
					sleep_ms(1000);
					iTry--;
					continue;
				}
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s-->RKU_ReadStorageList failed,err=%d", __func__, iRet);
				}
				break;

			} while (iTry > 0);
		}
	}
	else
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s-->RKU_ReadCapability failed,err=%d", __func__, iRet);
		}
	}
	return false;
}

bool switch_device_storage(CRKComm *pComm, STORAGE_CODE storage)
{
	int iRet;
	uint_8 index;
	index = get_storage_index(storage);
	if (index == 0xFF)
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s-->switch_device_storage invalid storage,storage=0x%x", __func__, storage);
		return false;
	}
	iRet = pComm->RKU_SwitchStorage(index);

	if (iRet != ERR_SUCCESS)
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s-->RKU_SwitchStorage failed,err=%d", __func__, iRet);
		return false;
	}
	return true;
}
string get_composite_storage_string(uint32_t storage)
{
	string value, ret = "";
	size_t i;
	for (i = 0; i < 32; i++)
	{
		if (storage & (1 << i))
		{
			if (i == 0)
				value = "FLASH";
			else if (i == 1)
				value = "EMMC";
			else if (i == 2)
				value = "SD";
			else if (i == 9)
				value = "SPINOR";
			else if (i == 8)
				value = "SPINAND";
			else if (i == 10)
				value = "SATA";
			else if (i == 11)
				value = "PCIE";
			else if (i == 12)
				value = "UFS";
			else if (i == 13)
				value = "RVD";
			else
				continue;
			if (ret.size() == 0)
				ret = value;
			else
				ret = ret + "|" + value;
		}
	}
	return ret;
}
string get_storage_string(uint_32 storage)
{
	if (storage & (1 << 0))
		return "FLASH";
	else if (storage & (1 << 1))
		return "EMMC";
	else if (storage & (1 << 2))
		return "SD";
	else if (storage & (1 << 3))
		return "SD1";
	else if (storage & (1 << 9))
		return "SPINOR";
	else if (storage & (1 << 8))
		return "SPINAND";
	else if (storage & (1 << 6))
		return "RAM";
	else if (storage & (1 << 7))
		return "USB";
	else if (storage & (1 << 10))
		return "SATA";
	else if (storage & (1 << 11))
		return "PCIE";
	else if (storage & (1 << 12))
		return "UFS";
	else if (storage & (1 << 13))
		return "RVD";
	else
		return "";
}
uint_32 get_storage_value(string strStorage)
{
	if (stricmp(strStorage.c_str(), "FLASH") == 0)
		return 1 << 0;
	else if (stricmp(strStorage.c_str(), "EMMC") == 0)
		return 1 << 1;
	else if (stricmp(strStorage.c_str(), "SD") == 0)
		return 1 << 2;
	else if (stricmp(strStorage.c_str(), "SD1") == 0)
		return 1 << 3;
	else if (stricmp(strStorage.c_str(), "SPINOR") == 0)
		return 1 << 9;
	else if (stricmp(strStorage.c_str(), "SPINAND") == 0)
		return 1 << 8;
	else if (stricmp(strStorage.c_str(), "RAM") == 0)
		return 1 << 6;
	else if (stricmp(strStorage.c_str(), "USB") == 0)
		return 1 << 7;
	else if (stricmp(strStorage.c_str(), "SATA") == 0)
		return 1 << 10;
	else if (stricmp(strStorage.c_str(), "PCIE") == 0)
		return 1 << 11;
	else if (stricmp(strStorage.c_str(), "UFS") == 0)
		return 1 << 12;
	else if (stricmp(strStorage.c_str(), "RVD") == 0)
		return 1 << 13;
	else
		return 0;
}
bool upgrade_firmware(STRUCT_RKDEVICE_DESC &dev, char *szFirmware, bool bNoReset = false)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKImage *pImage = NULL;
	vector<CRKImage *> vecImages, vecSelImages;
	bool bRet, bSuccess = false, bFirstStorage;
	int ret, i, err = 0;
	char szInfo[128];
	string strErr;
	uint_32 allStor;
	STORAGE_CODE curStor = (STORAGE_CODE)0;
	printf("Loading firmware...\r\n");
	if (is_storage_firmware(szFirmware))
	{
		STORAGE_FW_HDR hdr;
		STORAGE_ENTRY entry;
		vector<STORAGE_ENTRY> vecEntrys;
		FILE *file = NULL;
		size_t nFileSize;
		nFileSize = get_file_size(szFirmware);
		file = fopen(szFirmware, "rb");
		if (file)
		{
			fread(&hdr, 1, sizeof(hdr), file);
			for (i = 0; i < hdr.entry_count; i++)
			{
				fread(&entry, 1, hdr.entry_size, file);
				vecEntrys.push_back(entry);
			}
			fclose(file);
			for (i = 0; i < vecEntrys.size(); i++)
			{
				if (vecEntrys[i].offset == 0xFFFFFFFF)
				{
					vecImages.push_back(NULL);
					continue;
				}
				if (nFileSize < (vecEntrys[i].offset + vecEntrys[i].size))
				{
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:%s->%s data is out of file!", __func__, vecEntrys[i].file);
					}
					DestroyImages(vecImages);
					break;
				}

				pImage = new CRKImage();
				ret = pImage->ObjectInit(szFirmware, false, vecEntrys[i].offset, vecEntrys[i].size);
				if (ret != 0)
				{
					err = pImage->Error;
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:%s->Fail to create image object,ret=%d!", __func__, ret);
					}
					delete pImage;
					DestroyImages(vecImages);
					break;
				}
				if (hdr.entry_size == sizeof(STORAGE_ENTRY))
					pImage->Storage = vecEntrys[i].storage;
				vecImages.push_back(pImage);
			}
		}
	}
	else
	{
		pImage = new CRKImage();
		ret = pImage->ObjectInit(szFirmware, false);
		if (ret != 0)
		{
			err = pImage->Error;
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to create image object,ret=%d!", __func__, ret);
			}
			delete pImage;
		}
		else
			vecImages.push_back(pImage);
	}
	if (vecImages.size() == 0)
	{
		ERROR_COLOR_ATTR;
		printf("Loading firmware failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		if (err)
		{
			strErr = GetErrString(err);
			if (strErr.size())
				printf("%s\r\n", strErr.c_str());
		}
		return bSuccess;
	}
	pImage = vecImages[0];

	string strTemp;
	char szText[100];
	uint_8 btMajor, btMinor;
	uint_16 usSmall;
	ENUM_RKDEVICE_TYPE type;
	type = pImage->SupportDevice;
	switch (type)
	{
	case RK27_DEVICE:
		strTemp = "RK27";
		break;
	case RK28_DEVICE:
		strTemp = "RK28";
		break;
	case RK281X_DEVICE:
		strTemp = "RK281X";
		break;
	case RKPANDA_DEVICE:
		strTemp = "RKPANDA";
		break;
	case RKCROWN_DEVICE:
		strTemp = "RKCROWN";
		break;
	case RKNANO_DEVICE:
		strTemp = "RKNANO";
		break;
	case RKSMART_DEVICE:
		strTemp = "RKSMART";
		break;
	case RKCAYMAN_DEVICE:
		strTemp = "RKCAYMAN";
		break;
	case RK29_DEVICE:
		strTemp = "RK29";
		break;
	case RK292X_DEVICE:
		strTemp = "RK292X";
		break;
	case RK30_DEVICE:
		strTemp = "RK30";
		break;
	case RK30B_DEVICE:
		strTemp = "RK30B";
		break;
	case RK31_DEVICE:
		strTemp = "RK31";
		break;
	case RK32_DEVICE:
		strTemp = "RK32";
		break;
	default:
		char szChip[7];
		memset(szChip, 0, 7);
		uint_8 *pValue = (uint_8 *)(&type);
		// szChip[0] = 'R';
		// szChip[1] = 'K';
		szChip[0] = (char)(pValue[3]);
		szChip[1] = (char)(pValue[2]);
		szChip[2] = (char)(pValue[1]);
		szChip[3] = (char)(pValue[0]);
		strTemp = szChip;
	}
	printf("Support Type:%s\t", strTemp.c_str());

	btMajor = ((pImage->Version) & 0xFF000000) >> 24;
	btMinor = ((pImage->Version) & 0x00FF0000) >> 16;
	usSmall = (pImage->Version) & 0x0000FFFF;
	sprintf(szText, "%x.%x.%02x", btMajor, btMinor, usSmall);
	printf("FW Ver:%s\t", szText);

	sprintf(szText, "%04u-%02u-%02u %02u:%02u:%02u", ((STRUCT_RKTIME)pImage->ReleaseTime).usYear, ((STRUCT_RKTIME)pImage->ReleaseTime).ucMonth, ((STRUCT_RKTIME)pImage->ReleaseTime).ucDay,
			((STRUCT_RKTIME)pImage->ReleaseTime).ucHour, ((STRUCT_RKTIME)pImage->ReleaseTime).ucMinute, ((STRUCT_RKTIME)pImage->ReleaseTime).ucSecond);
	printf("FW Time:%s\r\n", szText);

	btMajor = ((pImage->m_bootObject->Version) & 0x0000FF00) >> 8;
	btMinor = ((pImage->m_bootObject->Version) & 0x000000FF);
	sprintf(szText, "%u.%02u", btMajor, btMinor);
	printf("Loader ver:%s\t", szText);

	sprintf(szText, "%04u-%02u-%02u %02u:%02u:%02u", ((STRUCT_RKTIME)pImage->m_bootObject->ReleaseTime).usYear, ((STRUCT_RKTIME)pImage->m_bootObject->ReleaseTime).ucMonth,
			((STRUCT_RKTIME)pImage->m_bootObject->ReleaseTime).ucDay, ((STRUCT_RKTIME)pImage->m_bootObject->ReleaseTime).ucHour, ((STRUCT_RKTIME)pImage->m_bootObject->ReleaseTime).ucMinute, ((STRUCT_RKTIME)pImage->m_bootObject->ReleaseTime).ucSecond);
	printf("Loader Time:%s\r\n", szText);

	CRKScan *pScan = NULL;
	pScan = new CRKScan();
	pScan->SetVidPid(g_usMscVid, g_usMscPid);
	pScan->AddRockusbVidPid(g_usDevNewVid, g_usDevNewPid, g_usDevOldVid, g_usDevOldPid);
	CRKComm *pComm = NULL;
	CRKDevice *pDevice = NULL;
	CRKUpgrade *pUpgradeCtrlObject = NULL;
	uint_32 uiMscOper = MSC_NONE_OPER;
	RK_UPGRADE_SET ctrlStepStruct;

	type = dev.emDeviceType = pImage->SupportDevice;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (!bRet)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		}
		if (pScan)
			delete pScan;
		DestroyImages(vecImages);
		ERROR_COLOR_ATTR;
		printf("Creating Comm Object failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		return bSuccess;
	}

	bRet = CRKUpgrade::CreateUpgradeStep(type, ctrlStepStruct, uiMscOper);
	if (!bRet)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to create step object!", __func__);
		}
		if (pComm)
			delete pComm;
		if (pScan)
			delete pScan;
		DestroyImages(vecImages);
		ERROR_COLOR_ATTR;
		printf("Creating upgrade step failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		return bSuccess;
	}
	// remove reset step
	ctrlStepStruct.pop_back();

	pDevice = CDeviceFactory::CreateDevice(dev);
	if (!pDevice)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to create device object!", __func__);
		}
		if (pComm)
			delete pComm;
		if (pScan)
			delete pScan;
		DestroyImages(vecImages);
		DestroyUpgradeSet(ctrlStepStruct);
		ERROR_COLOR_ATTR;
		printf("Creating device object failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		return bSuccess;
	}

	pDevice->RemallocDisk = false;
	pDevice->SetObject(pImage, pComm, g_pLogObject);
	pDevice->CallBackPointer = ProgressInfoProc;
	pDevice->MiscModifyFlag = MISC_MODIFY_NONE;
	pDevice->NoCheckMode = g_bReadCheckOff;
	pDevice->IDBCheckMode = g_bCloseCheckIDB;
	pDevice->NorSingleIDB = g_bNorSingleIDB;
	pDevice->ForceDataBand = g_uiForceBand;
	pDevice->Usb3Transfer = g_bUsb3Transfer;
	pDevice->UsbDriver4M = g_bUsbDriver4m;

	pUpgradeCtrlObject = new CRKUpgrade(ctrlStepStruct, WF_UPGRADE);

	bRet = pUpgradeCtrlObject->SetObject(pDevice, pScan);
	if (!bRet)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to create upgrade object!", __func__);
		}
		if (pUpgradeCtrlObject)
			delete pUpgradeCtrlObject;
		if (pDevice)
			CDeviceFactory::FreeDevice(pDevice);
		else
		{
			if (pComm)
				delete pComm;
		}
		if (pScan)
			delete pScan;
		DestroyImages(vecImages);
		DestroyUpgradeSet(ctrlStepStruct);
		ERROR_COLOR_ATTR;
		printf("Creating upgrade object failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		return bSuccess;
	}
	pUpgradeCtrlObject->CallBackPointer = PromptInfoProc;

	printf("Start to upgrade firmware...\r\n");
	if (dev.emUsbType == RKUSB_MASKROM)
	{
		pUpgradeCtrlObject->RemoveStep(DOWNLOADBOOT_STEP);
		pUpgradeCtrlObject->RemoveStep(WAITMASKROM_STEP);
		if (dev.strSerial.size() == 0)
		{
			PromptInfoProc(pDevice->LocationID, DOWNLOADBOOT_START, 0);
			ret = pDevice->DownloadBoot();
			if (ret != 0)
			{
				PromptInfoProc(pDevice->LocationID, DOWNLOADBOOT_FAIL, 0);
				goto Exit_Upgrade;
			}
			PromptInfoProc(pDevice->LocationID, DOWNLOADBOOT_PASS, 0);

			PromptInfoProc(pDevice->LocationID, WAITMASKROM_START, 0);
			dev.emUsbType = RKUSB_NONE;
			bRet = pScan->Wait(dev, RKUSB_MASKROM);
			if (!bRet)
			{
				PromptInfoProc(pDevice->LocationID, WAITMASKROM_FAIL, 0);
				goto Exit_Upgrade;
			}
			PromptInfoProc(pDevice->LocationID, WAITMASKROM_PASS, 0);
			pDevice->VendorID = dev.usVid;
			pDevice->ProductID = dev.usPid;
			pDevice->UsbType = dev.emUsbType;
			pDevice->BcdUsb = dev.usbcdUsb;
			pDevice->LocationID = dev.uiLocationID;
			pDevice->LinkName = dev.strDevPath;
			pComm = (CRKComm *)pDevice->CommObjectPointer;
			pComm->Reset_Usb_Config(dev);
		}
	}
	vecSelImages.clear();
	if (vecImages.size() > 1)
	{
		bFirstStorage = true;
		for (i = 0; i < vecImages.size(); i++)
		{
			pImage = vecImages[i];
			if (!pImage)
			{
				if (vecSelImages.size())
				{
					break;
				}
				bFirstStorage = true;
				continue;
			}
			if (pImage->Storage == 0)
			{
				if (vecSelImages.size())
				{
					continue;
				}
				vecSelImages.push_back(pImage);
				break;
			}

			if ((uint32_t)curStor == 0)
			{
				bRet = get_device_storage(pComm, (uint_8 *)&allStor, curStor);
				if (!bRet)
				{
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:%s->Fail to get device storage", __func__);
					}
					CURSOR_MOVEUP_LINE(1);
					CURSOR_DEL_LINE;
					ERROR_COLOR_ATTR;
					printf("Failed to get storage from device");
					NORMAL_COLOR_ATTR;
					printf("\r\n");
					goto Exit_Upgrade;
				}
			}
			if (bFirstStorage)
			{
				bFirstStorage = false;
				if ((uint32_t)curStor & pImage->GetStorage())
				{
					vecSelImages.push_back(pImage);
				}
			}
			else
			{
				if (vecSelImages.size())
				{
					vecSelImages.push_back(pImage);
				}
			}
		}
		if (vecSelImages.size() == 0)
		{
			CURSOR_MOVEUP_LINE(1);
			CURSOR_DEL_LINE;
			ERROR_COLOR_ATTR;
			printf("Failed to get %s update in the firmware", get_storage_string(curStor).c_str());
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			goto Exit_Upgrade;
		}
		if (vecSelImages.size() == 1)
		{
			pImage = vecSelImages[0];
			if (pImage->Storage != 0)
			{
				if ((uint32_t)curStor != pImage->Storage)
				{
					pImage->Storage = (uint32_t)curStor;
				}
			}
		}
	}
	else
	{
		pImage = vecImages[0];
		vecSelImages.push_back(pImage);
	}

	for (i = 0; i < vecSelImages.size(); i++)
	{
		pImage = vecSelImages[i];
		pDevice->SetObject(pImage, pComm, g_pLogObject);

		if (pImage->Storage != 0)
		{
			sprintf(szInfo, "Start to download %s firmware...", get_storage_string(pImage->Storage).c_str());
			// CURSOR_MOVEUP_LINE(1);
			// CURSOR_DEL_LINE;
			printf("%s\r\n", szInfo);

			if ((pDevice->UsbType == RKUSB_MASKROM) && ((pImage->Storage == (1 << 10)) || (pImage->Storage == (1 << 11))))
			{ // sata and pcie need to switch into loader
				if (g_pLogObject)
				{
					sprintf(szInfo, "reboot loader for switching storage=%x", pImage->GetStorage());
					g_pLogObject->Record("Info:%s", szInfo);
				}
				bRet = pDevice->ResetDevice(6);
				if (!bRet)
				{
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:%s->Fail to reboot device for switching storage", __func__);
					}
					ERROR_COLOR_ATTR;
					printf("Failed to reboot into loader for switching storage");
					NORMAL_COLOR_ATTR;
					printf("\r\n");
					goto Exit_Upgrade;
				}
				bRet = pScan->Wait(dev, RKUSB_LOADER);
				if (!bRet)
				{
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:%s->Fail to wait device for switching storage", __func__);
					}
					ERROR_COLOR_ATTR;
					printf("Failed to wait loader for switching storage");
					NORMAL_COLOR_ATTR;
					printf("\r\n");
					goto Exit_Upgrade;
				}
				pDevice->VendorID = dev.usVid;
				pDevice->ProductID = dev.usPid;
				pDevice->UsbType = dev.emUsbType;
				pDevice->BcdUsb = dev.usbcdUsb;
				pDevice->LocationID = dev.uiLocationID;
				pDevice->LinkName = dev.strDevPath;
				pComm = (CRKComm *)pDevice->CommObjectPointer;
				pComm->Reset_Usb_Config(dev);
			}

			bRet = get_device_storage(pComm, (uint_8 *)&allStor, curStor);
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to get device storage", __func__);
				}
				ERROR_COLOR_ATTR;
				printf("Failed to get storage from device");
				NORMAL_COLOR_ATTR;
				printf("\r\n");
				goto Exit_Upgrade;
			}

			if ((uint32_t)curStor != pImage->GetStorage())
			{
				if (g_pLogObject)
				{
					sprintf(szInfo, "current storage=%s,switch storage=%s", get_storage_string((uint32_t)curStor).c_str(), get_storage_string(pImage->Storage).c_str());
					g_pLogObject->Record("Info:%s", szInfo);
				}
				// if (pDevice->UsbType == RKUSB_LOADER)
				// {
				// 	if (g_pLogObject)
				// 	{
				// 		g_pLogObject->Record("Error:%s->Loader did not support to switch storage", __func__);
				// 	}
				// 	ERROR_COLOR_ATTR;
				// 	printf("Loader did not support to switch storage");
				// 	NORMAL_COLOR_ATTR;
				// 	printf("\r\n");
				// 	goto Exit_Upgrade;
				// }
				bRet = switch_device_storage(pComm, (STORAGE_CODE)pImage->GetStorage());
				if (!bRet)
				{
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:%s->Fail to switch storage", __func__);
					}
					ERROR_COLOR_ATTR;
					printf("Failed to switch storage");
					NORMAL_COLOR_ATTR;
					printf("\r\n");
					goto Exit_Upgrade;
				}
				bRet = get_device_storage(pComm, (uint_8 *)&allStor, curStor);
				if (!bRet)
				{
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:%s->Fail to get device storage after switching", __func__);
					}
					ERROR_COLOR_ATTR;
					printf("Failed to get storage from device after switching");
					NORMAL_COLOR_ATTR;
					printf("\r\n");
					goto Exit_Upgrade;
				}
				if ((uint32_t)curStor != pImage->GetStorage())
				{
					sprintf(szInfo, "Fail to switch,current storage=%s,switch storage=%s", get_storage_string(curStor).c_str(), get_storage_string(pImage->Storage).c_str());
					g_pLogObject->Record("Error:%s->%s", __func__, szInfo);
					ERROR_COLOR_ATTR;
					printf("Failed to switch storage");
					NORMAL_COLOR_ATTR;
					printf("\r\n");
					goto Exit_Upgrade;
				}
			}
		}
		bRet = pUpgradeCtrlObject->Run();
		if (!bRet)
		{
			err = pDevice->Error;
			strErr = GetErrString(err);
			if (strErr.size())
				printf("%s\r\n", strErr.c_str());
			goto Exit_Upgrade;
		}
	}
	if (!bNoReset)
	{
		pDevice->ResetDevice();
	}

	bSuccess = true;
Exit_Upgrade:
	if (pUpgradeCtrlObject)
		delete pUpgradeCtrlObject;

	DestroyImages(vecImages);

	DestroyUpgradeSet(ctrlStepStruct);

	if (bSuccess)
	{
		// CURSOR_MOVEUP_LINE(1);
		// CURSOR_DEL_LINE;
		bSuccess = true;
		printf("Upgrade firmware ok.\r\n");
	}

	return bSuccess;
}
bool upgrade_loader(STRUCT_RKDEVICE_DESC &dev, char *szLoader, bool bNoReset = false, uint_32 storage = 0)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKImage *pImage = NULL;
	CRKBoot *pBoot = NULL;
	bool bRet, bSuccess = false;
	int ret, err;
	char szInfo[128];
	string strErr;
	uint_32 allStor;
	STORAGE_CODE curStor;
	printf("Loading loader...\r\n");
	pImage = new CRKImage();
	ret = pImage->ObjectInit(szLoader, false);
	if (ret != 0)
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to open loader,err=%d", __func__, ret);
		err = pImage->Error;
		delete pImage;
		ERROR_COLOR_ATTR;
		printf("Loading loader failed,err=%d!", ret);
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		strErr = GetErrString(err);
		if (strErr.size())
			printf("%s\r\n", strErr.c_str());
		return bSuccess;
	}
	else
	{
		string strTemp;
		char szText[100];
		uint_8 btMajor, btMinor;
		ENUM_RKDEVICE_TYPE type;
		pBoot = (CRKBoot *)pImage->m_bootObject;
		type = pBoot->SupportDevice;
		switch (type)
		{
		case RK27_DEVICE:
			strTemp = "RK27";
			break;
		case RK28_DEVICE:
			strTemp = "RK28";
			break;
		case RK281X_DEVICE:
			strTemp = "RK281X";
			break;
		case RKPANDA_DEVICE:
			strTemp = "RKPANDA";
			break;
		case RKCROWN_DEVICE:
			strTemp = "RKCROWN";
			break;
		case RKNANO_DEVICE:
			strTemp = "RKNANO";
			break;
		case RKSMART_DEVICE:
			strTemp = "RKSMART";
			break;
		case RKCAYMAN_DEVICE:
			strTemp = "RKCAYMAN";
			break;
		case RK29_DEVICE:
			strTemp = "RK29";
			break;
		case RK292X_DEVICE:
			strTemp = "RK292X";
			break;
		case RK30_DEVICE:
			strTemp = "RK30";
			break;
		case RK30B_DEVICE:
			strTemp = "RK30B";
			break;
		case RK31_DEVICE:
			strTemp = "RK31";
			break;
		case RK32_DEVICE:
			strTemp = "RK32";
			break;
		default:
			char szChip[7];
			memset(szChip, 0, 7 * sizeof(char));
			uint_8 *pValue = (uint_8 *)(&type);
			szChip[0] = 'R';
			szChip[1] = 'K';
			szChip[2] = (char)(pValue[3]);
			szChip[3] = (char)(pValue[2]);
			szChip[4] = (char)(pValue[1]);
			szChip[5] = (char)(pValue[0]);
			strTemp = szChip;
		}
		printf("Support Type:%s\t", strTemp.c_str());

		btMajor = ((pBoot->Version) & 0x0000FF00) >> 8;
		btMinor = ((pBoot->Version) & 0x000000FF);
		sprintf(szText, "%x.%02x", btMajor, btMinor);
		printf("Loader ver:%s\t", szText);

		sprintf(szText, "%04u-%02u-%02u %02u:%02u:%02u", ((STRUCT_RKTIME)pBoot->ReleaseTime).usYear, ((STRUCT_RKTIME)pBoot->ReleaseTime).ucMonth,
				((STRUCT_RKTIME)pBoot->ReleaseTime).ucDay, ((STRUCT_RKTIME)pBoot->ReleaseTime).ucHour, ((STRUCT_RKTIME)pBoot->ReleaseTime).ucMinute, ((STRUCT_RKTIME)pBoot->ReleaseTime).ucSecond);
		printf("Loader Time:%s\r\n", szText);

		CRKScan *pScan = NULL;
		pScan = new CRKScan();
		pScan->SetVidPid(g_usMscVid, g_usMscPid);
		pScan->AddRockusbVidPid(g_usDevNewVid, g_usDevNewPid, g_usDevOldVid, g_usDevOldPid);
		CRKComm *pComm = NULL;
		CRKDevice *pDevice = NULL;
		CRKUpgrade *pUpgradeCtrlObject = NULL;
		RK_UPGRADE_SET ctrlStepStruct;

		type = dev.emDeviceType = pBoot->SupportDevice;
		pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
		if (!bRet)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
			if (pImage)
				delete pImage;
			if (pScan)
				delete pScan;
			ERROR_COLOR_ATTR;
			printf("Creating Comm Object failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			return bSuccess;
		}

		bRet = CRKUpgrade::CreateDownloadIDBStep(type, ctrlStepStruct);

		if (!bRet)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to create step object!", __func__);
			if (pImage)
				delete pImage;
			if (pComm)
				delete pComm;
			if (pScan)
				delete pScan;
			ERROR_COLOR_ATTR;
			printf("Creating download idb step failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			return bSuccess;
		}
		// remove reset step
		ctrlStepStruct.pop_back();
		pDevice = CDeviceFactory::CreateDevice(dev);
		if (!pDevice)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to create device object!", __func__);
			if (pImage)
				delete pImage;
			if (pComm)
				delete pComm;
			if (pScan)
				delete pScan;
			DestroyUpgradeSet(ctrlStepStruct);
			ERROR_COLOR_ATTR;
			printf("Creating device object failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			return bSuccess;
		}

		pDevice->SetObject(pImage, pComm, g_pLogObject);
		pDevice->GetNewDiskSizeFlag = false;
		pDevice->NorSingleIDB = g_bNorSingleIDB;
		pDevice->CallBackPointer = ProgressInfoProc;

		pUpgradeCtrlObject = new CRKUpgrade(ctrlStepStruct, WF_DOWNLOADBOOT);

		bRet = pUpgradeCtrlObject->SetObject(pDevice, pScan);
		if (!bRet)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to create upgrade object!", __func__);
			if (pUpgradeCtrlObject)
				delete pUpgradeCtrlObject;
			if (pDevice)
				CDeviceFactory::FreeDevice(pDevice);
			else
			{
				if (pComm)
					delete pComm;
			}
			if (pScan)
				delete pScan;
			if (pImage)
				delete pImage;
			DestroyUpgradeSet(ctrlStepStruct);
			ERROR_COLOR_ATTR;
			printf("Creating upgrade object failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			return bSuccess;
		}
		pUpgradeCtrlObject->CallBackPointer = PromptInfoProc;

		printf("Start to upgrade loader...\r\n");
		if (storage > 0)
		{
			if (dev.emUsbType == RKUSB_MASKROM)
			{
				pUpgradeCtrlObject->RemoveStep(DOWNLOADBOOT_STEP);
				pUpgradeCtrlObject->RemoveStep(WAITMASKROM_STEP);
				PromptInfoProc(pDevice->LocationID, DOWNLOADBOOT_START, 0);
				if (dev.strSerial.size() == 0) {
					ret = pDevice->DownloadBoot();
					if (ret != 0)
					{
						PromptInfoProc(pDevice->LocationID, DOWNLOADBOOT_FAIL, 0);
						goto Exit_Upgrade_Loader;
					}
					PromptInfoProc(pDevice->LocationID, DOWNLOADBOOT_PASS, 0);

					PromptInfoProc(pDevice->LocationID, WAITMASKROM_START, 0);
					dev.emUsbType = RKUSB_NONE;
					bRet = pScan->Wait(dev, RKUSB_MASKROM);
					if (!bRet)
					{
						PromptInfoProc(pDevice->LocationID, WAITMASKROM_FAIL, 0);
						goto Exit_Upgrade_Loader;
					}
					PromptInfoProc(pDevice->LocationID, WAITMASKROM_PASS, 0);
					pDevice->VendorID = dev.usVid;
					pDevice->ProductID = dev.usPid;
					pDevice->UsbType = dev.emUsbType;
					pDevice->BcdUsb = dev.usbcdUsb;
					pDevice->LocationID = dev.uiLocationID;
					pDevice->LinkName = dev.strDevPath;
					pComm = (CRKComm *)pDevice->CommObjectPointer;
					pComm->Reset_Usb_Config(dev);
				}
			}
			else
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:%s->Loader did not support to switch storage", __func__);
				// CURSOR_MOVEUP_LINE(1);
				// CURSOR_DEL_LINE;
				ERROR_COLOR_ATTR;
				printf("Loader did not support to switch storage");
				NORMAL_COLOR_ATTR;
				printf("\r\n");
				goto Exit_Upgrade_Loader;
			}
			sprintf(szInfo, "Switch storage into %s", get_storage_string(storage).c_str());
			// CURSOR_MOVEUP_LINE(1);
			// CURSOR_DEL_LINE;
			printf("%s\r\n", szInfo);

			bRet = get_device_storage(pComm, (uint_8 *)&allStor, curStor);
			if (!bRet)
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:%s->Fail to get device storage", __func__);
				// CURSOR_MOVEUP_LINE(1);
				// CURSOR_DEL_LINE;
				ERROR_COLOR_ATTR;
				printf("Failed to get storage from device");
				NORMAL_COLOR_ATTR;
				printf("\r\n");
				goto Exit_Upgrade_Loader;
			}

			if (g_pLogObject)
			{
				sprintf(szInfo, "current storage=%s,switch storage=%s", get_storage_string(curStor).c_str(), get_storage_string(storage).c_str());
				g_pLogObject->Record("Info:%s", szInfo);
			}
			if (curStor != (STORAGE_CODE)storage)
			{
				bRet = switch_device_storage(pComm, (STORAGE_CODE)storage);
				if (!bRet)
				{
					if (g_pLogObject)
						g_pLogObject->Record("Error:%s->Fail to switch device storage", __func__);
					// CURSOR_MOVEUP_LINE(1);
					// CURSOR_DEL_LINE;
					ERROR_COLOR_ATTR;
					printf("Failed to switch storage");
					NORMAL_COLOR_ATTR;
					printf("\r\n");
					goto Exit_Upgrade_Loader;
				}
				bRet = get_device_storage(pComm, (uint_8 *)&allStor, curStor);
				if (!bRet)
				{
					if (g_pLogObject)
						g_pLogObject->Record("Error:%s->Fail to get device storage after switching", __func__);
					// CURSOR_MOVEUP_LINE(1);
					// CURSOR_DEL_LINE;
					ERROR_COLOR_ATTR;
					printf("Failed to get storage from device after switching");
					NORMAL_COLOR_ATTR;
					printf("\r\n");
					goto Exit_Upgrade_Loader;
				}
				if (curStor != (STORAGE_CODE)storage)
				{
					sprintf(szInfo, "Fail to switch,current storage=%s,switch storage=%s", get_storage_string((uint_32)curStor).c_str(), get_storage_string(storage).c_str());
					g_pLogObject->Record("Error:%s->%s", __func__, szInfo);
					// CURSOR_MOVEUP_LINE(1);
					// CURSOR_DEL_LINE;
					ERROR_COLOR_ATTR;
					printf("Failed to switch storage");
					NORMAL_COLOR_ATTR;
					printf("\r\n");
					goto Exit_Upgrade_Loader;
				}
			}
		}
		bRet = pUpgradeCtrlObject->Run();
		if (bRet)
		{
			if (!bNoReset)
			{
				if (g_pLogObject)
					g_pLogObject->Record("Reset device Start");
				pDevice->ResetDevice();
			}
			// CURSOR_MOVEUP_LINE(1);
			// CURSOR_DEL_LINE;
			bSuccess = true;
			printf("Upgrade loader ok.\r\n");
		}
		else
		{
			err = pDevice->Error;
			strErr = GetErrString(err);
			if (strErr.size())
				printf("%s\r\n", strErr.c_str());
		}
	Exit_Upgrade_Loader:
		if (pUpgradeCtrlObject)
			delete pUpgradeCtrlObject;

		if (pImage)
			delete pImage;

		DestroyUpgradeSet(ctrlStepStruct);
	}
	return bSuccess;
}
bool ReadFileToBuffer(char *szFile, char *&pBuffer)
{
	FILE *file = NULL;
	file = fopen(szFile, "rb");
	if (!file)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed,err=%d,can't open file: %s\r\n", __func__, errno, szFile);
		return false;
	}
	int iFileSize;
	fseek(file, 0, SEEK_END);
	iFileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	pBuffer = new char[iFileSize];
	if (!pBuffer)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed,err=%d,can't new memory,size=%d\r\n", __func__, errno, iFileSize);
		fclose(file);
		return false;
	}
	memset(pBuffer, 0, iFileSize);
	int iRead;
	iRead = fread(pBuffer, 1, iFileSize, file);
	if (iRead != iFileSize)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed,err=%d,read=%d,total=%d\r\n", __func__, errno, iRead, iFileSize);
		fclose(file);
		delete[] pBuffer;
		return false;
	}
	fclose(file);
	return true;
}

bool MakeParamBuffer(char *pParamFile, char *&pParamData)
{
	FILE *file = NULL;
	file = fopen(pParamFile, "rb");
	if (!file)
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to open %s,err=%d", __func__, pParamFile, errno);
		return false;
	}
	int iFileSize;
	fseek(file, 0, SEEK_END);
	iFileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	char *pParamBuf = NULL;
	pParamBuf = new char[iFileSize + 12];
	if (!pParamBuf)
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to new memory,err=%d", __func__, errno);
		fclose(file);
		return false;
	}
	memset(pParamBuf, 0, iFileSize + 12);
	*(uint_32 *)(pParamBuf) = 0x4D524150;

	int iRead;
	iRead = fread(pParamBuf + 8, 1, iFileSize, file);
	if (iRead != iFileSize)
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to read,err=%d,read=%d,total=%d", __func__, errno, iRead, iFileSize);
		fclose(file);
		delete[] pParamBuf;
		return false;
	}
	fclose(file);

	*(uint_32 *)(pParamBuf + 4) = iFileSize;
	*(uint_32 *)(pParamBuf + 8 + iFileSize) = CRC_32((uint_8 *)pParamBuf + 8, iFileSize);
	pParamData = pParamBuf;
	return true;
}
bool download_gpt_parameter(STRUCT_RKDEVICE_DESC &dev, char *szImage)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet = false, bSuccess = false, bGrowAlign;
	int iRet,nGptSector, nLBsector, nGptEntryLB, nMasterGptSector;
	STRUCT_FLASHINFO_CMD info;
	uint_32 uiReadCount;
	uint_32 i,nPartOffset;
	PARAM_ITEM_VECTOR vecItems;
	CONFIG_ITEM_VECTOR vecUuids;
	char *pParamBuf = NULL;
	uint_8 *readBuf=NULL;
	uint_8 *master_gpt=NULL, *backup_gpt=NULL;
	char szDateTime[100];
	string strFile;
	struct tm timeNow;
	printf("Write gpt...\r\n");
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		if (!ReadFileToBuffer(szImage, pParamBuf))
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to load %s", __func__, szImage);
			}
			printf("Read gpt info from %s failed!\r\n", szImage);
			goto exit_download_gpt_param;
		}
		bRet = parse_parameter(pParamBuf, vecItems);
		if (!bRet)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to parse parameter", __func__);
			}
			printf("Parse gpt info failed!\r\n");
			goto exit_download_gpt_param;
		}
		get_uuid_from_parameter(pParamBuf, vecUuids);
		//check partition if overlapped
		nPartOffset = 0;
		for ( i = 0; i < vecItems.size(); i++)
		{
			if (nPartOffset > vecItems[i].uiItemOffset)
				break;
			nPartOffset = vecItems[i].uiItemOffset + vecItems[i].uiItemSize;
		}
		if (i < vecItems.size()) {
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to check parameter, %s was overlapped", __func__, vecItems[i].szItemName);
			}
			printf("Failed to check gpt,because of overlapped partition at %s!\r\n", vecItems[i].szItemName);
			goto exit_download_gpt_param;
		}
		

		iRet = pComm->RKU_ReadFlashInfo((uint_8 *)&info, &uiReadCount);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to read flash info,err=%d", __func__, iRet);
			}
			printf("Read flashinfo failed!\r\n");
			goto exit_download_gpt_param;
		}
		if (info.bManufCode > 200) {
			nLBsector = info.bManufCode - 200;
			nGptEntryLB = CALC_UNIT(128, (nLBsector * 4));
			nGptSector = (3 + nGptEntryLB * 2) * nLBsector;
			nMasterGptSector = (2 + nGptEntryLB) * nLBsector;
		}
		else {
			nLBsector = 1;
			nGptEntryLB = 32;
			nGptSector = 67;
			nMasterGptSector = 34;
		}
		readBuf = new uint_8[nMasterGptSector * SECTOR_SIZE];
		memset(readBuf, 0, nMasterGptSector * SECTOR_SIZE);
		master_gpt = new uint_8[nMasterGptSector * SECTOR_SIZE];
		memset(master_gpt, 0, nMasterGptSector * SECTOR_SIZE);
		backup_gpt = new uint_8[(nGptSector-nMasterGptSector) * SECTOR_SIZE];
		memset(backup_gpt, 0, (nGptSector - nMasterGptSector) * SECTOR_SIZE);
		bRet = get_grow_align_flag(pParamBuf, bGrowAlign);
		if (bRet)
			create_gpt_buffer(master_gpt, vecItems, vecUuids, info.uiFlashSize, nLBsector, &bGrowAlign);
		else
			create_gpt_buffer(master_gpt, vecItems, vecUuids, info.uiFlashSize, nLBsector);
		memcpy(backup_gpt, master_gpt + 2 * nLBsector * SECTOR_SIZE, nGptEntryLB * nLBsector * SECTOR_SIZE);
		memcpy(backup_gpt + nGptEntryLB * nLBsector * SECTOR_SIZE, master_gpt + nLBsector * SECTOR_SIZE, nLBsector * SECTOR_SIZE);
		prepare_gpt_backup(master_gpt, backup_gpt, nLBsector);

		iRet = pComm->RKU_WriteLBA(0, nMasterGptSector, master_gpt);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to write lba for master gpt,err=%d", __func__, iRet);
			}
			printf("Writing master gpt failed!\r\n");
			goto exit_download_gpt_param;
		}
		iRet = pComm->RKU_WriteLBA(info.uiFlashSize-(nGptSector-nMasterGptSector), (nGptSector - nMasterGptSector), backup_gpt);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to write lba for backup gpt,err=%d", __func__, iRet);
			}
			printf("Writing backup gpt failed!\r\n");
			goto exit_download_gpt_param;
		}
		if (g_bReadCheckOff)
		{
			printf("Write gpt ok.\r\n");
			bSuccess = true;
			goto exit_download_gpt_param;
		}
		iRet = pComm->RKU_ReadLBA(0, nMasterGptSector, readBuf);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to read lba for master gpt,err=%d", __func__, iRet);
			}
			printf("Reading master gpt failed!\r\n");
			goto exit_download_gpt_param;
		}
		if (memcmp(master_gpt, readBuf, nMasterGptSector * SECTOR_SIZE) != 0)
		{
			printf("Checking master gpt failed!\r\n");
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to memcmp for master gpt", __func__);
				timeNow = get_now_time();
				sprintf(szDateTime, "%02d-%02d-%02d", timeNow.tm_hour, timeNow.tm_min, timeNow.tm_sec);
				strFile = g_pLogObject->LogSavePath;
				strFile += szDateTime;
				strFile += "file.bin";
				g_pLogObject->SaveBuffer(strFile, master_gpt, nMasterGptSector * SECTOR_SIZE);
				strFile = g_pLogObject->LogSavePath;
				strFile += szDateTime;
				strFile += "flash.bin";
				g_pLogObject->SaveBuffer(strFile, readBuf, nMasterGptSector * SECTOR_SIZE);
			}
			goto exit_download_gpt_param;
		}
		memset(readBuf, 0, nMasterGptSector * SECTOR_SIZE);
		iRet = pComm->RKU_ReadLBA(info.uiFlashSize- (nGptSector - nMasterGptSector), (nGptSector - nMasterGptSector), readBuf);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to read lba for backup gpt,err=%d", __func__, iRet);
			}
			printf("Reading backup gpt failed!\r\n");
			goto exit_download_gpt_param;
		}
		if (memcmp(backup_gpt, readBuf, (nGptSector - nMasterGptSector) * SECTOR_SIZE) != 0)
		{
			printf("Checking backup gpt failed!\r\n");
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to memcmp for backup gpt", __func__);
				timeNow = get_now_time();
				sprintf(szDateTime, "%02d-%02d-%02d", timeNow.tm_hour, timeNow.tm_min, timeNow.tm_sec);
				strFile = g_pLogObject->LogSavePath;
				strFile += szDateTime;
				strFile += "file.bin";
				g_pLogObject->SaveBuffer(strFile, backup_gpt, (nGptSector - nMasterGptSector) * SECTOR_SIZE);
				strFile = g_pLogObject->LogSavePath;
				strFile += szDateTime;
				strFile += "flash.bin";
				g_pLogObject->SaveBuffer(strFile, readBuf, (nGptSector - nMasterGptSector) * SECTOR_SIZE);
			}
			goto exit_download_gpt_param;
		}
		printf("Write gpt ok.\r\n");
		bSuccess = true;
	}
	else
	{
		printf("Download gpt quit,Creating comm object failed!\r\n");
	}
exit_download_gpt_param:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (pParamBuf)
		delete []pParamBuf;
	if (readBuf)
		delete []readBuf;
	if (master_gpt)
		delete []master_gpt;
	if (backup_gpt)
		delete []backup_gpt;
	return bSuccess;
}

bool download_parameter(STRUCT_RKDEVICE_DESC &dev, char *szImage, STRUCT_PARAM_ITEM item)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	char *pParam = NULL;
	if (!MakeParamBuffer(szImage, pParam))
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create param buffer", __func__);
		ERROR_COLOR_ATTR;
		printf("Creating parameter buffer failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		return false;
	}
	CRKUsbComm *pComm = NULL;
	int iRet, i;
	int iTryCount;
	uint_32 dwTotal, dwCurrent;
	bool bRet;
	string strError;
	uint_32 dwMaxReadWriteSectors = 32;
	uint_32 dwMaxReadWriteBytes = dwMaxReadWriteSectors * SECTOR_SIZE;
	uint_8 *pBuf = new uint_8[dwMaxReadWriteBytes];
	uint_8 *pRead = new uint_8[dwMaxReadWriteBytes];
	uint_32 uiBegin, uiTransferSec, uiFlashOffset = item.uiItemOffset;
	uint_32 dwTotalWritten, dwTotalRead;

	uint_32 dwTransferBytes, dwParamSize, dwParamSec, dwTotalSize;
	char szDateTime[100];
	string strFile;
	struct tm timeNow;
	if (!g_bGpt)
	{
		if (g_bFirst4Access)
		{
			uiFlashOffset += 0x2000;
		}
	}

	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		bRet = false;
		printf("Test device start...\r\n");
		do
		{
			iTryCount = 3;
			while (iTryCount > 0)
			{
				iRet = pComm->RKU_TestDeviceReady(&dwTotal, &dwCurrent);
				if ((iRet == ERR_SUCCESS) || (iRet == ERR_DEVICE_UNREADY))
				{
					break;
				}
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to test device,RetCode(%d)", __func__, iRet);
				}
				iTryCount--;
				sleep_s(1);
			}
			if (iTryCount <= 0)
			{
				strError = "Test device failed,exit download image!\r\n";
				goto Exit_DownloadParameter;
			}

			if (iRet == ERR_SUCCESS)
			{
				break;
			}
			if (dwCurrent > dwTotal)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->current is larger than total,Total=%d,Current=%d", __func__, dwTotal, dwCurrent);
				}
				strError = "Test device innormal,exit download image!\r\n";
				goto Exit_DownloadParameter;
			}
			if (dev.emUsbType == RKUSB_LOADER)
			{
				if (dwTotal == 0)
				{
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:%s->Total is zero", __func__);
					}
					strError = "Test device innormal,exit download image!\r\n";
					goto Exit_DownloadParameter;
				}
			}
			sleep_s(1);
		} while (iRet == ERR_DEVICE_UNREADY);
		dwParamSize = *((uint_32 *)(pParam + 4));
		dwParamSize += 12;
		dwParamSec = item.uiItemSize / 8;
		dwTotalSize = dwParamSize * 8;
		for (i = 0; i < 8; i++)
		{
			uiBegin = uiFlashOffset + dwParamSec * i;
			dwTotalWritten = 0;
			while (dwTotalWritten < dwParamSize)
			{
				memset(pBuf, 0, dwMaxReadWriteBytes);
				if ((dwParamSize - dwTotalWritten) >= dwMaxReadWriteBytes)
				{
					dwTransferBytes = dwMaxReadWriteBytes;
					uiTransferSec = dwMaxReadWriteSectors;
				}
				else
				{
					dwTransferBytes = dwParamSize - dwTotalWritten;
					uiTransferSec = ((dwTransferBytes % SECTOR_SIZE == 0) ? (dwTransferBytes / SECTOR_SIZE) : (dwTransferBytes / SECTOR_SIZE + 1));
				}
				memcpy(pBuf, pParam + dwTotalWritten, dwTransferBytes);
				iRet = pComm->RKU_WriteLBA(uiBegin, uiTransferSec, pBuf);
				if (ERR_SUCCESS == iRet)
				{
					dwTotalWritten += dwTransferBytes;
					uiBegin += uiTransferSec;

					CURSOR_MOVEUP_LINE(1);
					CURSOR_DEL_LINE;
					if (dwParamSize >= 1024)
						printf("Write parameter...(%d%%)\r\n", ((dwTotalWritten + dwParamSize * i) / 1024) * 100 / (dwTotalSize / 1024));
					else
						printf("Write parameter...(%d%%)\r\n", (dwTotalWritten + dwParamSize * i) * 100 / dwTotalSize);
				}
				else
				{
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:%s->Fail to write lba,Written(%d),RetCode(%d)", __func__, dwTotalWritten, iRet);
					}
					strError = "Writing data to flash failed in writing,exit download parameter!\r\n";
					goto Exit_DownloadParameter;
				}
			}
		}
		if (g_bReadCheckOff)
		{
			strError = "Download parameter ok.\r\n";
			bRet = true;
			goto Exit_DownloadParameter;
		}

		for (i = 0; i < 8; i++)
		{
			uiBegin = uiFlashOffset + dwParamSec * i;
			dwTotalRead = 0;
			while (dwTotalRead < dwParamSize)
			{
				memset(pBuf, 0, dwMaxReadWriteBytes);
				memset(pRead, 0, dwMaxReadWriteBytes);
				if ((dwParamSize - dwTotalRead) >= dwMaxReadWriteBytes)
				{
					dwTransferBytes = dwMaxReadWriteBytes;
					uiTransferSec = dwMaxReadWriteSectors;
				}
				else
				{
					dwTransferBytes = dwParamSize - dwTotalRead;
					uiTransferSec = ((dwTransferBytes % SECTOR_SIZE == 0) ? (dwTransferBytes / SECTOR_SIZE) : (dwTransferBytes / SECTOR_SIZE + 1));
				}
				memcpy(pBuf, pParam + dwTotalRead, dwTransferBytes);

				iRet = pComm->RKU_ReadLBA(uiBegin, uiTransferSec, pRead);

				if (ERR_SUCCESS == iRet)
				{
					dwTotalRead += dwTransferBytes;
					uiBegin += uiTransferSec;
				}
				else
				{
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:%s->Fail to read lba,Read(%d),RetCode(%d)", __func__, dwTotalRead, iRet);
					}
					strError = "Reading data from flash failed in checking,exit download parameter!\r\n";
					goto Exit_DownloadParameter;
				}

				if (0 != memcmp(pBuf, pRead, dwTransferBytes))
				{
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:%s->Fail to memcmp,Checked(%dK)", __func__, dwTotalRead / 1024);
					}

					if (g_pLogObject)
					{
						timeNow = get_now_time();
						sprintf(szDateTime, "%02d-%02d-%02d", timeNow.tm_hour, timeNow.tm_min, timeNow.tm_sec);
						strFile = g_pLogObject->LogSavePath;
						strFile += szDateTime;
						strFile += "file.bin";
						g_pLogObject->SaveBuffer(strFile, pBuf, dwMaxReadWriteBytes);
						strFile = g_pLogObject->LogSavePath;
						strFile += szDateTime;
						strFile += "flash.bin";
						g_pLogObject->SaveBuffer(strFile, pRead, dwMaxReadWriteBytes);
					}
					strError = "Compare data  failed in checking,exit download parameter!\r\n";
					goto Exit_DownloadParameter;
				}
				CURSOR_MOVEUP_LINE(1);
				CURSOR_DEL_LINE;
				if (dwParamSize >= 1024)
					printf("Check file...(%d%%)\r\n", ((dwTotalRead + dwParamSize * i) / 1024) * 100 / (dwTotalSize / 1024));
				else
					printf("Check file...(%d%%)\r\n", (dwTotalRead + dwParamSize * i) * 100 / dwTotalSize);
			}
		}
		iRet = pComm->RKU_ReadLBA(0, 4, pBuf);
		if (iRet == ERR_SUCCESS)
		{
			legacy_mbr *mbr = (legacy_mbr *)pBuf;
			if (mbr->signature == MSDOS_MBR_SIGNATURE)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Info:%s->destroy gpt", __func__);
				}
				memset(pBuf, 0, 8 * SECTOR_SIZE);//ufs 4k gpt header on sector 4
				pComm->RKU_WriteLBA(0, 8, pBuf); // destroy gpt
			}
		}

		strError = "Download parameter ok.\r\n";
		bRet = true;
	}
	else
	{
		printf("Download parameter quit,Creating comm object failed!\r\n");
		goto Exit_DownloadParameter;
	}
Exit_DownloadParameter:
	if (pParam)
		delete[] pParam;
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (pBuf)
		delete[] pBuf;
	if (pRead)
		delete[] pRead;
	// CURSOR_MOVEUP_LINE(1);
	// CURSOR_DEL_LINE;
	printf("%s", strError.c_str());
	return bRet;
}
bool download_image(STRUCT_RKDEVICE_DESC &dev, char *szImage, uint32_t uiOffset, uint32_t uiPartSize)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	STRUCT_FLASHINFO_CMD info;
	int iRet, nPercent, nPrevPercent;
	int iTryCount;
	long long iFileSize, dwFWSize, dwTotalWritten, dwTotalRead, dwPartSizeByte;
	uint_32 dwTotal, dwCurrent;
	uint32_t uiUbiPartSize = uiPartSize, ubiPageSize;
	bool bRet, bSuccess = false, b_ubi = false, b_jffs = false, b_dhara = false;
	FILE *file = NULL;
	string strError;
	char szFormat[256];
	uint_32 dwMaxReadWriteSectors;
	uint_32 dwMaxReadWriteBytes;
	uint_8 *pBuf = NULL;
	uint_8 *pRead = NULL;
	uint_32 uiBegin, uiRead, uiTransferSec;
	uint_8 subCode=0;
	uint_32 dwTransferBytes;
	char szDateTime[100];
	string strFile;
	struct tm timeNow;

	if (is_ubi_image(szImage, &ubiPageSize))
	{
		b_ubi = true;
		if (g_pLogObject)
		{
			g_pLogObject->Record("Info:it's a ubi image");
		}
	}
		
	else if (is_jffs_image(szImage))
	{
		b_jffs = true;
		if (g_pLogObject)
		{
			g_pLogObject->Record("Info:it's a jffs image");
		}
	}
	else if (is_dhara_image(szImage))
	{
		b_dhara = true;
		if (g_pLogObject)
		{
			g_pLogObject->Record("Info:it's a dhara image");
		}
	}
		
	if ((!g_bGpt)&&(!g_bEnv))
	{
		if (g_bFirst4Access)
			uiOffset += 0x2000;
	}

	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		printf("Test device start...\r\n");
		do
		{
			iTryCount = 3;
			while (iTryCount > 0)
			{
				iRet = pComm->RKU_TestDeviceReady(&dwTotal, &dwCurrent);
				if ((iRet == ERR_SUCCESS) || (iRet == ERR_DEVICE_UNREADY))
				{
					break;
				}
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to test device,RetCode(%d)", __func__, iRet);
				}
				iTryCount--;
				sleep_s(1);
			}
			if (iTryCount <= 0)
			{
				strError = "Test device failed,exit download image!\r\n";
				goto Exit_DownloadImage;
			}

			if (iRet == ERR_SUCCESS)
			{
				break;
			}
			if (dwCurrent > dwTotal)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->current is larger than total,Total=%d,Current=%d", __func__, dwTotal, dwCurrent);
				}
				strError = "Test device innormal,exit download image!\r\n";
				goto Exit_DownloadImage;
			}
			if (dev.emUsbType == RKUSB_LOADER)
			{
				if (dwTotal == 0)
				{
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:%s->Total is zero", __func__);
					}
					strError = "Test device innormal,exit download image!\r\n";
					goto Exit_DownloadImage;
				}
			}
			sleep_s(1);
		} while (iRet == ERR_DEVICE_UNREADY);
		dwTotal = dwCurrent = 0;
		if (g_uiForceBand == 0)
		{
			if (g_bUsbDriver4m)
				subCode = TU_USBTRANSFER4M_SUBCODE;
			iRet = pComm->RKU_TestDeviceReady(&dwTotal, &dwCurrent,subCode);
			if (iRet != ERR_SUCCESS)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Failt to get lba times,err=%d", __func__, iRet);
				}
				strError = "Download image failed,ReadLBATimes failed\r\n";
				goto Exit_DownloadImage;
			}
		}
		else
			dwCurrent = g_uiForceBand;

	#ifndef _WIN32
		if (dwCurrent>8)
			dwCurrent = 8;//limit 4m transfer buffer on non-windows
	#endif
		dwMaxReadWriteSectors = 32 * (1 << dwCurrent);
		dwMaxReadWriteBytes = dwMaxReadWriteSectors * SECTOR_SIZE;
		pBuf = new uint8_t[dwMaxReadWriteBytes];
		pRead = new uint8_t[dwMaxReadWriteBytes];
		if ((!pBuf) || (!pRead))
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to new memory,err=%d", __func__, errno);
			}
			strError = "Download image failed,can't new memory\r\n";
			goto Exit_DownloadImage;
		}
		file = fopen(szImage, "rb");
		if (!file)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to open %s,err=%d", __func__, szImage, errno);
			}
			sprintf(szFormat, "Download image failed,err=%d,can't open file: %s\r\n", errno, szImage);
			strError = szFormat;
			goto Exit_DownloadImage;
		}
		iRet = pComm->RKU_ReadFlashInfo((uint_8 *)&info, NULL);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to read flash info,err=%d", __func__, iRet);
			}
			strError = "Download image failed,ReadFlashInfo failed\r\n";
			goto Exit_DownloadImage;
		}
		if (uiOffset >= info.uiFlashSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Partition offset is larger than flash size, partition_offset=0x%08X,flash_size=0x%08X", __func__, uiOffset, info.uiFlashSize);
			}
			strError = "Partition offset is larger than flash\r\n";
			goto Exit_DownloadImage;
		}
		if (uiPartSize == 0xFFFFFFFF)
		{
			uiPartSize = info.uiFlashSize - uiOffset;
			if (g_bGpt)
			{
				if (info.uiFlashSize >= 0x800000)
				{ // exclude backup gpt
					uiPartSize -= 64;
				}
				else
					uiPartSize -= 33;
			}
		}
		if ((uiOffset + uiPartSize) > info.uiFlashSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Partition ending is larger than flash size, partition_end=0x%08X,flash_size=0x%08X", __func__, uiOffset + uiPartSize, info.uiFlashSize);
			}
			strError = "Partition ending is larger than flash\r\n";
			goto Exit_DownloadImage;
		}

		fseek(file, 0, SEEK_END);
		iFileSize = ftell(file);
		fseek(file, 0, SEEK_SET);
		nPercent = nPrevPercent = 0;
		dwFWSize = iFileSize;
		dwPartSizeByte = (long long)uiPartSize * SECTOR_SIZE;
		dwTotalWritten = 0;
		uiBegin = uiOffset;
		if (iFileSize > dwPartSizeByte)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->size of file is larger than partition,file=%lld,partition=%lld", __func__, iFileSize, dwPartSizeByte);
			}
			sprintf(szFormat, "Download image failed,partition too small,file=%lld,partition=%lld\r\n", iFileSize, dwPartSizeByte);
			strError = szFormat;
			goto Exit_DownloadImage;
		}
		if (b_ubi)
		{
			if ((info.bPageSize * SECTOR_SIZE) != ubiPageSize)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s-->Fail to check UBI page size,image:%u,device:%u", __func__, ubiPageSize / SECTOR_SIZE, info.bPageSize);
				}
				strError = "Download image failed,check ubi pagesize failure\r\n";
				goto Exit_DownloadImage;
			}

			bRet = erase_ubi_block(pComm, uiOffset, uiUbiPartSize);
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to erase ubi partition", __func__);
				}
				strError = "Download image failed,erase ubi partition failure\r\n";
				goto Exit_DownloadImage;
			}
		}
		else if (b_jffs)
		{
			bRet = erase_partition(pComm, uiOffset, uiPartSize, 256);
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to erase jffs partition", __func__);
				}
				strError = "Download image failed,erase jffs partition failure\r\n";
				goto Exit_DownloadImage;
			}
		}
		else if (b_dhara)
		{
			bRet = erase_ubi_block(pComm, uiOffset, uiUbiPartSize);
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to erase dhara partition", __func__);
				}
				strError = "Download image failed,erase dhara partition failure\r\n";
				goto Exit_DownloadImage;
			}
		}

		while (dwTotalWritten < dwFWSize)
		{
			memset(pBuf, 0, dwMaxReadWriteBytes);
			if ((dwFWSize - dwTotalWritten) >= dwMaxReadWriteBytes)
			{
				dwTransferBytes = dwMaxReadWriteBytes;
				uiTransferSec = dwMaxReadWriteSectors;
			}
			else
			{
				dwTransferBytes = dwFWSize - dwTotalWritten;
				uiTransferSec = ((dwTransferBytes % SECTOR_SIZE == 0) ? (dwTransferBytes / SECTOR_SIZE) : (dwTransferBytes / SECTOR_SIZE + 1));
			}
			uiRead = fread(pBuf, 1, dwTransferBytes, file);
			if (uiRead != dwTransferBytes)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to read data from file in write step,err=%d,read=%d,ret=%d", __func__, errno, dwTransferBytes, uiRead);
				}
				strError = "Read data from file failed in writing,exit download image!\r\n";
				goto Exit_DownloadImage;
			}
			
			iRet = pComm->RKU_WriteLBA(uiBegin, uiTransferSec, pBuf);
			if (ERR_SUCCESS == iRet)
			{
				dwTotalWritten += dwTransferBytes;
				uiBegin += uiTransferSec;

				nPercent = ((double)dwTotalWritten / dwFWSize) * 100;
				if (nPercent > nPrevPercent)
				{
					CURSOR_MOVEUP_LINE(1);
					CURSOR_DEL_LINE;
					printf("Write file...(%d%%)\r\n", nPercent);
					nPrevPercent = nPercent;
				}
			}
			else
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to write lba,Written(%dK),RetCode(%d)", __func__, dwTotalWritten / 1024, iRet);
				}
				strError = "Writing data to flash failed in writing,exit download image!\r\n";
				goto Exit_DownloadImage;
			}
		}
		if (g_bReadCheckOff)
		{
			strError = "Download image ok.\r\n";
			bSuccess = true;
			goto Exit_DownloadImage;
		}
		uiBegin = uiOffset, dwTotalRead = 0;
		nPrevPercent = nPercent = 0;
		fseek(file, 0, SEEK_SET);

		while (dwTotalRead < dwFWSize)
		{
			memset(pBuf, 0, dwMaxReadWriteBytes);
			memset(pRead, 0, dwMaxReadWriteBytes);
			if ((dwFWSize - dwTotalRead) >= dwMaxReadWriteBytes)
			{
				dwTransferBytes = dwMaxReadWriteBytes;
				uiTransferSec = dwMaxReadWriteSectors;
			}
			else
			{
				dwTransferBytes = dwFWSize - dwTotalRead;
				uiTransferSec = ((dwTransferBytes % SECTOR_SIZE == 0) ? (dwTransferBytes / SECTOR_SIZE) : (dwTransferBytes / SECTOR_SIZE + 1));
			}
			uiRead = fread(pBuf, 1, dwTransferBytes, file);
			if (uiRead != dwTransferBytes)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to read data from file in check step,err=%d,read=%d,ret=%d", __func__, errno, dwTransferBytes, uiRead);
				}
				strError = "Read data from file failed in checking,exit download image!\r\n";
				goto Exit_DownloadImage;
			}
			iRet = pComm->RKU_ReadLBA(uiBegin, uiTransferSec, pRead);

			if (ERR_SUCCESS == iRet)
			{
				dwTotalRead += dwTransferBytes;
				uiBegin += uiTransferSec;
			}
			else
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to read lba,Read(%dK),RetCode(%d)", __func__, dwTotalRead / 1024, iRet);
				}
				strError = "Reading data from flash failed in checking,exit download image!\r\n";
				goto Exit_DownloadImage;
			}

			if (0 != memcmp(pBuf, pRead, dwTransferBytes))
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to memcmp,Checked(%dK)", __func__, dwTotalRead / 1024);
				}

				if (g_pLogObject)
				{
					timeNow = get_now_time();
					sprintf(szDateTime, "%02d-%02d-%02d", timeNow.tm_hour, timeNow.tm_min, timeNow.tm_sec);
					strFile = g_pLogObject->LogSavePath;
					strFile += szDateTime;
					strFile += "file.bin";
					g_pLogObject->SaveBuffer(strFile, pBuf, dwMaxReadWriteBytes);
					strFile = g_pLogObject->LogSavePath;
					strFile += szDateTime;
					strFile += "flash.bin";
					g_pLogObject->SaveBuffer(strFile, pRead, dwMaxReadWriteBytes);
				}
				strError = "Compare data  failed in checking,exit download image!\r\n";
				goto Exit_DownloadImage;
			}
			nPercent = ((double)dwTotalRead / dwFWSize) * 100;
			if (nPercent > nPrevPercent)
			{
				CURSOR_MOVEUP_LINE(1);
				CURSOR_DEL_LINE;
				printf("Check file...(%d%%)\r\n", nPercent);
				nPrevPercent = nPercent;
			}
		}
		strError = "Download image ok.\r\n";
		bSuccess = true;
	}
	else
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to create comm object", __func__);
		}
		printf("Download image quit,Creating comm object failed!\r\n");
		return false;
	}
Exit_DownloadImage:

	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (file)
		fclose(file);
	if (pBuf)
		delete[] pBuf;
	if (pRead)
		delete[] pRead;
	// CURSOR_MOVEUP_LINE(1);
	// CURSOR_DEL_LINE;
	printf("%s", strError.c_str());
	return bSuccess;
}
bool download_sparse_image(STRUCT_RKDEVICE_DESC &dev, char *szFile, uint_32 uiOffset, uint_32 uiPartSize)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	STRUCT_FLASHINFO_CMD info;
	CRKUsbComm *pComm = NULL;
	int iRet, iRead, i;
	int curChunk, nPercent, nPrevPercent;
	uint_64 iFileSize, iTotalWrite, iTotalRead;
	sparse_header header;
	chunk_header chunk;
	bool bRet, bSuccess = false;
	FILE *file = NULL;
	string strError;
	char szFormat[256];
	uint_32 dwMaxReadWriteSectors;
	uint_32 dwMaxReadWriteBytes;
	uint_8 *pBuf = NULL;
	uint_8 *pRead = NULL;
	uint_8 subCode=0;
	uint_32 uiBegin, uiTransferSec, uiSize, dwTotal, dwCurrent;
	uint_64 dwChunkDataSize;
	uint_32 dwTransferBytes, dwFillByte, dwCrc;
	char szDateTime[100];
	string strFile;
	struct tm timeNow;

	if ((!g_bGpt)&&(!g_bEnv))
	{
		if (g_bFirst4Access)
			uiOffset += 0x2000;
	}

	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		dwTotal = dwCurrent = 0;
		if (g_uiForceBand == 0)
		{
			if (g_bUsbDriver4m)
				subCode = TU_USBTRANSFER4M_SUBCODE;
			iRet = pComm->RKU_TestDeviceReady(&dwTotal, &dwCurrent,subCode);
			if (iRet != ERR_SUCCESS)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to get lba times,err=%d", __func__, iRet);
				}
				strError = "Download Sparse image failed,ReadLBATimes failed\r\n";
				goto Exit_DownloadSparseImage;
			}
		}
		else
			dwCurrent = g_uiForceBand;
	#ifndef _WIN32
		if (dwCurrent>8)
			dwCurrent = 8;//limit 4m transfer buffer on non-windows
	#endif
		dwMaxReadWriteSectors = 32 * (1 << dwCurrent);
		dwMaxReadWriteBytes = dwMaxReadWriteSectors * SECTOR_SIZE;
		pBuf = new uint8_t[dwMaxReadWriteBytes];
		pRead = new uint8_t[dwMaxReadWriteBytes];
		if ((!pBuf) || (!pRead))
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to new memory,err=%d", __func__, errno);
			}
			strError = "Download Sparse image failed,can't new memory\r\n";
			goto Exit_DownloadSparseImage;
		}

		file = fopen(szFile, "rb");
		if (!file)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to open %s,err=%d", __func__, szFile, errno);
			}
			printf("%s failed, err=%d, can't open file: %s\r\n", __func__, errno, szFile);
			sprintf(szFormat, "Download Sparse image failed,err=%d,can't open file: %s\r\n", errno, szFile);
			strError = szFormat;
			goto Exit_DownloadSparseImage;
		}
		fseek(file, 0, SEEK_SET);
		iRead = fread(&header, 1, sizeof(header), file);
		if (iRead != sizeof(sparse_header))
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to read sparse header,file=%s,err=%d", __func__, szFile, errno);
			}
			sprintf(szFormat, "Download Sparse image failed,err=%d,read file: %s\r\n", errno, szFile);
			strError = szFormat;
			goto Exit_DownloadSparseImage;
		}
		iFileSize = header.blk_sz * (uint_64)header.total_blks;
		uiBegin = uiOffset;
		nPercent = nPrevPercent = 0;
		iTotalWrite = 0;
		curChunk = 0;
		uiSize = iFileSize / SECTOR_SIZE;
		iRet = pComm->RKU_ReadFlashInfo((uint_8 *)&info, NULL);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to read flash info,err=%d", __func__, iRet);
			}
			strError = "Download Sparse image failed,ReadFlashInfo failed\r\n";
			goto Exit_DownloadSparseImage;
		}

		if (uiOffset >= info.uiFlashSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Partition offset is larger than flash size, partition_offset=0x%08X,flash_size=0x%08X", __func__, uiOffset, info.uiFlashSize);
			}
			strError = "Partition offset is larger than flash\r\n";
			goto Exit_DownloadSparseImage;
		}
		if (uiPartSize == 0xFFFFFFFF)
		{
			uiPartSize = info.uiFlashSize - uiOffset;
			if (g_bGpt)
			{
				if (info.uiFlashSize >= 0x800000)
				{ // exclude backup gpt
					uiPartSize -= 64;
				}
				else
					uiPartSize -= 33;
			}
		}
		if ((uiOffset + uiPartSize) > info.uiFlashSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Partition ending is larger than flash size, partition_end=0x%08X,flash_size=0x%08X", __func__, uiOffset + uiPartSize, info.uiFlashSize);
			}
			strError = "Partition ending is larger than flash\r\n";
			goto Exit_DownloadSparseImage;
		}
		if (uiSize > uiPartSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->size of sparse file is larger than partition size,sparse=%d,partition=%d", __func__, uiSize, uiPartSize);
			}
			sprintf(szFormat, "Download Sparse image failed,partition too small,sparse=%d,partition=%d\r\n", uiSize, uiPartSize);
			strError = szFormat;
			goto Exit_DownloadSparseImage;
		}
		if (g_pLogObject)
		{
			g_pLogObject->Record("Info:%s->erase start, unsparse=0x%08x,partsize=0x%08x", __func__, uiSize, uiPartSize);
		}
		bRet = erase_partition(pComm, uiBegin, uiSize);
		if (!bRet)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to erase partition", __func__);
			}
			printf("%s failed, erase partition error\r\n", __func__);
			strError = "Download Sparse image failed,Erase Partition failed\r\n";
			goto Exit_DownloadSparseImage;
		}
		// erase last 32k to destroy verity meta
		bRet = erase_partition(pComm, uiBegin + uiPartSize - 64, 64);
		if (!bRet)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to erase verity part", __func__);
			}
			printf("%s failed, erase partition last error\r\n", __func__);
			strError = "Download Sparse image failed,Erase Partition last failed\r\n";
			goto Exit_DownloadSparseImage;
		}
		if (g_pLogObject)
		{
			g_pLogObject->Record("Info:%s->start to write sparse, total_chunk=%d", __func__, header.total_chunks);
		}
		while (curChunk < header.total_chunks)
		{
			if (!EatSparseChunk(file, chunk))
			{
				goto Exit_DownloadSparseImage;
			}
			curChunk++;
			switch (chunk.chunk_type)
			{
			case CHUNK_TYPE_RAW:
				dwChunkDataSize = chunk.total_sz - sizeof(chunk_header);
				while (dwChunkDataSize)
				{
					memset(pBuf, 0, dwMaxReadWriteBytes);
					if (dwChunkDataSize >= dwMaxReadWriteBytes)
					{
						dwTransferBytes = dwMaxReadWriteBytes;
						uiTransferSec = dwMaxReadWriteSectors;
					}
					else
					{
						dwTransferBytes = dwChunkDataSize;
						uiTransferSec = ((dwTransferBytes % SECTOR_SIZE == 0) ? (dwTransferBytes / SECTOR_SIZE) : (dwTransferBytes / SECTOR_SIZE + 1));
					}
					if (!EatSparseData(file, pBuf, dwTransferBytes))
					{
						goto Exit_DownloadSparseImage;
					}
					iRet = pComm->RKU_WriteLBA(uiBegin, uiTransferSec, pBuf);
					if (ERR_SUCCESS == iRet)
					{
						dwChunkDataSize -= dwTransferBytes;
						iTotalWrite += dwTransferBytes;
						uiBegin += uiTransferSec;
					}
					else
					{
						if (g_pLogObject)
						{
							g_pLogObject->Record("Error:%s->Fail to write lba,Written(%d),RetCode(%d)", __func__, iTotalWrite, iRet);
						}
						sprintf(szFormat, "Download Sparse image failed,WriteLBA failed,err=%d\r\n", iRet);
						strError = szFormat;
						goto Exit_DownloadSparseImage;
					}
					nPercent = ((double)iTotalWrite / iFileSize) * 100;
					if (nPercent > nPrevPercent)
					{
						if (nPrevPercent == 0)
							printf("Write LBA from file ...(%d%%)\r\n", nPercent);
						else
						{
							CURSOR_MOVEUP_LINE(1);
							CURSOR_DEL_LINE;
							printf("Write LBA from file ...(%d%%)\r\n", nPercent);
						}
						nPrevPercent = nPercent;
					}
				}
				break;
			case CHUNK_TYPE_FILL:
				dwChunkDataSize = (uint_64)chunk.chunk_sz * header.blk_sz;
				if (!EatSparseData(file, (uint_8 *)&dwFillByte, 4))
				{
					goto Exit_DownloadSparseImage;
				}
				if (dwFillByte == 0)
				{
					iTotalWrite += dwChunkDataSize;
					uiTransferSec = ((dwChunkDataSize % SECTOR_SIZE == 0) ? (dwChunkDataSize / SECTOR_SIZE) : (dwChunkDataSize / SECTOR_SIZE + 1));
					uiBegin += uiTransferSec;
					nPercent = ((double)iTotalWrite / iFileSize) * 100;

					if (nPercent > nPrevPercent)
					{
						if (nPrevPercent == 0)
							printf("Write LBA from file ...(%d%%)\r\n", nPercent);
						else
						{
							CURSOR_MOVEUP_LINE(1);
							CURSOR_DEL_LINE;
							printf("Write LBA from file ...(%d%%)\r\n", nPercent);
						}
						nPrevPercent = nPercent;
					}
				}
				else
				{
					while (dwChunkDataSize)
					{
						memset(pBuf, 0, dwMaxReadWriteBytes);
						if (dwChunkDataSize >= dwMaxReadWriteBytes)
						{
							dwTransferBytes = dwMaxReadWriteBytes;
							uiTransferSec = dwMaxReadWriteSectors;
						}
						else
						{
							dwTransferBytes = dwChunkDataSize;
							uiTransferSec = ((dwTransferBytes % SECTOR_SIZE == 0) ? (dwTransferBytes / SECTOR_SIZE) : (dwTransferBytes / SECTOR_SIZE + 1));
						}
						for (i = 0; i < dwTransferBytes / 4; i++)
						{
							*(uint_32 *)(pBuf + i * 4) = dwFillByte;
						}
						iRet = pComm->RKU_WriteLBA(uiBegin, uiTransferSec, pBuf);
						if (ERR_SUCCESS == iRet)
						{
							dwChunkDataSize -= dwTransferBytes;
							iTotalWrite += dwTransferBytes;
							uiBegin += uiTransferSec;
						}
						else
						{
							if (g_pLogObject)
							{
								g_pLogObject->Record("Error:%s->Fail to write lba,Written(%d),RetCode(%d)", __func__, iTotalWrite, iRet);
							}
							sprintf(szFormat, "Download Sparse image failed,WriteLBA failed,err=%d\r\n", iRet);
							strError = szFormat;
							goto Exit_DownloadSparseImage;
						}
						nPercent = ((double)iTotalWrite / iFileSize) * 100;
						if (nPercent > nPrevPercent)
						{
							if (nPrevPercent == 0)
								printf("Write LBA from file ...(%d%%)\r\n", nPercent);
							else
							{
								CURSOR_MOVEUP_LINE(1);
								CURSOR_DEL_LINE;
								printf("Write LBA from file ...(%d%%)\r\n", nPercent);
							}
							nPrevPercent = nPercent;
						}
					}
				}

				break;
			case CHUNK_TYPE_DONT_CARE:
				dwChunkDataSize = (uint_64)chunk.chunk_sz * header.blk_sz;
				iTotalWrite += dwChunkDataSize;
				uiTransferSec = ((dwChunkDataSize % SECTOR_SIZE == 0) ? (dwChunkDataSize / SECTOR_SIZE) : (dwChunkDataSize / SECTOR_SIZE + 1));
				uiBegin += uiTransferSec;
				nPercent = ((double)iTotalWrite / iFileSize) * 100;
				if (nPercent > nPrevPercent)
				{
					if (nPrevPercent == 0)
						printf("Write LBA from file ...(%d%%)\r\n", nPercent);
					else
					{
						CURSOR_MOVEUP_LINE(1);
						CURSOR_DEL_LINE;
						printf("Write LBA from file ...(%d%%)\r\n", nPercent);
					}
					nPrevPercent = nPercent;
				}
				break;
			case CHUNK_TYPE_CRC32:
				EatSparseData(file, (uint_8 *)&dwCrc, 4);
				break;
			}
		}
		if (g_bReadCheckOff)
		{
			strError = "Download image ok.\r\n";
			bSuccess = true;
			goto Exit_DownloadSparseImage;
		}
		uiBegin = uiOffset, iTotalRead = 0;
		nPercent = nPrevPercent = 0;
		curChunk = 0;
		fseek(file, sizeof(header), SEEK_SET);
		while (curChunk < header.total_chunks)
		{
			if (!EatSparseChunk(file, chunk))
			{
				goto Exit_DownloadSparseImage;
			}
			curChunk++;
			switch (chunk.chunk_type)
			{
			case CHUNK_TYPE_RAW:
				dwChunkDataSize = chunk.total_sz - sizeof(chunk_header);
				while (dwChunkDataSize)
				{
					memset(pBuf, 0, dwMaxReadWriteBytes);
					if (dwChunkDataSize >= dwMaxReadWriteBytes)
					{
						dwTransferBytes = dwMaxReadWriteBytes;
						uiTransferSec = dwMaxReadWriteSectors;
					}
					else
					{
						dwTransferBytes = dwChunkDataSize;
						uiTransferSec = ((dwTransferBytes % SECTOR_SIZE == 0) ? (dwTransferBytes / SECTOR_SIZE) : (dwTransferBytes / SECTOR_SIZE + 1));
					}
					if (!EatSparseData(file, pBuf, dwTransferBytes))
					{
						goto Exit_DownloadSparseImage;
					}
					iRet = pComm->RKU_ReadLBA(uiBegin, uiTransferSec, pRead);
					if (ERR_SUCCESS == iRet)
					{
						dwChunkDataSize -= dwTransferBytes;
						iTotalRead += dwTransferBytes;
						uiBegin += uiTransferSec;
					}
					else
					{
						if (g_pLogObject)
						{
							g_pLogObject->Record("Error:%s->Fail to read lba,Read(%d),RetCode(%d)", __func__, iTotalRead, iRet);
						}
						sprintf(szFormat, "Download Sparse image failed,ReadLBA failed,err=%d\r\n", iRet);
						strError = szFormat;
						goto Exit_DownloadSparseImage;
					}
					if (0 != memcmp(pBuf, pRead, dwTransferBytes))
					{
						if (g_pLogObject)
						{
							g_pLogObject->Record("Error:%s->Fail to memcmp,Checked(%dK)", __func__, iTotalRead / 1024);
						}

						if (g_pLogObject)
						{
							timeNow = get_now_time();
							sprintf(szDateTime, "%02d-%02d-%02d", timeNow.tm_hour, timeNow.tm_min, timeNow.tm_sec);
							strFile = g_pLogObject->LogSavePath;
							strFile += szDateTime;
							strFile += "file.bin";
							g_pLogObject->SaveBuffer(strFile, pBuf, dwMaxReadWriteBytes);
							strFile = g_pLogObject->LogSavePath;
							strFile += szDateTime;
							strFile += "flash.bin";
							g_pLogObject->SaveBuffer(strFile, pRead, dwMaxReadWriteBytes);
						}
						strError = "Compare data  failed in checking,exit download sparse image!\r\n";
						goto Exit_DownloadSparseImage;
					}

					nPercent = ((double)iTotalRead / iFileSize) * 100;
					if (nPercent > nPrevPercent)
					{
						if (nPrevPercent == 0)
							printf("Check file ...(%d%%)\r\n", nPercent);
						else
						{
							CURSOR_MOVEUP_LINE(1);
							CURSOR_DEL_LINE;
							printf("Check file ...(%d%%)\r\n", nPercent);
						}
						nPrevPercent = nPercent;
					}
				}
				break;
			case CHUNK_TYPE_FILL:
				dwChunkDataSize = (uint_64)chunk.chunk_sz * header.blk_sz;
				if (!EatSparseData(file, (uint_8 *)&dwFillByte, 4))
				{
					goto Exit_DownloadSparseImage;
				}
				if (dwFillByte == 0)
				{
					iTotalRead += dwChunkDataSize;
					uiTransferSec = ((dwChunkDataSize % SECTOR_SIZE == 0) ? (dwChunkDataSize / SECTOR_SIZE) : (dwChunkDataSize / SECTOR_SIZE + 1));
					uiBegin += uiTransferSec;

					nPercent = ((double)iTotalRead / iFileSize) * 100;
					if (nPercent > nPrevPercent)
					{
						if (nPrevPercent == 0)
							printf("Check file ...(%d%%)\r\n", nPercent);
						else
						{
							CURSOR_MOVEUP_LINE(1);
							CURSOR_DEL_LINE;
							printf("Check file ...(%d%%)\r\n", nPercent);
						}
						nPrevPercent = nPercent;
					}
				}
				else
				{
					while (dwChunkDataSize)
					{
						memset(pBuf, 0, dwMaxReadWriteBytes);
						if (dwChunkDataSize >= dwMaxReadWriteBytes)
						{
							dwTransferBytes = dwMaxReadWriteBytes;
							uiTransferSec = dwMaxReadWriteSectors;
						}
						else
						{
							dwTransferBytes = dwChunkDataSize;
							uiTransferSec = ((dwTransferBytes % SECTOR_SIZE == 0) ? (dwTransferBytes / SECTOR_SIZE) : (dwTransferBytes / SECTOR_SIZE + 1));
						}
						for (i = 0; i < dwTransferBytes / 4; i++)
						{
							*(uint_32 *)(pBuf + i * 4) = dwFillByte;
						}
						iRet = pComm->RKU_ReadLBA(uiBegin, uiTransferSec, pBuf);
						if (ERR_SUCCESS == iRet)
						{
							dwChunkDataSize -= dwTransferBytes;
							iTotalRead += dwTransferBytes;
							uiBegin += uiTransferSec;
						}
						else
						{
							if (g_pLogObject)
							{
								g_pLogObject->Record("Error:%s->Fail to read lba,Read(%d),RetCode(%d)", __func__, iTotalRead, iRet);
							}
							sprintf(szFormat, "Download Sparse image failed,ReadLBA failed,err=%d\r\n", iRet);
							strError = szFormat;
							goto Exit_DownloadSparseImage;
						}
						if (0 != memcmp(pBuf, pRead, dwTransferBytes))
						{
							if (g_pLogObject)
							{
								g_pLogObject->Record("Error:%s->Fail to memcmp,Checked(%dK)", __func__, iTotalRead / 1024);
							}

							if (g_pLogObject)
							{
								timeNow = get_now_time();
								sprintf(szDateTime, "%02d-%02d-%02d", timeNow.tm_hour, timeNow.tm_min, timeNow.tm_sec);
								strFile = g_pLogObject->LogSavePath;
								strFile += szDateTime;
								strFile += "file.bin";
								g_pLogObject->SaveBuffer(strFile, pBuf, dwMaxReadWriteBytes);
								strFile = g_pLogObject->LogSavePath;
								strFile += szDateTime;
								strFile += "flash.bin";
								g_pLogObject->SaveBuffer(strFile, pRead, dwMaxReadWriteBytes);
							}
							strError = "Compare data  failed in checking,exit download sparse image!\r\n";
							goto Exit_DownloadSparseImage;
						}
						nPercent = ((double)iTotalRead / iFileSize) * 100;
						if (nPercent > nPrevPercent)
						{
							if (nPrevPercent == 0)
								printf("Check file ...(%d%%)\r\n", nPercent);
							else
							{
								CURSOR_MOVEUP_LINE(1);
								CURSOR_DEL_LINE;
								printf("Check file ...(%d%%)\r\n", nPercent);
							}
							nPrevPercent = nPercent;
						}
					}
				}
				break;
			case CHUNK_TYPE_DONT_CARE:
				dwChunkDataSize = (uint_64)chunk.chunk_sz * header.blk_sz;
				iTotalRead += dwChunkDataSize;
				uiTransferSec = ((dwChunkDataSize % SECTOR_SIZE == 0) ? (dwChunkDataSize / SECTOR_SIZE) : (dwChunkDataSize / SECTOR_SIZE + 1));
				uiBegin += uiTransferSec;
				nPercent = ((double)iTotalRead / iFileSize) * 100;
				if (nPercent > nPrevPercent)
				{
					if (nPrevPercent == 0)
						printf("Check file ...(%d%%)\r\n", nPercent);
					else
					{
						CURSOR_MOVEUP_LINE(1);
						CURSOR_DEL_LINE;
						printf("Check file ...(%d%%)\r\n", nPercent);
					}
					nPrevPercent = nPercent;
				}
				break;
			case CHUNK_TYPE_CRC32:
				EatSparseData(file, (uint_8 *)&dwCrc, 4);
				break;
			}
		}

		strError = "Download image ok.\r\n";
		bSuccess = true;
	}
	else
	{
		printf("Download image quit,Creating comm object failed!\r\n");
		return false;
	}
Exit_DownloadSparseImage:

	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (file)
		fclose(file);
	if (pBuf)
		delete[] pBuf;
	if (pRead)
		delete[] pRead;
	CURSOR_MOVEUP_LINE(1);
	CURSOR_DEL_LINE;
	printf("%s", strError.c_str());
	return bSuccess;
}

bool download_boot(STRUCT_RKDEVICE_DESC &dev, char *szLoader)
{
	if (!check_device_type(dev, RKUSB_MASKROM))
		return false;
		
	if (dev.strSerial.size()) {
		printf("Download boot...skip\r\n");
		return true;
	}

	CRKImage *pImage = NULL;
	CRKBoot *pBoot = NULL;
	bool bRet, bSuccess = false;
	int iRet, err;
	string strErr;
	pImage = new CRKImage();
	iRet = pImage->ObjectInit(szLoader, false);
	if (iRet != 0)
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to open loader,err=%d!", __func__, iRet);
		err = pImage->Error;
		delete pImage;
		ERROR_COLOR_ATTR;
		printf("Open loader failed(err=%d),exit download boot!", iRet);
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		strErr = GetErrString(err);
		if (strErr.size())
			printf("%s\r\n", strErr.c_str());

		return bSuccess;
	}
	else
	{
		ENUM_RKDEVICE_TYPE type;
		pBoot = (CRKBoot *)pImage->m_bootObject;
		type = pBoot->SupportDevice;

		CRKComm *pComm = NULL;
		CRKDevice *pDevice = NULL;

		dev.emDeviceType = pBoot->SupportDevice;
		pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
		if (!bRet)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
			if (pImage)
				delete pImage;
			ERROR_COLOR_ATTR;
			printf("Creating Comm Object failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");

			return bSuccess;
		}

		pDevice = CDeviceFactory::CreateDevice(dev);
		if (!pDevice)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to create device object!", __func__);
			if (pImage)
				delete pImage;
			if (pComm)
				delete pComm;
			ERROR_COLOR_ATTR;
			printf("Creating device object failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");

			return bSuccess;
		}

		pDevice->SetObject(pImage, pComm, g_pLogObject);

		printf("Download boot...\r\n");
		iRet = pDevice->DownloadBoot();

		CURSOR_MOVEUP_LINE(1);
		CURSOR_DEL_LINE;
		if (iRet == 0)
		{
			pComm->Reset_Usb_Device();
			CRKScan *pScan = NULL;
			pScan = new CRKScan();
			if (pScan)
			{
				dev.emUsbType = RKUSB_NONE;
				pScan->SetVidPid(g_usMscVid, g_usMscPid);
				pScan->AddRockusbVidPid(g_usDevNewVid, g_usDevNewPid, g_usDevOldVid, g_usDevOldPid);
				pScan->Wait(dev, RKUSB_MASKROM /*,dev.usVid,dev.usPid*/);
				delete pScan;
			}
			bSuccess = true;
			printf("Download boot ok.\r\n");
		}
		else
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to download boot,err=%d", __func__, iRet);
			printf("Download boot failed!\r\n");
			err = pDevice->Error;
			strErr = GetErrString(err);
			if (strErr.size())
				printf("%s\r\n", strErr.c_str());
		}

		if (pImage)
			delete pImage;
		if (pDevice)
			CDeviceFactory::FreeDevice(pDevice);
	}
	return bSuccess;
}
bool erase_flash(STRUCT_RKDEVICE_DESC &dev, char *szLoader)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKImage *pImage = NULL;
	CRKBoot *pBoot = NULL;
	bool bRet, bSuccess = false;
	int ret;
	printf("Loading loader...\r\n");
	pImage = new CRKImage();
	ret = pImage->ObjectInit(szLoader, false);
	if (ret != 0)
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to open loader,err=%d!", __func__, ret);
		delete pImage;
		ERROR_COLOR_ATTR;
		printf("Loading loader failed,err=%d!", ret);
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		return bSuccess;
	}
	else
	{
		ENUM_RKDEVICE_TYPE type;
		pBoot = (CRKBoot *)pImage->m_bootObject;
		type = pBoot->SupportDevice;

		CRKScan *pScan = NULL;
		pScan = new CRKScan();
		pScan->SetVidPid(g_usMscVid, g_usMscPid);
		pScan->AddRockusbVidPid(g_usDevNewVid, g_usDevNewPid, g_usDevOldVid, g_usDevOldPid);
		CRKComm *pComm = NULL;
		CRKDevice *pDevice = NULL;
		CRKUpgrade *pUpgradeCtrlObject = NULL;
		RK_UPGRADE_SET ctrlStepStruct;

		dev.emDeviceType = pBoot->SupportDevice;
		pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
		if (!bRet)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
			if (pImage)
				delete pImage;
			if (pScan)
				delete pScan;
			ERROR_COLOR_ATTR;
			printf("Creating Comm Object failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			return bSuccess;
		}

		bRet = CRKUpgrade::CreateEraseAllBlocksStep(type, ctrlStepStruct);

		if (!bRet)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to create step flow object!", __func__);
			if (pImage)
				delete pImage;
			if (pComm)
				delete pComm;
			if (pScan)
				delete pScan;
			ERROR_COLOR_ATTR;
			printf("Creating erase flash step failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			return bSuccess;
		}

		pDevice = CDeviceFactory::CreateDevice(dev);
		if (!pDevice)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to create device object!", __func__);
			if (pImage)
				delete pImage;
			if (pComm)
				delete pComm;
			if (pScan)
				delete pScan;
			DestroyUpgradeSet(ctrlStepStruct);
			ERROR_COLOR_ATTR;
			printf("Creating device object failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			return bSuccess;
		}

		pDevice->SetObject(pImage, pComm, g_pLogObject);
		pDevice->GetNewDiskSizeFlag = false;
		pDevice->CallBackPointer = ProgressInfoProc;

		pUpgradeCtrlObject = new CRKUpgrade(ctrlStepStruct, WF_ERASEFLASH);

		bRet = pUpgradeCtrlObject->SetObject(pDevice, pScan);
		if (!bRet)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to create upgrade object!", __func__);
			if (pUpgradeCtrlObject)
				delete pUpgradeCtrlObject;
			if (pDevice)
				CDeviceFactory::FreeDevice(pDevice);
			else
			{
				if (pComm)
					delete pComm;
			}
			if (pScan)
				delete pScan;
			DestroyUpgradeSet(ctrlStepStruct);
			ERROR_COLOR_ATTR;
			printf("Creating upgrade object failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			return bSuccess;
		}
		pUpgradeCtrlObject->CallBackPointer = PromptInfoProc;

		printf("Start to erase flash...\r\n");
		bRet = pUpgradeCtrlObject->Run();

		if (pUpgradeCtrlObject)
			delete pUpgradeCtrlObject;

		if (pImage)
			delete pImage;

		DestroyUpgradeSet(ctrlStepStruct);

		if (bRet)
		{
			pScan = new CRKScan();
			if (pScan)
			{
				pScan->SetVidPid(g_usMscVid, g_usMscPid);
				pScan->AddRockusbVidPid(g_usDevNewVid, g_usDevNewPid, g_usDevOldVid, g_usDevOldPid);
				pScan->Wait(dev, RKUSB_MASKROM /*,dev.usVid,dev.usPid*/);
				delete pScan;
			}
			CURSOR_MOVEUP_LINE(1);
			CURSOR_DEL_LINE;
			bSuccess = true;
			printf("Erase flash ok.\r\n");
		}
	}
	return bSuccess;
}
bool lower_format(STRUCT_RKDEVICE_DESC &dev)
{
	if (!check_device_type(dev, RKUSB_LOADER))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		int iResult, iTryCount;
		uint_32 dwTotal, dwCurrent;
		uint_8 subCode = TU_LOWERFORMAT_SUBCODE;
		do
		{
			iTryCount = 3;
			while (iTryCount > 0)
			{
				iResult = pComm->RKU_TestDeviceReady(&dwTotal, &dwCurrent, subCode);
				if ((iResult == ERR_SUCCESS) || (iResult == ERR_DEVICE_UNREADY))
				{
					break;
				}
				if (g_pLogObject)
					g_pLogObject->Record("Error:%s->Fail to test device,subCode(0x%x),RetCode(%d)", __func__, subCode, iResult);
				iTryCount--;
				sleep_s(1);
			}
			if (iTryCount <= 0)
			{
				printf("Lower Format fail!\r\n");
				break;
			}
			if (iResult == ERR_SUCCESS)
			{
				if (subCode == TU_LOWERFORMAT_SUBCODE)
				{
					printf("Lower Format fail,no support!\r\n");
					break;
				}
				bSuccess = true;
				printf("Lower Format Device OK.\r\n");
				break;
			}
			if (subCode == TU_LOWERFORMAT_SUBCODE)
			{
				subCode = TU_NONE_SUBCODE;
				printf("Lower Format Device,total(%d),current(%d)\r\n", dwTotal, dwCurrent);
			}
			else
			{
				CURSOR_MOVEUP_LINE(1);
				CURSOR_DEL_LINE;
				printf("Lower Format Device,total(%d),current(%d)\r\n", dwTotal, dwCurrent);
			}
			sleep_s(1);
		} while (iResult == ERR_DEVICE_UNREADY);
	}
	else
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		printf("Lower Format quit,Creating comm object failed!\r\n");
	}
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	return bSuccess;
}
bool test_device(STRUCT_RKDEVICE_DESC &dev)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	int iRet;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		iRet = pComm->RKU_TestDeviceReady();
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to test device,err=%d", __func__, iRet);
			printf("Test Device Fail!\r\n");
		}
		else
		{
			bSuccess = true;
			printf("Test Device OK.\r\n");
		}
	}
	else
	{
		printf("Test Device quit,Creating comm object failed!\r\n");
	}
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	return bSuccess;
}
bool partition_list(STRUCT_RKDEVICE_DESC &dev)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	bool bSuccess = false, bGpt, bEnv, bMcu;
	PARAM_ITEM_VECTOR vecItem;
	bSuccess = get_parameter_device(dev, vecItem, bGpt, bEnv, bMcu);
	if (bSuccess)
	{
		if (bGpt)
			printf("Partition Info(gpt):\r\n");
		else if (bEnv)
			printf("Partition Info(env):\r\n");
		else if (bMcu)
			printf("Partition Info(mcu):\r\n");
		else
			printf("Partition Info(parameter):\r\n");
		printf("NO  LBA        Size       Name\r\n");
		int i;
		for (i = 0; i < vecItem.size(); i++)
		{
			printf("%02d  0x%08x 0x%08x %s\r\n", i + 1, vecItem[i].uiItemOffset, vecItem[i].uiItemSize, vecItem[i].szItemName);
		}
	}
	else
	{
		printf("Reading partition info from device failed!\r\n");
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to read partition info from device", __func__);
	}

	return bSuccess;
}

bool reset_device(STRUCT_RKDEVICE_DESC &dev, uint_8 subCode = RST_NONE_SUBCODE)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	int iRet;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		iRet = pComm->RKU_ResetDevice(subCode);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to reset,err=%d", __func__, iRet);
			printf("Reset Device Fail!\r\n");
		}
		else
		{
			bSuccess = true;
			printf("Reset Device OK.\r\n");
		}
	}
	else
	{
		printf("Reset Device quit,Creating comm object failed!\r\n");
	}
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	return bSuccess;
}
void reset_pipe(STRUCT_RKDEVICE_DESC &dev, uint_8 pipe = 2)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return;
	CRKUsbComm *pComm = NULL;
	bool bRet;
	int iRet;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		iRet = pComm->RKU_ResetPipe(pipe);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:reset_pipe failed,err=%d", iRet);
			printf("Reset Pipe Fail!\r\n");
		}
		else
			printf("Reset Pipe OK.\r\n");
	}
	else
	{
		printf("Reset Pipe quit,Creating comm object failed!\r\n");
	}
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
}

bool read_flash_id(STRUCT_RKDEVICE_DESC &dev)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	int iRet;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		uint_8 flashID[5];
		iRet = pComm->RKU_ReadFlashID(flashID);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to read flash ID,err=%d", __func__, iRet);
			printf("Read flash ID Fail!\r\n");
		}
		else
		{
			printf("Flash ID:%02X %02X %02X %02X %02X \r\n", flashID[0], flashID[1], flashID[2], flashID[3], flashID[4]);
			bSuccess = true;
		}
	}
	else
	{
		printf("Read flash ID quit,Creating comm object failed!\r\n");
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
	}
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	return bSuccess;
}
bool read_flash_info(STRUCT_RKDEVICE_DESC &dev)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	int iRet;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		STRUCT_FLASHINFO_CMD info;
		uint_32 uiRead;
		iRet = pComm->RKU_ReadFlashInfo((uint_8 *)&info, &uiRead);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to read flashinfo,err=%d", __func__, iRet);
			printf("Read flash Info Fail!\r\n");
		}
		else
		{
			printf("Flash Info:\r\n");
			
			printf("\tFlash Size: %dMB,value=0x%x\r\n", info.uiFlashSize / 2 / 1024, info.uiFlashSize);
			printf("\tBlock Size: %dKB,value=%d\r\n", info.usBlockSize / 2, info.usBlockSize);
			printf("\tPage Size: %dKB,value=%d\r\n", info.bPageSize / 2, info.bPageSize);
			if (info.bManufCode > 200)
			{
				printf("\tLBA Size: %d,value=%d\r\n", info.bManufCode-200, info.bManufCode);
			}
			else
				printf("\tLBA Size: 1\r\n");
			printf("\tECC Bits: %d\r\n", info.bECCBits);
			printf("\tAccess Time: %d\r\n", info.bAccessTime);
			printf("\tFlash CS: ");
			for (int i = 0; i < 8; i++)
			{
				if (info.bFlashCS & (1 << i))
					printf("<%d> ", i);
			}
			printf("\r\n");
			bSuccess = true;
		}
	}
	else
	{
		printf("Read flash Info quit,Creating comm object failed!\r\n");
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
	}
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	return bSuccess;
}
bool read_otp_cpuid(STRUCT_RKDEVICE_DESC &dev, string strOut)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	int iRet;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		unsigned char cpuid[32];
		iRet = pComm->RKU_ReadOtpCpuID(cpuid);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to read cpuid,err=%d", __func__, iRet);
			printf("Read CPU ID Fail!\r\n");
		}
		else
		{
			if (cpuid[0] > 16)
			{
				printf("\tCPU ID: [ ");
				for (int i = 0; i < 16; i++)
				{
					printf("%02X ", cpuid[i+1]);
				}
				printf("]\r\n");
				if (strOut.size()>0)
				{
					if (strOut.find(".bin")!=string::npos) {
						FILE *fout=NULL;
						fout = fopen(strOut.c_str(), "wb");
						fwrite(cpuid+1,1,16,fout);
						fclose(fout);
					} else {
						ofstream fout(strOut);
						if (fout.is_open()) {
							for (size_t i = 0; i < 16; i++)
							{
								//fout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << cpuid[i+1];
								fout << std::hex  << std::uppercase << std::setw(1) << (cpuid[i+1] >> 4);
								fout << std::hex  << std::uppercase << std::setw(1) << (cpuid[i+1] & 0xF);
							}
							fout.close();
						}
					}
				}
				
				bSuccess = true;
			}
			else
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:%s->invalid cpuid ,wrong size=[%02X]", __func__, cpuid[0]);
				printf("CPU ID size Invalid!\r\n");
			}
		}
	}
	else
	{
		printf("Read CPU ID quit,Creating comm object failed!\r\n");
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
	}
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	return bSuccess;
}
bool read_chip_info(STRUCT_RKDEVICE_DESC &dev)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	int iRet;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		uint_8 chipInfo[16];
		iRet = pComm->RKU_ReadChipInfo(chipInfo);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to read chipinfo,err=%d", __func__, iRet);
			printf("Read Chip Info Fail!\r\n");
		}
		else
		{
			string strChipInfo;
			g_pLogObject->PrintBuffer(strChipInfo, chipInfo, 16, 16);
			printf("Chip Info:%s\r\n", strChipInfo.c_str());
			bSuccess = true;
		}
	}
	else
	{
		printf("Read Chip Info quit,Creating comm object failed!\r\n");
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
	}
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	return bSuccess;
}
bool read_secure_mode(STRUCT_RKDEVICE_DESC &dev)
{
	if (!check_device_type(dev, RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	int iRet;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		uint_8 capability[8];
		iRet = pComm->RKU_ReadCapability(capability);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to read capability,err=%d", __func__, iRet);
			printf("Read Capability Fail!\r\n");
		}
		else
		{
			if (capability[0] & 0x80)
			{
				iRet = pComm->RKU_ReadNewEfuse(0, 0, (uint_8 *)&bRet, EFUSE_SECURE_FLAG);
				if (iRet != ERR_SUCCESS)
				{
					if (g_pLogObject)
						g_pLogObject->Record("Error:%s->Fail to read new efuse,err=%d", __func__, iRet);
					printf("Read Secure Mode Fail!\r\n");
				}
				else
				{
					if (bRet)
						printf("Device is in the Secure Mode.\r\n");
					else
						printf("Device is in the Normal Mode.\r\n");
					bSuccess = true;
				}
			}
			else
			{
				printf("Device did not support to read secure mode!\r\n");
				if (g_pLogObject)
					g_pLogObject->Record("Error:%s->can't support to read secure mode!", __func__);
			}
		}
	}
	else
	{
		printf("Read Secure Mode quit,Creating comm object failed!\r\n");
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
	}
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	return bSuccess;
}
bool read_sector(STRUCT_RKDEVICE_DESC &dev, uint_32 uiBegin, uint_32 uiLen, char *szFile)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	FILE *file = NULL;
	bool bRet, bSuccess = false;
	int iRet, nPercent, nPrevPercent;
	uint_32 iTotalRead = 0, iRead = 0;
	int nSectorSize = 528;
	uint32_t uiTotal, uiPageSizeB = 2 * 1024, uiPaseSpareB = 64, uiPageTotalB, uiPageSector = 4, uiSectorTotalB;
	uint8_t *pBuf = NULL;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{

		file = fopen(szFile, "wb+");
		if (!file)
		{
			printf("Read sector failed,err=%d,can't open file: %s\r\n", errno, szFile);
			goto Exit_ReadSector;
		}

		uiPageTotalB = uiPageSizeB + uiPaseSpareB;
		uiSectorTotalB = uiPageTotalB / uiPageSector;
		pBuf = new uint8_t[uiPageTotalB];
		nPercent = nPrevPercent = 0;
		iRead = (uiSectorTotalB << 16) | uiPageSector;
		uiTotal = uiLen;
		while (uiLen > 0)
		{
			memset(pBuf, 0, uiPageTotalB);
			iRet = pComm->RKU_ReadSector(uiBegin + iTotalRead, iRead, pBuf);
			if (ERR_SUCCESS == iRet)
			{
				uiLen -= uiPageSector;
				iTotalRead += uiPageSector;
				nPercent = ((double)iTotalRead / uiTotal) * 100;
				if (nPercent > nPrevPercent)
				{
					if (nPrevPercent == 0)
						printf("Read Sector into file (%d%%)\r\n", nPercent);
					else
					{
						CURSOR_MOVEUP_LINE(1);
						CURSOR_DEL_LINE;
						printf("Read Sector into file (%d%%)\r\n", nPercent);
					}
					nPrevPercent = nPercent;
				}
				fwrite(pBuf, 1, uiPageTotalB, file);
			}
			else
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:RKU_ReadSector failed,err=%d", iRet);

				printf("Error:Read sector failed!\r\n");
				goto Exit_ReadSector;
			}
		}
		bSuccess = true;
	}
	else
	{
		printf("Read sector quit,Creating comm object failed!\r\n");
	}
Exit_ReadSector:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (file)
		fclose(file);
	if (pBuf)
	{
		delete[] pBuf;
	}
	return bSuccess;
}
bool write_sector(STRUCT_RKDEVICE_DESC &dev, uint32_t uiBegin, uint32_t uiPageSizeK, uint32_t uiPaseSpareB, char *szFile)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	FILE *file = NULL;
	bool bRet, bSuccess = false;
	int iRet, nPercent, nPrevPercent;
	long long iTotalWrite = 0, iFileSize = 0;
	int iWrite = 0;
	uint32_t uiLen, uiPageSizeB = uiPageSizeK * 1024, uiPageTotalB, uiPageSector, uiSectorTotalB;
	uint8_t *pBuf = NULL;

	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		file = fopen(szFile, "rb");
		if (!file)
		{
			printf("Write sector failed,err=%d,can't open file: %s\r\n", errno, szFile);
			goto Exit_WriteSector;
		}
		fseek(file, 0, SEEK_END);
		iFileSize = ftell(file);
		fseek(file, 0, SEEK_SET);
		uiPageTotalB = uiPageSizeB + uiPaseSpareB;
		uiPageSector = uiPageSizeK * 2;
		uiSectorTotalB = uiPageTotalB / uiPageSector;
		if ((iFileSize % uiPageTotalB) != 0)
		{
			printf("File size is not algin with page+spare=%d\r\n", uiPageTotalB);
			goto Exit_WriteSector;
		}
		pBuf = new uint8_t[uiPageTotalB];
		nPercent = nPrevPercent = 0;
		uiLen = (uiSectorTotalB << 16) | uiPageSector;
		while (iTotalWrite < iFileSize)
		{
			memset(pBuf, 0, uiPageTotalB);
			iWrite = fread(pBuf, 1, uiPageTotalB, file);
			iRet = pComm->RKU_WriteSector(uiBegin, uiLen, pBuf);
			if (ERR_SUCCESS == iRet)
			{
				iTotalWrite += iWrite;
				uiBegin += uiPageSector;
				nPercent = ((double)iTotalWrite / iFileSize) * 100;
				if (nPercent > nPrevPercent)
				{
					if (nPrevPercent == 0)
						printf("Write Sector from file (%d%%)\r\n", nPercent);
					else
					{
						CURSOR_MOVEUP_LINE(1);
						CURSOR_DEL_LINE;
						printf("Write Sector from file (%d%%)\r\n", nPercent);
					}
					nPrevPercent = nPercent;
				}
			}
			else
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:RKU_WriteSector failed,err=%d", iRet);

				printf("Error:Write sector failed!\r\n");
				goto Exit_WriteSector;
			}
		}
		bSuccess = true;
	}
	else
	{
		printf("Write sector quit,Creating comm object failed!\r\n");
	}
Exit_WriteSector:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (file)
		fclose(file);
	if (pBuf)
		delete[] pBuf;
	return bSuccess;
}
bool read_efuse(STRUCT_RKDEVICE_DESC &dev, uint_32 uiBegin, uint_32 uiLen, char *szFile)
{
	if (!check_device_type(dev, RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	FILE *file = NULL;
	bool bRet, bSuccess = false;
	int iRet;
	uint_8 buf[512 * 8];

	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		if (szFile)
		{
			file = fopen(szFile, "wb+");
			if (!file)
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:%s->Fail to create %s,err=%d!", __func__, szFile, errno);
				printf("Read Efuse failed,err=%d,can't open file: %s\r\n", errno, szFile);
				goto Exit_ReadEfuse;
			}
		}

		iRet = pComm->RKU_ReadNewEfuse(uiBegin, uiLen, buf, EFUSE_EXTEND_RW);
		if (ERR_SUCCESS == iRet)
		{
			if (szFile)
			{
				fwrite(buf, 1, uiLen, file);
				printf("Read Efuse to %s OK.\r\n", szFile);
			}
			else
				PrintEfuseData(buf, uiLen, uiBegin);
		}
		else
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to read new efuse,err=%d", __func__, iRet);

			printf("Read Efuse failed!\r\n");
			goto Exit_ReadEfuse;
		}
		bSuccess = true;
	}
	else
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		printf("Read Efuse quit,Creating comm object failed!\r\n");
	}
Exit_ReadEfuse:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (file)
		fclose(file);
	return bSuccess;
}
bool write_efuse(STRUCT_RKDEVICE_DESC &dev, uint_32 uiBegin, char *szFile)
{
	if (!check_device_type(dev, RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	FILE *file = NULL;
	bool bRet, bSuccess = false;
	int iRet, i;
	uint_32 iRead = 0;
	uint_32 uiLen;
	uint_8 *pBuf = NULL;
	string strData = szFile;

	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		if ((strData[0] == '\"') && (strData[strData.size() - 1] == '\"'))
		{ // data
			uiLen = strData.size() - 2;
			pBuf = new uint_8[uiLen];
			if (!pBuf)
			{
				printf("Write Efuse failed,err=%d,can't new memory!\r\n", errno);
				goto Exit_WriteEfuse;
			}
			uiLen = 0;
			for (i = 0; i < strData.size(); i++)
			{
				if ((strData[i] == '0') || (strData[i] == '1'))
					pBuf[uiLen++] = strData[i] - 0x30;
			}
		}
		else
		{ // file
			file = fopen(szFile, "rb");
			if (!file)
			{
				printf("Write Efuse failed,err=%d,can't open file: %s\r\n", errno, szFile);
				goto Exit_WriteEfuse;
			}

			fseek(file, 0, SEEK_END);
			uiLen = ftell(file);
			fseek(file, 0, SEEK_SET);
			pBuf = new uint_8[uiLen];
			if (!pBuf)
			{
				printf("Write Efuse failed,err=%d,can't new memory!\r\n", errno);
				goto Exit_WriteEfuse;
			}
			iRead = fread(pBuf, 1, uiLen, file);
			if (iRead != uiLen)
			{
				printf("Write Efuse failed,err=%d,read file error: total=%d,read=%d\r\n", errno, uiLen, iRead);
				goto Exit_WriteEfuse;
			}
			for (i = 0; i < uiLen; i++)
				if ((pBuf[i] != 0) && (pBuf[i] != 1))
				{
					printf("Write Efuse failed,file invalid: index=%d,value=%x\r\n", i, pBuf[i]);
					goto Exit_WriteEfuse;
				}
		}

		iRet = pComm->RKU_WriteNewEfuse(uiBegin, uiLen, pBuf, EFUSE_EXTEND_RW);
		if (ERR_SUCCESS != iRet)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:RKU_WriteNewEfuse failed,err=%d", iRet);

			printf("Write Efuse failed!\r\n");
			goto Exit_WriteEfuse;
		}
		printf("Write Efuse OK.\r\n");
		bSuccess = true;
	}
	else
	{
		printf("Write Efuse quit,Creating comm object failed!\r\n");
	}
Exit_WriteEfuse:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (pBuf)
	{
		delete[] pBuf;
		pBuf = NULL;
	}
	if (file)
		fclose(file);
	return bSuccess;
}
bool write_vendor_sn(STRUCT_RKDEVICE_DESC &dev, char *sn)
{
	if (!check_device_type(dev, RKUSB_MASKROM | RKUSB_LOADER))
		return false;
	CRKUsbComm *pComm = NULL;
	PPROVISIONING_DATA pProvision = NULL;
	bool bRet, bSuccess = false;
	int iRet, snLen;
	uint_32 nBufferSize = 0;
	uint_8 *pBuf = NULL;

	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		snLen = strlen(sn);
		nBufferSize = ((snLen + 7) / SECTOR_SIZE + 1) * SECTOR_SIZE;
		pBuf = new uint_8[nBufferSize];
		if (!pBuf)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to new memory,err=%d", __func__, errno);
			printf("ERROR:malloc memory failed,err=%d!\r\n", errno);
			goto Exit_WriteVendorSn;
		}
		memset(pBuf, 0, nBufferSize);
		pProvision = (PPROVISIONING_DATA)pBuf;
		pProvision->id = 1;
		pProvision->size = snLen;
		memcpy(pProvision->data, sn, snLen);
		iRet = pComm->RKU_WriteLBA(0xFFF00001, nBufferSize / SECTOR_SIZE, pBuf);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to write sn,err=%d", __func__, iRet);
			printf("ERROR:failed to write sn,err=%d!\r\n", iRet);
			goto Exit_WriteVendorSn;
		}
		printf("Write sn(%s) OK.\r\n", sn);
		bSuccess = true;
	}
	else
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		printf("Write Sn quit,Creating comm object failed!\r\n");
	}
Exit_WriteVendorSn:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (pBuf)
	{
		delete[] pBuf;
		pBuf = NULL;
	}
	return bSuccess;
}
bool read_vendor_sn(STRUCT_RKDEVICE_DESC &dev)
{
	if (!check_device_type(dev, RKUSB_MASKROM | RKUSB_LOADER))
		return false;
	CRKUsbComm *pComm = NULL;
	PPROVISIONING_DATA pProvision = NULL;
	bool bRet, bSuccess = false;
	int iRet, snLen;
	uint_32 nBufferSize = 0;
	uint_8 *pBuf = NULL;

	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		snLen = 60;
		nBufferSize = ((snLen + 7) / SECTOR_SIZE + 1) * SECTOR_SIZE;
		pBuf = new uint_8[nBufferSize];
		if (!pBuf)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to new memory,err=%d", __func__, errno);
			printf("ERROR:malloc memory failed,err=%d!\r\n", errno);
			goto Exit_ReadVendorSn;
		}
		memset(pBuf, 0, nBufferSize);
		pProvision = (PPROVISIONING_DATA)pBuf;
		pProvision->id = 1;
		pProvision->size = nBufferSize;

		iRet = pComm->RKU_ReadLBA(0xFFF00001, nBufferSize / SECTOR_SIZE, pBuf);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to read sn,err=%d", __func__, iRet);
			printf("ERROR:failed to read sn,err=%d!\r\n", iRet);
			goto Exit_ReadVendorSn;
		}
		if (snLen < pProvision->size)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to check sn size,size=%d", __func__, pProvision->size);
			printf("ERROR:sn is limited by 60,%d is too long!\r\n", pProvision->size);
			goto Exit_ReadVendorSn;
		}

		printf("sn is %s.\r\n", (char *)pProvision->data);
		bSuccess = true;
	}
	else
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		printf("Read Sn quit,Creating comm object failed!\r\n");
	}
Exit_ReadVendorSn:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (pBuf)
	{
		delete[] pBuf;
		pBuf = NULL;
	}
	return bSuccess;
}

bool read_lba(STRUCT_RKDEVICE_DESC &dev, uint_32 uiBegin, uint_32 uiLen, char *szFile)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	FILE *file = NULL;
	bool bRet, bSuccess = false;
	int iRet, nPercent, nPrevPercent, nTotalRead = uiLen;
	uint_32 iTotalRead = 0, iRead = 0;
	int nSectorSize = 512;
	uint_8 pBuf[SECTOR_SIZE * DEFAULT_RW_LBA];
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		if (szFile)
		{
			file = fopen(szFile, "wb+");
			if (!file)
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:%s->Fail to create %s,err=%d", __func__, szFile, errno);
				printf("Read LBA failed,err=%d,can't open file: %s\r\n", errno, szFile);
				goto Exit_ReadLBA;
			}
		}
		nPercent = nPrevPercent = 0;
		while (uiLen > 0)
		{
			memset(pBuf, 0, nSectorSize * DEFAULT_RW_LBA);
			iRead = (uiLen >= DEFAULT_RW_LBA) ? DEFAULT_RW_LBA : uiLen;
			iRet = pComm->RKU_ReadLBA(uiBegin + iTotalRead, iRead, pBuf);
			if (ERR_SUCCESS == iRet)
			{
				uiLen -= iRead;
				iTotalRead += iRead;

				if (szFile)
				{
					fwrite(pBuf, 1, iRead * nSectorSize, file);
					nPercent = ((double)iTotalRead / nTotalRead) * 100;
					if (nPercent > nPrevPercent)
					{
						if (nPrevPercent == 0)
							printf("Read LBA to file (%d%%)\r\n", nPercent);
						else
						{
							CURSOR_MOVEUP_LINE(1);
							CURSOR_DEL_LINE;
							printf("Read LBA to file (%d%%)\r\n", nPercent);
						}
						nPrevPercent = nPercent;
					}
				}
				else
					PrintData(pBuf, nSectorSize * iRead);
			}
			else
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:%s->Fail to read lba,err=%d", __func__, iRet);

				printf("Read LBA failed!\r\n");
				goto Exit_ReadLBA;
			}
		}
		bSuccess = true;
	}
	else
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		printf("Read LBA quit,Creating comm object failed!\r\n");
	}
Exit_ReadLBA:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (file)
		fclose(file);
	return bSuccess;
}
bool read_disk(CRKDisk *disk, uint64_t uiBegin, uint64_t uiLen, char *szFile)
{
	FILE *file = NULL;
	bool bRet, bSuccess = false;
	int nPercent, nPrevPercent;
	uint32_t iRead = 0, transfer_bulk_size;
	uint64_t iTotalRead = 0, nTotalRead;
	uint_8 pBuf[SECTOR_SIZE * DEFAULT_RW_LBA];
	transfer_bulk_size = SECTOR_SIZE * DEFAULT_RW_LBA;

	file = fopen(szFile, "wb+");
	if (!file)
	{
		printf("err=%d,failed to open file: %s\r\n", errno, szFile);
		goto Exit_ReadDisk;
	}

	nPercent = nPrevPercent = 0;
	uiBegin *= SECTOR_SIZE;
	uiLen *= SECTOR_SIZE;
	nTotalRead = uiLen;
	while (uiLen > 0)
	{
		memset(pBuf, 0, transfer_bulk_size);
		iRead = (uiLen >= transfer_bulk_size) ? transfer_bulk_size : uiLen;
		bRet = disk->ReadDisk(uiBegin + iTotalRead, iRead, pBuf);
		if (bRet)
		{
			uiLen -= iRead;
			iTotalRead += iRead;

			fwrite(pBuf, 1, iRead, file);
			nPercent = ((double)iTotalRead / nTotalRead) * 100;
			if (nPercent > nPrevPercent)
			{
				if (nPrevPercent == 0)
					printf("Read Disk to file (%d%%)\r\n", nPercent);
				else
				{
					CURSOR_MOVEUP_LINE(1);
					CURSOR_DEL_LINE;
					printf("Read Disk to file (%d%%)\r\n", nPercent);
				}
				nPrevPercent = nPercent;
			}
		}
		else
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:ReadDisk failed,err=%d", disk->GetLastDiskError());

			printf("Read Disk failed!\r\n");
			goto Exit_ReadDisk;
		}
	}
	bSuccess = true;

Exit_ReadDisk:
	if (file)
		fclose(file);
	return bSuccess;
}
bool write_sparse_lba(STRUCT_RKDEVICE_DESC &dev, uint_32 uiBegin, uint_32 uiPartSize, char *szFile)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	STRUCT_FLASHINFO_CMD info;
	CRKUsbComm *pComm = NULL;
	FILE *file = NULL;
	bool bRet, bSuccess = false;
	int iRet, nPercent, nPrevPercent;
	uint_64 iTotalWrite = 0, iFileSize = 0, dwChunkDataSize;
	uint_32 iRead = 0, uiTransferSec, curChunk, i, uiSize;
	uint_32 dwMaxReadWriteBytes, dwTransferBytes, dwFillByte, dwCrc;
	uint_8 pBuf[SECTOR_SIZE * DEFAULT_RW_LBA];
	sparse_header header;
	chunk_header chunk;
	string file_name = szFile;
	size_t pos;
	pos = file_name.find_last_of(PATH_DELIMITER);
	if (pos != string::npos)
		file_name = file_name.substr(pos + 1);
	if (g_pLogObject)
	{
		g_pLogObject->Record("Info:%s-->Start to write %s at 0x%x...", __func__, file_name.c_str(), uiBegin);
	}
	printf("Start to write %s at 0x%x...sparse\r\n", file_name.c_str(), uiBegin);
	dwMaxReadWriteBytes = DEFAULT_RW_LBA * SECTOR_SIZE;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{

		file = fopen(szFile, "rb");
		if (!file)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to open %s,err=%d", __func__, szFile, errno);
			printf("%s failed, err=%d, can't open file: %s\r\n", __func__, errno, szFile);
			goto Exit_WriteSparseLBA;
		}
		fseek(file, 0, SEEK_SET);
		iRead = fread(&header, 1, sizeof(header), file);
		if (iRead != sizeof(sparse_header))
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to read sparse header,file=%s,err=%d", __func__, szFile, errno);
			}
			goto Exit_WriteSparseLBA;
		}
		iFileSize = header.blk_sz * (uint_64)header.total_blks;
		iTotalWrite = 0;
		curChunk = 0;
		uiSize = iFileSize / SECTOR_SIZE;
		iRet = pComm->RKU_ReadFlashInfo((uint_8 *)&info, NULL);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to read flash info,err=%d", __func__, iRet);
			}
			printf("ReadFlashInfo failed, err=%d\r\n", iRet);
			goto Exit_WriteSparseLBA;
		}

		if (uiBegin >= info.uiFlashSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Partition offset is larger than flash size, partition_offset=0x%08X,flash_size=0x%08X", __func__, uiBegin, info.uiFlashSize);
			}
			printf("Begin is larger than flash size\r\n");
			goto Exit_WriteSparseLBA;
		}
		if (uiPartSize == 0xFFFFFFFF)
		{
			uiPartSize = info.uiFlashSize - uiBegin;

			// if (info.uiFlashSize >= 0x800000)
			// { // exclude backup gpt
			// 	uiPartSize -= 64;
			// }
			// else
			// 	uiPartSize -= 33;
		}
		if ((uiBegin + uiPartSize) > info.uiFlashSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Partition ending is larger than flash size, partition_end=0x%08X,flash_size=0x%08X", __func__, uiBegin + uiPartSize, info.uiFlashSize);
			}
			printf("Partition ending is larger than flash size\r\n");
			goto Exit_WriteSparseLBA;
		}
		if (uiSize > uiPartSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->size of sparse file is larger than partition size,sparse=%d,partition=%d", __func__, uiSize, uiPartSize);
			}
			printf("partition too small,sparse=%d,partition=%d\r\n", uiSize, uiPartSize);
			goto Exit_WriteSparseLBA;
		}

		if (g_pLogObject)
		{
			g_pLogObject->Record("Info:%s->erase start, unsparse=0x%08x,partsize=0x%08x", __func__, uiSize, uiPartSize);
		}
		printf("Start to erase...0x%x,0x%x\r\n", uiBegin, uiSize);
		bRet = erase_partition(pComm, uiBegin, uiSize);
		if (!bRet)
		{
			printf("erase sparse space error\r\n");
			goto Exit_WriteSparseLBA;
		}
		// erase last 32k to destroy verity meta
		bRet = erase_partition(pComm, uiBegin + uiPartSize - 64, 64);
		if (!bRet)
		{
			printf("erase sparse verity data error\r\n");
			goto Exit_WriteSparseLBA;
		}
		printf("erase done\r\n");
		if (g_pLogObject)
		{
			g_pLogObject->Record("Info:%s->start to write sparse, total_chunk=%d", __func__, header.total_chunks);
		}
		nPrevPercent = nPercent = 0;
		while (curChunk < header.total_chunks)
		{
			if (!EatSparseChunk(file, chunk))
			{
				goto Exit_WriteSparseLBA;
			}
			curChunk++;
			switch (chunk.chunk_type)
			{
			case CHUNK_TYPE_RAW:
				dwChunkDataSize = chunk.total_sz - sizeof(chunk_header);
				while (dwChunkDataSize)
				{
					memset(pBuf, 0, dwMaxReadWriteBytes);
					if (dwChunkDataSize >= dwMaxReadWriteBytes)
					{
						dwTransferBytes = dwMaxReadWriteBytes;
						uiTransferSec = DEFAULT_RW_LBA;
					}
					else
					{
						dwTransferBytes = dwChunkDataSize;
						uiTransferSec = ((dwTransferBytes % SECTOR_SIZE == 0) ? (dwTransferBytes / SECTOR_SIZE) : (dwTransferBytes / SECTOR_SIZE + 1));
					}
					if (!EatSparseData(file, pBuf, dwTransferBytes))
					{
						goto Exit_WriteSparseLBA;
					}
					iRet = pComm->RKU_WriteLBA(uiBegin, uiTransferSec, pBuf);
					if (ERR_SUCCESS == iRet)
					{
						dwChunkDataSize -= dwTransferBytes;
						iTotalWrite += dwTransferBytes;
						uiBegin += uiTransferSec;
					}
					else
					{
						if (g_pLogObject)
						{
							g_pLogObject->Record("Error:%s->Fail to write lba,Written(%d),RetCode(%d)", __func__, iTotalWrite, iRet);
						}
						goto Exit_WriteSparseLBA;
					}
					nPercent = ((double)iTotalWrite / iFileSize) * 100;
					if (nPercent > nPrevPercent)
					{
						if (nPrevPercent == 0)
							printf("Write LBA from file (%d%%)\r\n", nPercent);
						else
						{
							CURSOR_MOVEUP_LINE(1);
							CURSOR_DEL_LINE;
							printf("Write LBA from file (%d%%)\r\n", nPercent);
						}
						nPrevPercent = nPercent;
					}
				}
				break;
			case CHUNK_TYPE_FILL:
				dwChunkDataSize = (uint_64)chunk.chunk_sz * header.blk_sz;
				if (!EatSparseData(file, (uint_8 *)&dwFillByte, 4))
				{
					goto Exit_WriteSparseLBA;
				}
				if (dwFillByte == 0)
				{
					iTotalWrite += dwChunkDataSize;
					uiTransferSec = ((dwChunkDataSize % SECTOR_SIZE == 0) ? (dwChunkDataSize / SECTOR_SIZE) : (dwChunkDataSize / SECTOR_SIZE + 1));
					uiBegin += uiTransferSec;
					nPercent = ((double)iTotalWrite / iFileSize) * 100;
					if (nPercent > nPrevPercent)
					{
						if (nPrevPercent == 0)
							printf("Write LBA from file (%d%%)\r\n", nPercent);
						else
						{
							CURSOR_MOVEUP_LINE(1);
							CURSOR_DEL_LINE;
							printf("Write LBA from file (%d%%)\r\n", nPercent);
						}
						nPrevPercent = nPercent;
					}
				}
				else
				{
					while (dwChunkDataSize)
					{
						memset(pBuf, 0, dwMaxReadWriteBytes);
						if (dwChunkDataSize >= dwMaxReadWriteBytes)
						{
							dwTransferBytes = dwMaxReadWriteBytes;
							uiTransferSec = DEFAULT_RW_LBA;
						}
						else
						{
							dwTransferBytes = dwChunkDataSize;
							uiTransferSec = ((dwTransferBytes % SECTOR_SIZE == 0) ? (dwTransferBytes / SECTOR_SIZE) : (dwTransferBytes / SECTOR_SIZE + 1));
						}
						for (i = 0; i < dwTransferBytes / 4; i++)
						{
							*(uint_32 *)(pBuf + i * 4) = dwFillByte;
						}
						iRet = pComm->RKU_WriteLBA(uiBegin, uiTransferSec, pBuf);
						if (ERR_SUCCESS == iRet)
						{
							dwChunkDataSize -= dwTransferBytes;
							iTotalWrite += dwTransferBytes;
							uiBegin += uiTransferSec;
						}
						else
						{
							if (g_pLogObject)
							{
								g_pLogObject->Record("Error:%s->Fail to write lba,Written(%d),RetCode(%d)", __func__, iTotalWrite, iRet);
							}
							goto Exit_WriteSparseLBA;
						}
						nPercent = ((double)iTotalWrite / iFileSize) * 100;
						if (nPercent > nPrevPercent)
						{
							if (nPrevPercent == 0)
								printf("Write LBA from file (%d%%)\r\n", nPercent);
							else
							{
								CURSOR_MOVEUP_LINE(1);
								CURSOR_DEL_LINE;
								printf("Write LBA from file (%d%%)\r\n", nPercent);
							}
							nPrevPercent = nPercent;
						}
					}
				}
				break;
			case CHUNK_TYPE_DONT_CARE:
				dwChunkDataSize = (uint_64)chunk.chunk_sz * header.blk_sz;
				iTotalWrite += dwChunkDataSize;
				uiTransferSec = ((dwChunkDataSize % SECTOR_SIZE == 0) ? (dwChunkDataSize / SECTOR_SIZE) : (dwChunkDataSize / SECTOR_SIZE + 1));
				uiBegin += uiTransferSec;
				nPercent = ((double)iTotalWrite / iFileSize) * 100;
				if (nPercent > nPrevPercent)
				{
					if (nPrevPercent == 0)
						printf("Write LBA from file (%d%%)\r\n", nPercent);
					else
					{
						CURSOR_MOVEUP_LINE(1);
						CURSOR_DEL_LINE;
						printf("Write LBA from file (%d%%)\r\n", nPercent);
					}
					nPrevPercent = nPercent;
				}
				break;
			case CHUNK_TYPE_CRC32:
				EatSparseData(file, (uint_8 *)&dwCrc, 4);
				break;
			}
		}
		bSuccess = true;
	}
	else
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		printf("Write LBA quit, creating comm object failed!\r\n");
	}
Exit_WriteSparseLBA:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (file)
		fclose(file);
	return bSuccess;
}
bool write_ubi_lba(STRUCT_RKDEVICE_DESC &dev, uint_32 uiBegin, uint_32 uiPartSize, uint_32 uiPageSize, char *szFile)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	STRUCT_FLASHINFO_CMD info;
	CRKUsbComm *pComm = NULL;
	FILE *file = NULL;
	bool bRet, bSuccess = false, bDhara = false;
	int iRet, nPercent, nPrevPercent;
	uint_64 iTotalWrite = 0, iFileSize = 0;
	uint_32 iRead = 0, uiTransferSec, uiSize;
	uint_32 dwMaxReadWriteBytes, dwTransferBytes;
	uint_8 pBuf[SECTOR_SIZE * DEFAULT_RW_LBA];
	string file_name = szFile;
	size_t pos;

	if (uiPageSize == (uint_32)-1)
		bDhara = true;
	pos = file_name.find_last_of(PATH_DELIMITER);
	if (pos != string::npos)
		file_name = file_name.substr(pos + 1);
	if (g_pLogObject)
	{
		if (bDhara)
			g_pLogObject->Record("Info:%s-->Start to write %s at 0x%x...dhara", __func__, file_name.c_str(), uiBegin);
		else
			g_pLogObject->Record("Info:%s-->Start to write %s at 0x%x...ubi", __func__, file_name.c_str(), uiBegin);
	}
	if (bDhara)
		printf("Start to write %s at 0x%x...dhara\r\n", file_name.c_str(), uiBegin);
	else
		printf("Start to write %s at 0x%x...ubi\r\n", file_name.c_str(), uiBegin);	
	dwMaxReadWriteBytes = DEFAULT_RW_LBA * SECTOR_SIZE;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		iFileSize = get_file_size(szFile);
		file = fopen(szFile, "rb");
		if (!file)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to open %s,err=%d!", __func__, szFile, errno);
			printf("%s failed, err=%d, can't open file: %s\r\n", __func__, errno, szFile);
			goto Exit_WriteUbiLBA;
		}

		fseek(file, 0, SEEK_SET);

		iRet = pComm->RKU_ReadFlashInfo((uint_8 *)&info, NULL);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to read flash info,err=%d", __func__, iRet);
			}
			printf("ReadFlashInfo failed, err=%d\r\n", iRet);
			goto Exit_WriteUbiLBA;
		}

		if (uiBegin >= info.uiFlashSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Partition offset is larger than flash size, partition_offset=0x%08X,flash_size=0x%08X", __func__, uiBegin, info.uiFlashSize);
			}
			printf("Begin is larger than flash size\r\n");
			goto Exit_WriteUbiLBA;
		}
		if (uiPartSize == 0xFFFFFFFF)
		{
			uiPartSize = info.uiFlashSize - uiBegin;

			// if (info.uiFlashSize >= 0x800000)
			// { // exclude backup gpt
			// 	uiPartSize -= 64;
			// }
			// else
			// 	uiPartSize -= 33;
		}
		if ((uiBegin + uiPartSize) > info.uiFlashSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Partition ending is larger than flash size, partition_end=0x%08X,flash_size=0x%08X", __func__, uiBegin + uiPartSize, info.uiFlashSize);
			}
			printf("Partition ending is larger than flash size\r\n");
			goto Exit_WriteUbiLBA;
		}
		uiSize = iFileSize / SECTOR_SIZE;
		if (uiSize > uiPartSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->size of file is larger than partition size,image=%d,partition=%d", __func__, uiSize, uiPartSize);
			}
			printf("partition too small,image=%d,partition=%d\r\n", uiSize, uiPartSize);
			goto Exit_WriteUbiLBA;
		}
		if (!bDhara) {
			if ((info.bPageSize * SECTOR_SIZE) != uiPageSize)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to check UBI pagesize,image:%u,device:%u", __func__, uiPageSize / SECTOR_SIZE, info.bPageSize);
				}
				printf("failed to check ubi page size,image=%d,device=%d\r\n", uiPageSize / SECTOR_SIZE, info.bPageSize);
				goto Exit_WriteUbiLBA;
			}
			if (g_pLogObject)
			{
				g_pLogObject->Record("Info:%s->start to erase before writing ubi image,offset=0x%x count=%d", __func__, uiBegin, uiPartSize);
			}
		} else {
			if (g_pLogObject)
			{
				g_pLogObject->Record("Info:%s->start to erase before writing dhara image,offset=0x%x count=%d", __func__, uiBegin, uiPartSize);
			}
		}
		
		printf("start to erase...0x%x,0x%x\r\n", uiBegin, uiPartSize);
		bRet = erase_ubi_block(pComm, uiBegin, uiPartSize);
		if (!bRet)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to erase ubi partition", __func__);
			}
			printf("failed to erase ubi block\r\n");
			goto Exit_WriteUbiLBA;
		}
		printf("erase done\r\n");
		if (g_pLogObject)
		{
			if (bDhara)
				g_pLogObject->Record("Info:%s->start to write dhara, total_sector=%d", __func__, uiSize);
			else
				g_pLogObject->Record("Info:%s->start to write ubi, total_sector=%d", __func__, uiSize);

		}
		nPercent = nPrevPercent = 0;
		while (iTotalWrite < iFileSize)
		{
			memset(pBuf, 0, dwMaxReadWriteBytes);
			if ((iFileSize - iTotalWrite) >= dwMaxReadWriteBytes)
			{
				dwTransferBytes = dwMaxReadWriteBytes;
				uiTransferSec = dwMaxReadWriteBytes / SECTOR_SIZE;
			}
			else
			{
				dwTransferBytes = iFileSize - iTotalWrite;
				uiTransferSec = ((dwTransferBytes % SECTOR_SIZE == 0) ? (dwTransferBytes / SECTOR_SIZE) : (dwTransferBytes / SECTOR_SIZE + 1));
			}
			iRead = fread(pBuf, 1, dwTransferBytes, file);
			if (iRead != dwTransferBytes)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to read data from file in write step,err=%d,read=%d,ret=%d", __func__, errno, dwTransferBytes, iRead);
				}
				printf("Failed to read data from file,errno=%d!\r\n", errno);
				goto Exit_WriteUbiLBA;
			}

			iRet = pComm->RKU_WriteLBA(uiBegin, uiTransferSec, pBuf);
			if (ERR_SUCCESS == iRet)
			{
				iTotalWrite += dwTransferBytes;
				uiBegin += uiTransferSec;

				nPercent = ((double)iTotalWrite / iFileSize) * 100;
				if (nPercent > nPrevPercent)
				{
					if (nPrevPercent == 0)
						printf("Write LBA from file (%d%%)\r\n", nPercent);
					else
					{
						CURSOR_MOVEUP_LINE(1);
						CURSOR_DEL_LINE;
						printf("Write LBA from file (%d%%)\r\n", nPercent);
					}
					nPrevPercent = nPercent;
				}
			}
			else
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to write lba,Written(%dK),RetCode(%d)", __func__, iTotalWrite / 1024, iRet);
				}
				printf("Failed to write data into device,err=%d!\r\n", iRet);
				goto Exit_WriteUbiLBA;
			}
		}
		bSuccess = true;
	}
	else
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		printf("Write LBA quit, creating comm object failed!\r\n");
	}
Exit_WriteUbiLBA:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (file)
		fclose(file);
	return bSuccess;
}
bool write_jffs_lba(STRUCT_RKDEVICE_DESC &dev, uint_32 uiBegin, uint_32 uiPartSize, char *szFile)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	STRUCT_FLASHINFO_CMD info;
	CRKUsbComm *pComm = NULL;
	FILE *file = NULL;
	bool bRet, bSuccess = false;
	int iRet, nPercent, nPrevPercent;
	uint_64 iTotalWrite = 0, iFileSize = 0;
	uint_32 iRead = 0, uiTransferSec, uiSize;
	uint_32 dwMaxReadWriteBytes, dwTransferBytes;
	uint_8 pBuf[SECTOR_SIZE * DEFAULT_RW_LBA];
	string file_name = szFile;
	size_t pos;
	pos = file_name.find_last_of(PATH_DELIMITER);
	if (pos != string::npos)
		file_name = file_name.substr(pos + 1);
	if (g_pLogObject)
	{
		g_pLogObject->Record("Info:%s-->Start to write %s at 0x%x...jffs", __func__, file_name.c_str(), uiBegin);
	}
	printf("Start to write %s at 0x%x...jffs\r\n", file_name.c_str(), uiBegin);
	dwMaxReadWriteBytes = DEFAULT_RW_LBA * SECTOR_SIZE;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		iFileSize = get_file_size(szFile);
		file = fopen(szFile, "rb");
		if (!file)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to open %s,err=%d", __func__, szFile, errno);
			printf("%s failed, err=%d, can't open file: %s\r\n", __func__, errno, szFile);
			goto Exit_WriteJffsLBA;
		}

		fseek(file, 0, SEEK_SET);

		iRet = pComm->RKU_ReadFlashInfo((uint_8 *)&info, NULL);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to read flash info,err=%d", __func__, iRet);
			}
			printf("ReadFlashInfo failed, err=%d\r\n", iRet);
			goto Exit_WriteJffsLBA;
		}

		if (uiBegin >= info.uiFlashSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Partition offset is larger than flash size, partition_offset=0x%08X,flash_size=0x%08X", __func__, uiBegin, info.uiFlashSize);
			}
			printf("Begin is larger than flash size\r\n");
			goto Exit_WriteJffsLBA;
		}
		if (uiPartSize == 0xFFFFFFFF)
		{
			uiPartSize = info.uiFlashSize - uiBegin;

			// if (info.uiFlashSize >= 0x800000)
			// { // exclude backup gpt
			// 	uiPartSize -= 64;
			// }
			// else
			// 	uiPartSize -= 33;
		}
		if ((uiBegin + uiPartSize) > info.uiFlashSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Partition ending is larger than flash size, partition_end=0x%08X,flash_size=0x%08X", __func__, uiBegin + uiPartSize, info.uiFlashSize);
			}
			printf("Partition ending is larger than flash size\r\n");
			goto Exit_WriteJffsLBA;
		}
		uiSize = iFileSize / SECTOR_SIZE;
		if (uiSize > uiPartSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->size of file is larger than partition size,image=%d,partition=%d", __func__, uiSize, uiPartSize);
			}
			printf("partition too small,image=%d,partition=%d\r\n", uiSize, uiPartSize);
			goto Exit_WriteJffsLBA;
		}

		if (g_pLogObject)
		{
			g_pLogObject->Record("Info:%s->start to erase before writing jffs image,offset=0x%x count=%d", __func__, uiBegin, uiPartSize);
		}
		printf("start to erase...0x%x,0x%x\r\n", uiBegin, uiPartSize);
		bRet = erase_partition(pComm, uiBegin, uiPartSize, 256);
		if (!bRet)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to erase jffs partition", __func__);
			}
			printf("failed to erase jffs block\r\n");
			goto Exit_WriteJffsLBA;
		}
		printf("erase done\r\n");
		if (g_pLogObject)
		{
			g_pLogObject->Record("Info:%s->start to write jffs, total_sector=%d", __func__, uiSize);
		}
		nPercent = nPrevPercent = 0;
		while (iTotalWrite < iFileSize)
		{
			memset(pBuf, 0, dwMaxReadWriteBytes);
			if ((iFileSize - iTotalWrite) >= dwMaxReadWriteBytes)
			{
				dwTransferBytes = dwMaxReadWriteBytes;
				uiTransferSec = dwMaxReadWriteBytes / SECTOR_SIZE;
			}
			else
			{
				dwTransferBytes = iFileSize - iTotalWrite;
				uiTransferSec = ((dwTransferBytes % SECTOR_SIZE == 0) ? (dwTransferBytes / SECTOR_SIZE) : (dwTransferBytes / SECTOR_SIZE + 1));
			}
			iRead = fread(pBuf, 1, dwTransferBytes, file);
			if (iRead != dwTransferBytes)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to read data from file in write step,err=%d,read=%d,ret=%d", __func__, errno, dwTransferBytes, iRead);
				}
				printf("Failed to read data from file,errno=%d!\r\n", errno);
				goto Exit_WriteJffsLBA;
			}

			iRet = pComm->RKU_WriteLBA(uiBegin, uiTransferSec, pBuf);
			if (ERR_SUCCESS == iRet)
			{
				iTotalWrite += dwTransferBytes;
				uiBegin += uiTransferSec;

				nPercent = ((double)iTotalWrite / iFileSize) * 100;
				if (nPercent > nPrevPercent)
				{
					if (nPrevPercent == 0)
						printf("Write LBA from file (%d%%)\r\n", nPercent);
					else
					{
						CURSOR_MOVEUP_LINE(1);
						CURSOR_DEL_LINE;
						printf("Write LBA from file (%d%%)\r\n", nPercent);
					}
					nPrevPercent = nPercent;
				}
			}
			else
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to write lba,Written(%dK),RetCode(%d)", __func__, iTotalWrite / 1024, iRet);
				}
				printf("Failed to write data into device,err=%d!\r\n", iRet);
				goto Exit_WriteJffsLBA;
			}
		}
		bSuccess = true;
	}
	else
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		printf("Write LBA quit, creating comm object failed!\r\n");
	}
Exit_WriteJffsLBA:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (file)
		fclose(file);
	return bSuccess;
}

bool write_lba(STRUCT_RKDEVICE_DESC &dev, uint_32 uiBegin, uint_32 uiPartSize, char *szFile)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	FILE *file = NULL;
	bool bRet, bSuccess = false;
	int iRet;
	uint_8 flashID[5];
	STRUCT_FLASHINFO_CMD info;
	uint_64 iTotalWrite = 0, iFileSize = 0;
	uint_32 iWrite = 0;
	uint_32 uiLen,uiSize;
	int nSectorSize = 512, nPercent, nPrevPercent;
	uint_8 *pBuf = NULL;
	string file_name = szFile;
	size_t pos;
	bool nand_storage = false;
	pos = file_name.find_last_of(PATH_DELIMITER);
	if (pos != string::npos)
		file_name = file_name.substr(pos + 1);
	if (g_pLogObject)
	{
		g_pLogObject->Record("Info:%s->Start to write %s at 0x%x...", __func__, file_name.c_str(), uiBegin);
	}
	printf("Start to write %s at 0x%x...\r\n", file_name.c_str(), uiBegin);

	pBuf = new uint_8[nSectorSize * DEFAULT_RW_LBA];
	if (!pBuf)
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to new memory,err=%d", __func__, errno);
		printf("Write LBA failed, err=%d, can't new buffer\r\n", errno);
		goto Exit_WriteLBA;
	}

	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		iRet = pComm->RKU_ReadFlashID(flashID);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to read flash ID,err=%d", __func__, iRet);
			}
			printf("Error:Write LBA failed,can't read flash id from device!\r\n");
			goto Exit_WriteLBA;
		}
		iRet = pComm->RKU_ReadFlashInfo((uint_8 *)&info, NULL);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to read flash info,err=%d", __func__, iRet);
			}
			printf("Error:Write LBA failed,can't read flash info from device!\r\n");
			goto Exit_WriteLBA;
		}

		if ((flashID[0] == 'S') && (flashID[1] == 'N') && (flashID[2] == 'A') && (flashID[3] == 'N') && (flashID[4] == 'D'))
			nand_storage = true;
		if (info.bECCBits > 0)
		{
			nand_storage = true;
		}
		if (nand_storage)
		{
			if ((uiBegin % info.usBlockSize) != 0)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->0x%x is not aligned with block size(0x%x)", __func__, uiBegin, info.usBlockSize);
				}
				printf("Error:Write LBA failed,0x%x is not aligned with block size(0x%x)!\r\n", uiBegin, info.usBlockSize);
				goto Exit_WriteLBA;
			}
		}
		iFileSize = get_file_size(szFile);
		file = fopen(szFile, "rb");
		if (!file)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to open %s,err=%d", __func__, szFile, errno);
			}
			printf("Write LBA failed, err=%d, can't open file: %s\r\n", errno, szFile);
			goto Exit_WriteLBA;
		}
		fseek(file, 0, SEEK_SET);
		if (uiBegin >= info.uiFlashSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Partition offset is larger than flash size, partition_offset=0x%08X,flash_size=0x%08X", __func__, uiBegin, info.uiFlashSize);
			}
			printf("Begin is larger than flash size\r\n");
			goto Exit_WriteLBA;
		}
		if (uiPartSize == 0xFFFFFFFF)
		{
			uiPartSize = info.uiFlashSize - uiBegin;

			// if (info.uiFlashSize >= 0x800000)
			// { // exclude backup gpt
			// 	uiPartSize -= 64;
			// }
			// else
			// 	uiPartSize -= 33;
		}
		if ((uiBegin + uiPartSize) > info.uiFlashSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Partition ending is larger than flash size, partition_end=0x%08X,flash_size=0x%08X", __func__, uiBegin + uiPartSize, info.uiFlashSize);
			}
			printf("Partition ending is larger than flash size\r\n");
			goto Exit_WriteLBA;
		}
		uiSize = iFileSize / SECTOR_SIZE;
		if (uiSize > uiPartSize)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->size of file is larger than partition size,image=%d,partition=%d", __func__, uiSize, uiPartSize);
			}
			printf("partition too small,image=%d,partition=%d\r\n", uiSize, uiPartSize);
			goto Exit_WriteLBA;
		}

		nPercent = nPrevPercent = 0;
		while (iTotalWrite < iFileSize)
		{
			memset(pBuf, 0, nSectorSize * DEFAULT_RW_LBA);
			iWrite = fread(pBuf, 1, nSectorSize * DEFAULT_RW_LBA, file);
			uiLen = ((iWrite % 512) == 0) ? (iWrite / 512) : (iWrite / 512 + 1);
			iRet = pComm->RKU_WriteLBA(uiBegin, uiLen, pBuf);
			if (ERR_SUCCESS == iRet)
			{
				uiBegin += uiLen;
				iTotalWrite += iWrite;
				nPercent = ((double)iTotalWrite / iFileSize) * 100;
				if (nPercent > nPrevPercent)
				{
					if (nPrevPercent == 0)
						printf("Write LBA from file (%d%%)\r\n", nPercent);
					else
					{
						CURSOR_MOVEUP_LINE(1);
						CURSOR_DEL_LINE;
						printf("Write LBA from file (%d%%)\r\n", nPercent);
					}
					nPrevPercent = nPercent;
				}
			}
			else
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:%s->Fail to write lba, err=%d", __func__, iRet);
				printf("Write LBA failed!\r\n");
				goto Exit_WriteLBA;
			}
		}
		bSuccess = true;
	}
	else
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		printf("Write LBA quit, creating comm object failed!\r\n");
	}
Exit_WriteLBA:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (file)
		fclose(file);
	if (pBuf)
		delete[] pBuf;
	return bSuccess;
}
bool write_disk(CRKDisk *disk, uint64_t uiBegin, char *szFile)
{
	FILE *file = NULL;
	bool bRet, bSuccess = false;
	uint64_t iTotalWrite = 0, iFileSize = 0;
	uint32_t iWrite = 0;
	uint32_t transfer_bulk_size;
	int nPercent, nPrevPercent;
	uint8_t *pBuf = NULL;
	string file_name = szFile;
	size_t pos;
	transfer_bulk_size = SECTOR_SIZE * DEFAULT_RW_LBA;

	pos = file_name.find_last_of(PATH_DELIMITER);
	if (pos != string::npos)
		file_name = file_name.substr(pos + 1);
	printf("Start to write %s at 0x%llx...\r\n", file_name.c_str(), uiBegin);

	pBuf = new uint8_t[transfer_bulk_size];
	if (!pBuf)
	{
		printf("Write disk failed, err=%d, can't new buffer\r\n", errno);
		goto Exit_WriteDisk;
	}

	file = fopen(szFile, "rb");
	if (!file)
	{
		printf("Write disk failed, err=%d, can't open file: %s\r\n", errno, szFile);
		goto Exit_WriteDisk;
	}

	fseek(file, 0, SEEK_END);
	iFileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	uiBegin *= SECTOR_SIZE;
	nPercent = nPrevPercent = 0;
	while (iTotalWrite < iFileSize)
	{
		memset(pBuf, 0, transfer_bulk_size);
		iWrite = fread(pBuf, 1, transfer_bulk_size, file);

		bRet = disk->WriteDisk(uiBegin, iWrite, pBuf);
		if (bRet)
		{
			uiBegin += iWrite;
			iTotalWrite += iWrite;
			nPercent = ((double)iTotalWrite / iFileSize) * 100;
			if (nPercent > nPrevPercent)
			{
				if (nPrevPercent == 0)
					printf("Write disk from file (%d%%)\r\n", nPercent);
				else
				{
					CURSOR_MOVEUP_LINE(1);
					CURSOR_DEL_LINE;
					printf("Write disk from file (%d%%)\r\n", nPercent);
				}
				nPrevPercent = nPercent;
			}
		}
		else
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error: WriteDisk failed, err=%d", disk->GetLastDiskError());
			printf("Write disk failed!\r\n");
			goto Exit_WriteDisk;
		}
	}
	bSuccess = true;

Exit_WriteDisk:
	if (file)
		fclose(file);
	if (pBuf)
		delete[] pBuf;
	return bSuccess;
}
bool erase_lba(STRUCT_RKDEVICE_DESC &dev, uint_32 uiBegin, uint_32 uiLen)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	int iRet, nPercent, nPrevPercent, nEraseCount;
	int iTotalErase = 0, iErase = 0;
	unsigned int uiEraseOnce = MAX_ERASE_LBA;
	uint_8 flashID[5];
	uint_32 uiReadCount;
	STRUCT_FLASHINFO_CMD info;

	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		if ((uiBegin == 0) && (uiLen == (uint_32)-1))
		{
			iRet = pComm->RKU_ReadFlashInfo((uint_8 *)&info, &uiReadCount);
			if (iRet != ERR_SUCCESS)
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:%s->Fail to read flash info,err=%d", __func__, iRet);
				ERROR_COLOR_ATTR;
				printf("Error:Erase LBA failed!");
				NORMAL_COLOR_ATTR;
				printf("\r\n");
				goto Exit_EraseLBA;
			}
			uiLen = info.uiFlashSize;
		}
		iRet = pComm->RKU_ReadFlashID(flashID);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to read flash ID,err=%d", __func__, iRet);
			}
			ERROR_COLOR_ATTR;
			printf("Error:Erase LBA failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			goto Exit_EraseLBA;
		}

		if ((flashID[0] == 'N') && (flashID[1] == 'O') && (flashID[2] == 'R'))
			uiEraseOnce = 256;
		nPercent = nPrevPercent = 0;
		nEraseCount = uiLen;
		printf("Start to erase lba begin(%x) count(%x)...\r\n", uiBegin, uiLen);

		while (uiLen > 0)
		{
			iErase = (uiLen >= uiEraseOnce) ? uiEraseOnce : uiLen;
			iRet = pComm->RKU_EraseLBA(uiBegin + iTotalErase, iErase);
			if (ERR_SUCCESS == iRet)
			{
				uiLen -= iErase;
				iTotalErase += iErase;

				nPercent = ((double)iTotalErase / nEraseCount) * 100;
				if (nPercent > nPrevPercent)
				{
					if (nPrevPercent == 0)
						printf("Erase LBA (%d%%)\r\n", nPercent);
					else
					{
						CURSOR_MOVEUP_LINE(1);
						CURSOR_DEL_LINE;
						printf("Erase LBA (%d%%)\r\n", nPercent);
					}
					nPrevPercent = nPercent;
				}
			}
			else
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:%s->Fail to erase lba,err=%d", __func__, iRet);
				ERROR_COLOR_ATTR;
				printf("Error:Erase LBA failed!");
				NORMAL_COLOR_ATTR;
				printf("\r\n");
				goto Exit_EraseLBA;
			}
		}
		bSuccess = true;
		printf("Erase LBA ok.\r\n");
	}
	else
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		ERROR_COLOR_ATTR;
		printf("Erase LBA quit,Creating comm object failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
	}
Exit_EraseLBA:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	return bSuccess;
}

bool erase_block(STRUCT_RKDEVICE_DESC &dev, uint_8 cs, uint_32 uiBegin, uint_32 uiLen, bool bForce)
{
	if (!check_device_type(dev, RKUSB_LOADER))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	int iRet, nPercent, nPrevPercent = 0, nEraseCount = uiLen;
	int iTotalErase = 0, iErase = 0;
	uint_8 eraseType;
	eraseType = bForce ? ERASE_FORCE : ERASE_NORMAL;
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		printf("Start to erase block begin(%x) count(%x)...\r\n", uiBegin, uiLen);
		while (uiLen > 0)
		{
			iErase = (uiLen >= MAX_ERASE_BLOCKS) ? MAX_ERASE_BLOCKS : uiLen;
			iRet = pComm->RKU_EraseBlock(cs, uiBegin + iTotalErase, iErase, eraseType);
			if (ERR_SUCCESS == iRet)
			{
				uiLen -= iErase;
				iTotalErase += iErase;

				nPercent = ((double)iTotalErase / nEraseCount) * 100;
				if (nPercent > nPrevPercent)
				{
					if (nPrevPercent == 0)
						printf("Erase Block (%d%%)\r\n", nPercent);
					else
					{
						CURSOR_MOVEUP_LINE(1);
						CURSOR_DEL_LINE;
						printf("Erase Block (%d%%)\r\n", nPercent);
					}
					nPrevPercent = nPercent;
				}
			}
			else
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:RKU_EraseBlock failed,err=%d", iRet);
				ERROR_COLOR_ATTR;
				printf("Error:Erase Block failed!");
				NORMAL_COLOR_ATTR;
				printf("\r\n");
				goto Exit_EraseBlock;
			}
		}
		bSuccess = true;
	}
	else
	{
		ERROR_COLOR_ATTR;
		printf("Erase Block quit,Creating comm object failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
	}
Exit_EraseBlock:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	return bSuccess;
}
bool read_com_log(STRUCT_RKDEVICE_DESC &dev, char *szFile)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	FILE *file = NULL;
	bool bRet, bSuccess = false;
	uint_32 tag, logSize;
	int iRet;
	uint_8 pBuf[SECTOR_SIZE * 256];
	read_capability(dev);
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		if (szFile)
		{
			file = fopen(szFile, "wb+");
			if (!file)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to create %s,err=%d", __func__, szFile, errno);
				}
				ERROR_COLOR_ATTR;
				printf("Read Com Log failed,err=%d,can't open file: %s", errno, szFile);
				NORMAL_COLOR_ATTR;
				printf("\r\n");
				goto Exit_ReadComLog;
			}
		}

		if (!g_bReadComLog)
		{
			ERROR_COLOR_ATTR;
			printf("Read Com Log failed,device doesn't have the feature");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			goto Exit_ReadComLog;
		}
		iRet = pComm->RKU_ReadComData(pBuf);
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to read com log,err=%d", __func__, iRet);
			ERROR_COLOR_ATTR;
			printf("Error:Read Com Log failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			goto Exit_ReadComLog;
		}
		tag = *((uint_32 *)pBuf);
		if (tag != 0x55424544)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Log format is invalid", __func__);
			}
			ERROR_COLOR_ATTR;
			printf("Error:Format of log is wrong!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			goto Exit_ReadComLog;
		}
		logSize = *(uint_32 *)(pBuf + 4);
		fwrite(pBuf + 8, 1, logSize - 8, file);
		bSuccess = true;
		printf("Reading com Log ok,save at %s\r\n", szFile);
	}
	else
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object", __func__);
		ERROR_COLOR_ATTR;
		printf("Read Com Log quit,Creating comm object failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
	}
Exit_ReadComLog:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (file)
		fclose(file);
	return bSuccess;
}
void list_storage(STRING_VECTOR &vecTitle, STORAGE_CODE curStorage)
{
	int i, cnt;
	string strCurStor;
	cnt = vecTitle.size();
	strCurStor = get_storage_string(curStorage);
	for (i = 0; i < cnt; i++)
	{
		if (strCurStor == vecTitle[i])
			printf("No=%d\t%s(*)\r\n", i + 1, vecTitle[i].c_str());
		else
			printf("No=%d\t%s\r\n", i + 1, vecTitle[i].c_str());
	}
}

bool switch_usb3(STRUCT_RKDEVICE_DESC &dev)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	if (dev.emUsbSpeed == SUPER_SPEED) {
		printf("Current device is usb3.\r\n");
		return true;
	}
	CRKUsbComm *pComm = NULL;
	CRKScan* pScan = NULL;
	bool bRet, bSuccess = false;
	int iRet;
	char buf[10];
	
	read_capability(dev);
	if (!g_bSwitchUsb3) {
		ERROR_COLOR_ATTR;
		printf("device doesn't have the feature,please do rcb command firstly");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		goto Exit_SwitchUsb3;
	}
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (!bRet) {
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		ERROR_COLOR_ATTR;
		printf("SwitchUsb3 quit,Creating comm object failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
	}
	pScan = new CRKScan();
	if (!pScan)
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create scan object!", __func__);
		ERROR_COLOR_ATTR;
		printf("SwitchUsb3 quit,Creating scan object failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
	}
	pScan->SetVidPid(g_usMscVid, g_usMscPid);
	pScan->AddRockusbVidPid(g_usDevNewVid, g_usDevNewPid, g_usDevOldVid, g_usDevOldPid);

	sprintf(buf, "%08X", dev.uiLocationID);

	iRet = pComm->RKU_SwitchUsb3(buf);
	if (iRet != ERR_SUCCESS)
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to switch usb3,err=%d", __func__, iRet);
		ERROR_COLOR_ATTR;
		printf("Error:Switch usb3 failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		goto Exit_SwitchUsb3;
	}
	dev.strSerial = buf;
	bRet = pScan->Wait2(dev, dev.emUsbType, dev.usVid, dev.usPid);
	if (!bRet) {
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to wait usb3,err=%d", __func__, iRet);
		ERROR_COLOR_ATTR;
		printf("Error:Wait usb3 connect failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		goto Exit_SwitchUsb3;	
	}
	if (dev.emUsbSpeed != SUPER_SPEED) {
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->reconnected device is not at usb3", __func__);
		ERROR_COLOR_ATTR;
		printf("Error:Switch usb3 failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		goto Exit_SwitchUsb3;
	}
	bSuccess = true;
	printf("Switch usb3 ok.\r\n");
Exit_SwitchUsb3:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	if (pScan)
	{
		delete pScan;
		pScan = NULL;
	}
	return bSuccess;
}

bool switch_storage(STRUCT_RKDEVICE_DESC &dev, int nSelect = -1)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	int iRet, cnt;
	STORAGE_CODE curStorage, selStorage;
	uint_8 pBuf[8];
	char szUserInput[32];
	STRING_VECTOR vecStorage;
	read_capability(dev);
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		if (!g_bSwitchStorage)
		{
			ERROR_COLOR_ATTR;
			printf("device doesn't have the feature,please do rcb command firstly");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			goto Exit_SwitchStorage;
		}
		vecStorage.push_back("FLASH");
		vecStorage.push_back("EMMC");
		vecStorage.push_back("SD");
		vecStorage.push_back("SD1");
		vecStorage.push_back("SPINOR");
		vecStorage.push_back("SPINAND");
		vecStorage.push_back("RAM");
		vecStorage.push_back("USB");
		vecStorage.push_back("SATA");
		vecStorage.push_back("PCIE");
		vecStorage.push_back("UFS");
		vecStorage.push_back("RVD");

		if (nSelect == -1)
		{
			printf("List of supported storage\r\n");
			bRet = get_device_storage(pComm, pBuf, curStorage);
			if (!bRet)
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:%s->Fail to get device storage", __func__);
				ERROR_COLOR_ATTR;
				printf("Error:Read Storage list failed!");
				NORMAL_COLOR_ATTR;
				printf("\r\n");
				goto Exit_SwitchStorage;
			}

			list_storage(vecStorage, curStorage);
			cnt = vecStorage.size();
			nSelect = 0;

			do
			{
				printf("Input No to switch,Quit press <Q>:");
				fgets(szUserInput, 32, stdin);
				if (stricmp(szUserInput, "Q") == 0)
				{
					goto Exit_SwitchStorage;
				}
				iRet = sscanf(szUserInput, "%d", &nSelect);
				if (iRet == 1) {
					if ((nSelect > 0) && (nSelect <= cnt))
					{
						break;
					}
				}
			} while (1);
		}

		nSelect -= 1;
		selStorage = (STORAGE_CODE)get_storage_value(vecStorage[nSelect]);
		iRet = pComm->RKU_SwitchStorage(get_storage_index((uint_32)selStorage));
		if (iRet != ERR_SUCCESS)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to switch storage,err=%d", __func__, iRet);
			ERROR_COLOR_ATTR;
			printf("Error:Switch Storage failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			goto Exit_SwitchStorage;
		}
		bRet = get_device_storage(pComm, pBuf, curStorage);
		if (!bRet)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to get device storage After switching", __func__);
			ERROR_COLOR_ATTR;
			printf("Error:Read Storage list failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			goto Exit_SwitchStorage;
		}
		if (curStorage != selStorage)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to check Storage,storage=%s,switch=%s", __func__, get_storage_string((uint_32)curStorage).c_str(), vecStorage[nSelect].c_str());
			ERROR_COLOR_ATTR;
			printf("Error:Switch Storage failed!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			goto Exit_SwitchStorage;
		}

		bSuccess = true;
		printf("Switch %s ok.\r\n", vecStorage[nSelect].c_str());
	}
	else
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object!", __func__);
		ERROR_COLOR_ATTR;
		printf("SwitchStorage quit,Creating comm object failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
	}
Exit_SwitchStorage:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}
	return bSuccess;
}

bool run_system(STRUCT_RKDEVICE_DESC &dev, uint_32 uiUbootAddr, uint_32 uiTrustAddr, uint_32 uiBootAddr, char *szUboot, char *szTrust, char *szBoot)
{
	if (!check_device_type(dev, RKUSB_LOADER | RKUSB_MASKROM))
		return false;
	CRKUsbComm *pComm = NULL;
	bool bRet, bSuccess = false;
	int iRet;
	RUN_PARAM_HEAD rpm;
	memset((uint_8 *)&rpm, 0, sizeof(rpm));
	rpm.magic = *(uint_32 *)RUN_PARAM_MAGIC;
	rpm.param_count = 3;
	rpm.param[0].type = IMAGE_UBOOT;
	rpm.param[0].address = uiUbootAddr * SECTOR_SIZE;
	rpm.param[0].size = get_file_size(szUboot);
	if (rpm.param[0].size == (uint_32)-1)
	{
		printf("Failed to get size of uboot!\r\n");
		goto Exit_RunSys;
	}
	rpm.param[1].type = IMAGE_TRUST;
	rpm.param[1].address = uiTrustAddr * SECTOR_SIZE;
	rpm.param[1].size = get_file_size(szTrust);
	if (rpm.param[1].size == (uint_32)-1)
	{
		printf("Failed to get size of trust!\r\n");
		goto Exit_RunSys;
	}
	rpm.param[2].type = IMAGE_BOOT;
	rpm.param[2].address = uiBootAddr * SECTOR_SIZE;
	rpm.param[2].size = get_file_size(szBoot);
	if (rpm.param[2].size == (uint_32)-1)
	{
		printf("Failed to get size of boot!\r\n");
		goto Exit_RunSys;
	}
	pComm = new CRKUsbComm(dev, g_pLogObject, bRet);
	if (bRet)
	{
		printf("start to run system...\r\n");
		iRet = pComm->RKU_WriteSDRam(sizeof(rpm), (uint_8 *)&rpm);
		if (ERR_SUCCESS != iRet)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to write sdram, err=%d", __func__, iRet);
			ERROR_COLOR_ATTR;
			printf("Failed to send param!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			goto Exit_RunSys;
		}

		iRet = pComm->RKU_RunSDRam();
		if (ERR_SUCCESS != iRet)
		{
			if (g_pLogObject)
				g_pLogObject->Record("Error:%s->Fail to run sdram, err=%d", __func__, iRet);
			ERROR_COLOR_ATTR;
			printf("Failed to run system!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			goto Exit_RunSys;
		}
		printf("run system ok.\r\n");

		bSuccess = true;
	}
	else
	{
		if (g_pLogObject)
			g_pLogObject->Record("Error:%s->Fail to create comm object", __func__);
		ERROR_COLOR_ATTR;
		printf("Run system quit,Creating comm object failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
	}
Exit_RunSys:
	if (pComm)
	{
		delete pComm;
		pComm = NULL;
	}

	return bSuccess;
}

void split_item(STRING_VECTOR &vecItems, char *pszItems)
{
	string strItem;
	char szItem[100];
	char *pos = NULL, *pStart;
	pStart = pszItems;
	pos = strchr(pStart, ',');
	while (pos != NULL)
	{
		memset(szItem, 0, 100);
		strncpy(szItem, pStart, pos - pStart);
		strItem = szItem;
		vecItems.push_back(strItem);
		pStart = pos + 1;
		if (*pStart == NULL)
			break;
		pos = strchr(pStart, ',');
	}
	if (strlen(pStart) > 0)
	{
		memset(szItem, 0, 100);
		strncpy(szItem, pStart, strlen(pStart));
		strItem = szItem;
		vecItems.push_back(strItem);
	}
}
bool create_gpt_by_parameter(char *param_in, char *gpt_out)
{
	FILE *fout = NULL;
	bool bSuccess = false, bRet, bGrowAlign;
	unsigned int uiGptSize;
	PARAM_ITEM_VECTOR vecItems;
	CONFIG_ITEM_VECTOR vecUuids;
	char *pParamBuf = NULL;
	uint_8 master_gpt[34 * SECTOR_SIZE];
	printf("in=%s,out=%s\r\n", param_in, gpt_out);
	if (!ReadFileToBuffer(param_in, pParamBuf))
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed,can't open or read %s\r\n", __func__, param_in);
		ERROR_COLOR_ATTR;
		printf("Fail to open or read %s!", param_in);
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		goto Exit_Create_Gpt;
	}
	if (!get_parameter_gptflag(pParamBuf))
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed,can't get gpt flag\r\n", __func__);
		ERROR_COLOR_ATTR;
		printf("No Found Gpt Line in the parameter!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		goto Exit_Create_Gpt;
	}
	uiGptSize = get_parameter_gpt_size(pParamBuf);

	bRet = parse_parameter(pParamBuf, vecItems);
	if (!bRet)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed,can't find mtdparts line\r\n", __func__);
		ERROR_COLOR_ATTR;
		printf("Fail to find mtdparts in the parameter!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		goto Exit_Create_Gpt;
	}
	get_uuid_from_parameter(pParamBuf, vecUuids);
	bRet = get_grow_align_flag(pParamBuf, bGrowAlign);
	if (bRet)
		create_gpt_buffer(master_gpt, vecItems, vecUuids, uiGptSize, 1, &bGrowAlign);
	else
		create_gpt_buffer(master_gpt, vecItems, vecUuids, uiGptSize,1);
	fout = fopen(gpt_out, "wb+");
	if (!fout)
	{
		if (g_pLogObject)
			g_pLogObject->Record("%s failed,can't create %s,err=%d\r\n", __func__, gpt_out, errno);
		ERROR_COLOR_ATTR;
		printf("Create gpt image failed,err=%d!", errno);
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		goto Exit_Create_Gpt;
	}
	fwrite(master_gpt, 1, sizeof(master_gpt), fout);
	bSuccess = true;
	printf("Create Gpt Image OK.\r\n");

Exit_Create_Gpt:
	if (fout)
		fclose(fout);
	if (pParamBuf)
		delete[] pParamBuf;
	return bSuccess;
}
bool IsSparseEntry(CRKImage *pImage, STRUCT_RKIMAGE_ITEM &entry)
{
	bool bRet;
	sparse_header header;
	uint_32 dwFWOffset;
	uint_64 ulEntryStartOffset;
	dwFWOffset = pImage->FWOffset;
	if (entry.file[50] == 'H')
	{
		ulEntryStartOffset = *((uint_32 *)(&entry.file[51]));
		ulEntryStartOffset <<= 32;
		ulEntryStartOffset += entry.offset;
		ulEntryStartOffset += pImage->FWOffset;
	}
	else
	{
		ulEntryStartOffset = pImage->FWOffset;
		ulEntryStartOffset += entry.offset;
	}
	bRet = pImage->GetData(ulEntryStartOffset, sizeof(header), (uint_8 *)&header);
	if (!bRet)
	{
		return false;
	}
	if (header.magic != SPARSE_HEADER_MAGIC)
	{
		return false;
	}
	return true;
}
bool IsUbiEntry(CRKImage *pImage, STRUCT_RKIMAGE_ITEM &entry)
{
	bool bRet;
	uint_32 magic;
	uint_32 dwFWOffset;
	uint_64 ulEntryStartOffset;
	dwFWOffset = pImage->FWOffset;
	if (entry.file[50] == 'H')
	{
		ulEntryStartOffset = *((uint_32 *)(&entry.file[51]));
		ulEntryStartOffset <<= 32;
		ulEntryStartOffset += entry.offset;
		ulEntryStartOffset += pImage->FWOffset;
	}
	else
	{
		ulEntryStartOffset = pImage->FWOffset;
		ulEntryStartOffset += entry.offset;
	}
	bRet = pImage->GetData(ulEntryStartOffset, sizeof(uint_32), (uint_8 *)&magic);
	if (!bRet)
	{
		return false;
	}
	if (magic != UBI_HEADER_MAGIC)
	{
		return false;
	}
	return true;
}
void print_update_loader_meta(CRKBoot *pBoot)
{
	uint_8 btMajor, btMinor;
	char szText[100];
	uint8_t i;
	uint32_t uiSize, uiDelay;
	bool bRet;
	char szName[20];
	btMajor = ((pBoot->Version) & 0x0000FF00) >> 8;
	btMinor = ((pBoot->Version) & 0x000000FF);
	sprintf(szText, "%u.%02u", btMajor, btMinor);
	printf("Loader ver:%s\t", szText);

	sprintf(szText, "%04u-%02u-%02u %02u:%02u:%02u", ((STRUCT_RKTIME)pBoot->ReleaseTime).usYear, ((STRUCT_RKTIME)pBoot->ReleaseTime).ucMonth,
			((STRUCT_RKTIME)pBoot->ReleaseTime).ucDay, ((STRUCT_RKTIME)pBoot->ReleaseTime).ucHour, ((STRUCT_RKTIME)pBoot->ReleaseTime).ucMinute, ((STRUCT_RKTIME)pBoot->ReleaseTime).ucSecond);
	printf("Loader Time:%s\r\n", szText);

	if (pBoot->SignFlag)
		printf("Loader Sign:true\r\n");
	else
		printf("Loader Sign:false\r\n");

	if (pBoot->BootRc4Off)
		printf("Boot Rc4:false\r\n");
	else
		printf("Boot Rc4:true\r\n");

	if (pBoot->Rc4DisableFlag)
		printf("Loader Code Rc4:false\r\n");
	else
		printf("Loader Code Rc4:true\r\n");

	printf("471 Count:%d\r\n", pBoot->GetEntry471Count());
	for (i = 0; i < pBoot->Entry471Count; i++)
	{
		pBoot->GetEntryProperty(ENTRY471, i, uiSize, uiDelay);
		pBoot->GetEntryName(ENTRY471, i, szName);
		printf("    EntryNo=%d;    Name=%s;    Size=0x%x;\r\n", i, szName, uiSize);
	}
	printf("472 Count:%d\r\n", pBoot->GetEntry472Count());
	for (i = 0; i < pBoot->Entry472Count; i++)
	{
		pBoot->GetEntryProperty(ENTRY472, i, uiSize, uiDelay);
		pBoot->GetEntryName(ENTRY472, i, szName);
		printf("    EntryNo=%d;    Name=%s;    Size=0x%x;\r\n", i, szName, uiSize);
	}
	printf("loader Count:%d\r\n", pBoot->GetEntryLoaderCount());
	for (i = 0; i < pBoot->EntryLoaderCount; i++)
	{
		pBoot->GetEntryProperty(ENTRYLOADER, i, uiSize, uiDelay);
		pBoot->GetEntryName(ENTRYLOADER, i, szName);
		printf("    EntryNo=%d;    Name=%s;    Size=0x%x;\r\n", i, szName, uiSize);
	}
}
bool extract_update_fw(CRKImage *pImage, char *szOut)
{
	FILE *fout = NULL;
	uint8_t *pbuf = NULL;
	size_t nBufferSize = 256 * 1024;
	uint64_t nTotalWrite, nEntrySize, nEntryOffset;
	char szOutFile[256];
	string strEntryFile;
	int ret;
	size_t iPos;
	uint32_t i, j, nPrevPercent, nPercent, nWrite, nRead;
	STRUCT_RKIMAGE_HDR rkImageHead;
	STRUCT_RKIMAGE_ITEM *pDownloadItem;
	bool bRet;
	uint_8 headerData[64 * 1024];
	uint_8 *pHeaderData = headerData;
	int iHeadSize, iHeadMetaSize;
	bool bSuccess = false;

	pbuf = new uint8_t[nBufferSize];
	if (!pbuf)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to new memory,size=0x%x,err=%d!", __func__, nBufferSize, errno);
		}
		printf("Failed to new memory,err=%d\r\n", errno);
		goto exit_extract_update;
	}
	iHeadSize = sizeof(STRUCT_RKIMAGE_HDR);
	bRet = pImage->GetData(pImage->FWOffset, iHeadSize, (uint_8 *)&rkImageHead);
	if (!bRet)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to GetData for header!", __func__);
		}
		printf("Failed to get header of firmware\r\n");
		goto exit_extract_update;
	}
	if ((rkImageHead.manufacturer[56] == 0x55) && (rkImageHead.manufacturer[57] == 0x66))
	{
		uint_16 *pItemRemain;
		pItemRemain = (uint_16 *)(&rkImageHead.manufacturer[58]);
		rkImageHead.item_count += *pItemRemain;
	}
	iHeadMetaSize = iHeadSize = sizeof(STRUCT_RKIMAGE_HDR) - sizeof(STRUCT_RKIMAGE_ITEM) * MAX_PACKAGE_FILES;
	iHeadSize += rkImageHead.item_count * sizeof(STRUCT_RKIMAGE_ITEM);
	iHeadSize = CALC_UNIT(iHeadSize, PAGE_SIZE) * PAGE_SIZE;

	bRet = pImage->GetData(pImage->FWOffset, iHeadSize, (uint_8 *)pHeaderData);
	if (!bRet)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to GetData for entrys!", __func__);
		}
		printf("Failed to get entrys of firmware\r\n");
		goto exit_extract_update;
	}
	sprintf(szOutFile, "%s%cbootloader.bin", szOut, PATH_DELIMITER);
	printf("start to extract bootloader...");
	bRet = pImage->SaveBootFile(szOutFile);
	if (!bRet)
	{
		printf("failed\r\n");
		goto exit_extract_update;
	}
	printf("ok\r\n");
	printf("Entry Count:%u\r\n", rkImageHead.item_count);
	for (i = 0; i < rkImageHead.item_count; i++)
	{
		pDownloadItem = (STRUCT_RKIMAGE_ITEM *)(pHeaderData + iHeadMetaSize + i * sizeof(STRUCT_RKIMAGE_ITEM));
		if (pDownloadItem->file[50] == 'H')
		{
			nEntryOffset = *((uint_32 *)(&pDownloadItem->file[51]));
			nEntryOffset <<= 32;
			nEntryOffset += pDownloadItem->offset;
			nEntryOffset += pImage->FWOffset;
		}
		else
		{
			nEntryOffset = pImage->FWOffset;
			nEntryOffset += pDownloadItem->offset;
		}

		if (pDownloadItem->file[55] == 'H')
		{
			nEntrySize = *((uint_32 *)(&pDownloadItem->file[56]));
			nEntrySize <<= 32;
			nEntrySize += pDownloadItem->size;
		}
		else
			nEntrySize = pDownloadItem->size;

		if (stricmp(pDownloadItem->name, PARTNAME_PARAMETER) == 0)
		{
			nEntryOffset += 8;
			nEntrySize -= 12;
		}

		strEntryFile = pDownloadItem->file;
		replace(strEntryFile.begin(), strEntryFile.end(), PATH_DELIMITER_CS, PATH_DELIMITER);
		iPos = strEntryFile.find_last_of(PATH_DELIMITER);
		if (iPos != string::npos)
		{
			strEntryFile = strEntryFile.substr(iPos + 1);
		}
		sprintf(szOutFile, "%s%c%s", szOut, PATH_DELIMITER, strEntryFile.c_str());
		printf("start to extract %s...\r\n", strEntryFile.c_str());
		fout = fopen(szOutFile, "wb");
		if (!fout)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to create %s,err=%d!", __func__, szOutFile, errno);
			}
			printf("Failed to create %s,err=%d\r\n", strEntryFile.c_str(), errno);
			goto exit_extract_update;
		}

		nPercent = nPrevPercent = 0;
		nTotalWrite = 0;

		while (nTotalWrite < nEntrySize)
		{
			memset(pbuf, 0, nBufferSize);
			if ((nEntrySize - nTotalWrite) >= nBufferSize)
				nWrite = nBufferSize;
			else
				nWrite = nEntrySize - nTotalWrite;
			bRet = pImage->GetData(nEntryOffset + nTotalWrite, nWrite, pbuf);

			if (bRet)
			{
				fwrite(pbuf, 1, nWrite, fout);
				nTotalWrite += nWrite;
				nPercent = ((double)nTotalWrite / nEntrySize) * 100;
				if (nPercent > nPrevPercent)
				{
					CURSOR_MOVEUP_LINE(1);
					CURSOR_DEL_LINE;
					printf("start to extract %s...(%d%%)\r\n", strEntryFile.c_str(), nPercent);
					nPrevPercent = nPercent;
				}
			}
			else
			{
				if (g_pLogObject)
					g_pLogObject->Record("Error:%s->Fail to read, has_write=0x%llx err=%d", __func__, nTotalWrite, errno);
				printf("Failed to read,err=%d\r\n", errno);
				goto exit_extract_update;
			}
		}
		fclose(fout);
		fout = NULL;
	}
	bSuccess = true;
exit_extract_update:
	if (fout)
		fclose(fout);
	if (pbuf)
		delete[] pbuf;
	return bSuccess;
}
void print_update_fw_meta(CRKImage *pImage, uint64_t ullImageOffset)
{
	string strTemp;
	char szText[100];
	char szChip[7];
	uint_8 btMajor, btMinor;
	uint_16 usSmall;
	ENUM_RKDEVICE_TYPE type;
	STRUCT_RKIMAGE_HDR rkImageHead;
	STRUCT_RKIMAGE_ITEM *pDownloadItem;
	bool bRet;
	uint_8 headerData[64 * 1024];
	uint_8 *pHeaderData = headerData;
	int iHeadSize, iHeadMetaSize;

	type = pImage->SupportDevice;
	memset(szChip, 0, 7);
	uint_8 *pValue = (uint_8 *)(&type);

	szChip[0] = (char)(pValue[3]);
	szChip[1] = (char)(pValue[2]);
	szChip[2] = (char)(pValue[1]);
	szChip[3] = (char)(pValue[0]);
	strTemp = szChip;

	printf("Chip Tag:%s\t", strTemp.c_str());

	btMajor = ((pImage->Version) & 0xFF000000) >> 24;
	btMinor = ((pImage->Version) & 0x00FF0000) >> 16;
	usSmall = (pImage->Version) & 0x0000FFFF;
	sprintf(szText, "%x.%x.%02x", btMajor, btMinor, usSmall);
	printf("Version:%s\t", szText);

	sprintf(szText, "%04u-%02u-%02u %02u:%02u:%02u", ((STRUCT_RKTIME)pImage->ReleaseTime).usYear, ((STRUCT_RKTIME)pImage->ReleaseTime).ucMonth, ((STRUCT_RKTIME)pImage->ReleaseTime).ucDay,
			((STRUCT_RKTIME)pImage->ReleaseTime).ucHour, ((STRUCT_RKTIME)pImage->ReleaseTime).ucMinute, ((STRUCT_RKTIME)pImage->ReleaseTime).ucSecond);
	printf("Build Time:%s\t", szText);

	if (pImage->SignFlag)
		printf("Sign:true\r\n");
	else
		printf("Sign:false\r\n");

	btMajor = ((pImage->m_bootObject->Version) & 0x0000FF00) >> 8;
	btMinor = ((pImage->m_bootObject->Version) & 0x000000FF);
	sprintf(szText, "%u.%02u", btMajor, btMinor);
	printf("Loader ver:%s\t", szText);

	sprintf(szText, "%04u-%02u-%02u %02u:%02u:%02u", ((STRUCT_RKTIME)pImage->m_bootObject->ReleaseTime).usYear, ((STRUCT_RKTIME)pImage->m_bootObject->ReleaseTime).ucMonth,
			((STRUCT_RKTIME)pImage->m_bootObject->ReleaseTime).ucDay, ((STRUCT_RKTIME)pImage->m_bootObject->ReleaseTime).ucHour, ((STRUCT_RKTIME)pImage->m_bootObject->ReleaseTime).ucMinute, ((STRUCT_RKTIME)pImage->m_bootObject->ReleaseTime).ucSecond);
	printf("Loader Time:%s\r\n", szText);

	if (pImage->m_bootObject->SignFlag)
		printf("Loader Sign:true\r\n");
	else
		printf("Loader Sign:false\r\n");

	iHeadSize = sizeof(STRUCT_RKIMAGE_HDR);
	bRet = pImage->GetData(pImage->FWOffset, iHeadSize, (uint_8 *)&rkImageHead);
	if (!bRet)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to GetData for header!", __func__);
		}
		printf("Failed to get header of firmware\r\n");
		return;
	}
	if ((rkImageHead.manufacturer[56] == 0x55) && (rkImageHead.manufacturer[57] == 0x66))
	{
		uint_16 *pItemRemain;
		pItemRemain = (uint_16 *)(&rkImageHead.manufacturer[58]);
		rkImageHead.item_count += *pItemRemain;
	}
	iHeadMetaSize = iHeadSize = sizeof(STRUCT_RKIMAGE_HDR) - sizeof(STRUCT_RKIMAGE_ITEM) * MAX_PACKAGE_FILES;
	iHeadSize += rkImageHead.item_count * sizeof(STRUCT_RKIMAGE_ITEM);
	iHeadSize = CALC_UNIT(iHeadSize, PAGE_SIZE) * PAGE_SIZE;

	bRet = pImage->GetData(pImage->FWOffset, iHeadSize, (uint_8 *)pHeaderData);
	if (!bRet)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:%s->Fail to GetData for entrys!", __func__);
		}
		printf("Failed to get entrys of firmware\r\n");
		return;
	}
	printf("Entry Count:%u\r\n", rkImageHead.item_count);

	unsigned int i;
	uint_64 ullItemSize, ullItemOffset;
	string strItemName, strItemFile, strItemImageType;
	for (i = 0; i < rkImageHead.item_count; i++)
	{
		pDownloadItem = (STRUCT_RKIMAGE_ITEM *)(pHeaderData + iHeadMetaSize + i * sizeof(STRUCT_RKIMAGE_ITEM));
		if (pDownloadItem->file[50] == 'H')
		{
			ullItemOffset = *((uint_32 *)(&pDownloadItem->file[51]));
			ullItemOffset <<= 32;
			ullItemOffset += pDownloadItem->offset;
			ullItemOffset += pImage->FWOffset;
		}
		else
		{
			ullItemOffset = pImage->FWOffset;
			ullItemOffset += pDownloadItem->offset;
		}
		ullItemOffset += ullImageOffset;
		if (pDownloadItem->file[55] == 'H')
		{
			ullItemSize = *((uint_32 *)(&pDownloadItem->file[56]));
			ullItemSize <<= 32;
			ullItemSize += pDownloadItem->size;
		}
		else
			ullItemSize = pDownloadItem->size;

		strItemFile = pDownloadItem->file;
		strItemImageType = "";
		if (pDownloadItem->flash_offset != 0xFFFFFFFF)
		{
			strItemName = pDownloadItem->name;
			if (strcmp(pDownloadItem->name, PARTNAME_PARAMETER) != 0)
			{
				if (IsSparseEntry(pImage, *pDownloadItem))
				{
					strItemImageType = "sparse image";
				}
				else if (IsUbiEntry(pImage, *pDownloadItem))
				{
					strItemImageType = "ubi image";
				}
				else
					strItemImageType = "image";
			}
			else
				strItemImageType = PARTNAME_PARAMETER;
		}
		if (strItemName.size() == 0)
		{
			printf("    EntryNo=%-2d;    file=%s;    offset=0x%llx;    size=0x%llx\r\n",
				   i,
				   strItemFile.c_str(),
				   ullItemOffset,
				   ullItemSize);
		}
		else
		{
			printf("    EntryNo=%-2d;    file=%s;    partition=%s;    type=%s;    offset=0x%llx;    size=0x%llx\r\n",
				   i,
				   strItemFile.c_str(),
				   strItemName.c_str(),
				   strItemImageType.c_str(),
				   ullItemOffset,
				   ullItemSize);
		}
	}
}
bool show_firmware_info(char *szFirmware, int nEntry = -1)
{
	STORAGE_FW_HDR hdr;
	STORAGE_ENTRY entry;
	vector<STORAGE_ENTRY> vecEntrys;
	FILE *file = NULL;
	CRKImage *pImage = NULL;
	CRKBoot *pBoot = NULL;
	uint8_t *pbuf = NULL;
	size_t nFileSize;
	int ret;
	uint32_t i;
	uint64_t ullImageBase, ullImageSize;
	bool bSuccess = false;

	if (is_storage_firmware(szFirmware))
	{
		nFileSize = get_file_size(szFirmware);
		file = fopen(szFirmware, "rb");
		if (!file)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to open %s,err=%d!", __func__, szFirmware, errno);
			}
			printf("Failed to Open %s\r\n", szFirmware);
			goto exit_show_fw;
		}

		fread(&hdr, 1, sizeof(hdr), file);

		for (i = 0; i < hdr.entry_count; i++)
		{
			fread(&entry, 1, hdr.entry_size, file);
			vecEntrys.push_back(entry);
		}
		fclose(file);
		file = NULL;

		if (nEntry >= 0)
		{
			if (nEntry >= hdr.entry_count)
			{
				printf("EntryNo %d is out of range,total=%d\r\n", nEntry, hdr.entry_count);
				goto exit_show_fw;
			}
			if (vecEntrys[nEntry].offset == 0xFFFFFFFF)
			{
				printf("EntryNo %d is div entry\r\n", nEntry);
				goto exit_show_fw;
			}
			printf("File:%s\r\n", szFirmware);
			printf("Entry No:%d\r\n", nEntry);
			printf("Type:Update Firmware\r\n");
			ullImageBase = vecEntrys[nEntry].offset;
			ullImageSize = vecEntrys[nEntry].size;
			pImage = new CRKImage();
			ret = pImage->ObjectInit(szFirmware, false, ullImageBase, ullImageSize);
			if (ret != 0)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to create image object,ret=%d!", __func__, ret);
				}
				printf("Failed to Create update object\r\n");
				goto exit_show_fw;
			}
			printf("Storage:%s\r\n", get_storage_string(vecEntrys[i].storage).c_str());
			print_update_fw_meta(pImage, ullImageBase);
		}
		else
		{
			printf("File:%s\r\n", szFirmware);
			printf("Type:Multi-Storage Firmware\r\n");
			printf("Entry Count:%d\r\n", hdr.entry_count);
			for (i = 0; i < hdr.entry_count; i++)
			{
				if (vecEntrys[i].offset == 0xFFFFFFFF)
				{
					/* div entry */
					printf("    EntryNo=%d;    Storage=%8s;    name=%s;    offset=0;    size=0\r\n", i, "", "div");
				}
				else
				{
					printf("    EntryNo=%d;    Storage=%8s;    name=%s;    offset=0x%llx;    size=0x%llx\r\n",
						   i,
						   get_storage_string(vecEntrys[i].storage).c_str(),
						   vecEntrys[i].file,
						   vecEntrys[i].offset,
						   vecEntrys[i].size);
				}
			}
		}
		bSuccess = true;
	}
	else if (is_update_firmware(szFirmware))
	{
		printf("File:%s\r\n", szFirmware);
		printf("Type:Update Firmware\r\n");
		ullImageBase = 0;
		ullImageSize = get_file_size(szFirmware);
		pImage = new CRKImage();
		ret = pImage->ObjectInit(szFirmware, false);
		if (ret != 0)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to create image object,ret=%d!", __func__, ret);
			}
			printf("Failed to Create update object\r\n");
			goto exit_show_fw;
		}
		if (pImage->Storage > 0)
			printf("Storage:%s\r\n", get_composite_storage_string(pImage->Storage).c_str());
		print_update_fw_meta(pImage, ullImageBase);
		bSuccess = true;
	}
	else if (is_update_loader(szFirmware))
	{
		printf("File:%s\r\n", szFirmware);
		printf("Type:Update Loader\r\n");
		file = fopen(szFirmware, "rb");
		if (!file)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to open %s,err=%d!", __func__, szFirmware, errno);
			}
			printf("Failed to Open %s\r\n", szFirmware);
			goto exit_show_fw;
		}
		fseek(file, 0, SEEK_END);
		nFileSize = ftell(file);
		fseek(file, 0, SEEK_SET);
		pbuf = new uint8_t[nFileSize];
		fread(pbuf, 1, nFileSize, file);
		fclose(file);
		file = NULL;
		pBoot = new CRKBoot(pbuf, nFileSize);
		if (!pBoot)
		{
			delete[] pbuf;
			printf("Failed to Create Boot Object\r\n");
			goto exit_show_fw;
		}
		ret = pBoot->ObjectInit();
		if (ret != 0)
		{
			printf("Failed to init Boot Object,ret=%d\r\n", ret);
			goto exit_show_fw;
		}

		print_update_loader_meta(pBoot);
		bSuccess = true;
	}
	else
		printf("File is not valid format!\r\n");

exit_show_fw:
	if (file)
		fclose(file);
	if (pImage)
		delete pImage;
	if (pBoot)
		delete pBoot;

	return bSuccess;
}

bool extract_firmware(char *szFirmware, char *szOutDir)
{
	STORAGE_FW_HDR hdr;
	STORAGE_ENTRY entry;
	vector<STORAGE_ENTRY> vecEntrys;
	FILE *file = NULL, *fout = NULL;
	CRKImage *pImage = NULL;
	CRKBoot *pBoot = NULL;
	uint8_t *pbuf = NULL, *pEntryData = NULL;
	size_t nFileSize, nBufferSize = 256 * 1024;
	uint64_t nTotalWrite;
	char szOutFile[256], szEntryName[20];
	int ret;
	uint32_t i, j, nPrevPercent, nPercent, nWrite, nRead;
	uint32_t uiEntrySize, uiEntryDelay;
	bool bSuccess = false;

	if (is_storage_firmware(szFirmware))
	{
		nFileSize = get_file_size(szFirmware);
		file = fopen(szFirmware, "rb");
		if (!file)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to open %s,err=%d!", __func__, szFirmware, errno);
			}
			printf("Failed to Open %s\r\n", szFirmware);
			goto exit_extract_fw;
		}

		fread(&hdr, 1, sizeof(hdr), file);

		for (i = 0; i < hdr.entry_count; i++)
		{
			fread(&entry, 1, hdr.entry_size, file);
			vecEntrys.push_back(entry);
		}

		pbuf = new uint8_t[nBufferSize];
		if (!pbuf)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to new memory,size=0x%x,err=%d!", __func__, nBufferSize, errno);
			}
			printf("Failed to new memory,err=%d\r\n", errno);
			goto exit_extract_fw;
		}

		printf("File:%s\r\n", szFirmware);
		printf("Type:Multi-Storage Firmware\r\n");
		printf("Extract Dir:%s\r\n", szOutDir);
		for (i = 0; i < hdr.entry_count; i++)
		{
			if (vecEntrys[i].offset == 0xFFFFFFFF)
			{
				/* div entry */
				continue;
			}
			if (stricmp(vecEntrys[i].file, "link") == 0)
			{
				/* link entry */
				continue;
			}
			sprintf(szOutFile, "%s%c%s", szOutDir, PATH_DELIMITER, vecEntrys[i].file);
			printf("start to extract %s...\r\n", vecEntrys[i].file);
			fout = fopen(szOutFile, "wb");
			if (!fout)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:%s->Fail to create %s,err=%d!", __func__, szOutFile, errno);
				}
				printf("Failed to create %s\r\n", vecEntrys[i].file);
				goto exit_extract_fw;
			}

			nFileSize = vecEntrys[i].size;
			nPercent = nPrevPercent = 0;
			nTotalWrite = 0;
			fseek(file, vecEntrys[i].offset, SEEK_SET);
			while (nTotalWrite < nFileSize)
			{
				memset(pbuf, 0, nBufferSize);
				if ((nFileSize - nTotalWrite) >= nBufferSize)
					nWrite = nBufferSize;
				else
					nWrite = nFileSize - nTotalWrite;
				nRead = fread(pbuf, 1, nWrite, file);

				if (nRead == nWrite)
				{
					fwrite(pbuf, 1, nWrite, fout);
					nTotalWrite += nWrite;
					nPercent = ((double)nTotalWrite / nFileSize) * 100;
					if (nPercent > nPrevPercent)
					{
						CURSOR_MOVEUP_LINE(1);
						CURSOR_DEL_LINE;
						printf("start to extract %s...(%d%%)\r\n", vecEntrys[i].file, nPercent);
						nPrevPercent = nPercent;
					}
				}
				else
				{
					if (g_pLogObject)
						g_pLogObject->Record("Error:%s->Fail to read, has_write=0x%llx err=%d", __func__, nTotalWrite, errno);
					printf("Failed to read,err=%d\r\n", errno);
					goto exit_extract_fw;
				}
			}
			fclose(fout);
			fout = NULL;
		}
		bSuccess = true;
	}
	else if (is_update_firmware(szFirmware))
	{
		printf("File:%s\r\n", szFirmware);
		printf("Type:Update Firmware\r\n");
		pImage = new CRKImage();
		ret = pImage->ObjectInit(szFirmware, false);
		if (ret != 0)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to create image object,ret=%d!", __func__, ret);
			}
			printf("Failed to Create update object\r\n");
			goto exit_extract_fw;
		}
		bSuccess = extract_update_fw(pImage, szOutDir);
	}
	else if (is_update_loader(szFirmware))
	{
		printf("File:%s\r\n", szFirmware);
		printf("Type:Update Loader\r\n");
		file = fopen(szFirmware, "rb");
		if (!file)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to open %s,err=%d!", __func__, szFirmware, errno);
			}
			printf("Failed to Open %s,err=%d\r\n", szFirmware, errno);
			goto exit_extract_fw;
		}
		fseek(file, 0, SEEK_END);
		nFileSize = ftell(file);
		fseek(file, 0, SEEK_SET);
		pbuf = new uint8_t[nFileSize];
		if (!pbuf)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:%s->Fail to new memory,size=0x%x,err=%d!", __func__, nFileSize, errno);
			}
			printf("Failed to new memory,err=%d\r\n", errno);
			goto exit_extract_fw;
		}
		fread(pbuf, 1, nFileSize, file);

		pBoot = new CRKBoot(pbuf, nFileSize);
		if (!pBoot)
		{
			printf("Failed to Create Boot Object\r\n");
			goto exit_extract_fw;
		}
		ret = pBoot->ObjectInit();
		if (ret != 0)
		{
			printf("Failed to init Boot Object,ret=%d\r\n", ret);
			goto exit_extract_fw;
		}
		printf("471 Count:%d\r\n", pBoot->GetEntry471Count());
		for (i = 0; i < pBoot->Entry471Count; i++)
		{
			pBoot->GetEntryName(ENTRY471, i, szEntryName);
			pBoot->GetEntryProperty(ENTRY471, i, uiEntrySize, uiEntryDelay);
			pEntryData = new uint8_t[uiEntrySize];
			pBoot->GetEntryData(ENTRY471, i, pEntryData);
			if (!pBoot->BootRc4Off)
				P_RC4(pEntryData, uiEntrySize);
			sprintf(szOutFile, "%s%c%s.bin", szOutDir, PATH_DELIMITER, szEntryName);
			printf("start to extract %s...", szEntryName);
			fout = fopen(szOutFile, "wb");
			fwrite(pEntryData, 1, uiEntrySize, fout);
			fclose(fout);
			fout = NULL;
			delete[] pEntryData;
			pEntryData = NULL;
			printf("ok\r\n");
		}
		printf("472 Count:%d\r\n", pBoot->GetEntry472Count());
		for (i = 0; i < pBoot->Entry472Count; i++)
		{
			pBoot->GetEntryName(ENTRY472, i, szEntryName);
			pBoot->GetEntryProperty(ENTRY472, i, uiEntrySize, uiEntryDelay);
			pEntryData = new uint8_t[uiEntrySize];
			pBoot->GetEntryData(ENTRY472, i, pEntryData);
			if (!pBoot->BootRc4Off)
				P_RC4(pEntryData, uiEntrySize);
			sprintf(szOutFile, "%s%c%s.bin", szOutDir, PATH_DELIMITER, szEntryName);
			printf("start to extract %s...", szEntryName);
			fout = fopen(szOutFile, "wb");
			fwrite(pEntryData, 1, uiEntrySize, fout);
			fclose(fout);
			fout = NULL;
			delete[] pEntryData;
			pEntryData = NULL;
			printf("ok\r\n");
		}
		printf("loader Count:%d\r\n", pBoot->GetEntryLoaderCount());
		for (i = 0; i < pBoot->EntryLoaderCount; i++)
		{
			pBoot->GetEntryName(ENTRYLOADER, i, szEntryName);
			pBoot->GetEntryProperty(ENTRYLOADER, i, uiEntrySize, uiEntryDelay);
			pEntryData = new uint8_t[uiEntrySize];
			pBoot->GetEntryData(ENTRYLOADER, i, pEntryData);
			for (j = 0; j < uiEntrySize / 512; j++)
				P_RC4(pEntryData + j * 512, 512);
			if ((uiEntrySize % 512) != 0)
			{
				P_RC4(pEntryData + j * 512, uiEntrySize - j * 512);
			}
			sprintf(szOutFile, "%s%c%s.bin", szOutDir, PATH_DELIMITER, szEntryName);
			printf("start to extract %s...", szEntryName);
			fout = fopen(szOutFile, "wb");
			fwrite(pEntryData, 1, uiEntrySize, fout);
			fclose(fout);
			fout = NULL;
			delete[] pEntryData;
			pEntryData = NULL;
			printf("ok\r\n");
		}
		bSuccess = true;
	}
	else
		printf("File is not valid format!\r\n");
exit_extract_fw:
	if (file)
		fclose(file);
	if (fout)
		fclose(fout);
	if (pImage)
		delete pImage;
	if (pBoot)
		delete pBoot;
	else
	{
		if (pbuf)
			delete[] pbuf;
	}
	if (bSuccess)
		printf("Extract ok.\r\n");
	return bSuccess;
}
bool handle_command(int argc, char *argv[], CRKScan *pScan)
{
	string strCmd = "", strSerial = "", strEntry = "";
	int cnt;
	bool bRet, bFound, bSuccess = false;
	int ret, i;
	uint_32 uiLocationID = 0;
	STRUCT_RKDEVICE_DESC dev;
	STRING_VECTOR vecVolumes;
	for (i = 0; i < argc; i++)
		strCmd = strCmd + " " + argv[i];
	if (g_pLogObject)
		g_pLogObject->Record("Command Line:%s", strCmd.c_str());
	if (argc > 1)
		strCmd = argv[1];
	else
		strCmd = "";
	if ((strcmp(strCmd.c_str(), "-s") == 0) || (strcmp(strCmd.c_str(), "-S") == 0))
	{
		strSerial = argv[2];
		char *pszEnd;
		uiLocationID = strtoul(argv[2], &pszEnd, 16);
		argv[2] = argv[0];
		argv += 2;
		argc -= 2;
		strCmd = argv[1];
		if (argc == 1)
		{
			ERROR_COLOR_ATTR;
			printf("Command is invalid,please check help!");
			NORMAL_COLOR_ATTR;
			printf("\r\n");
			return bSuccess;
		}
	}
	transform(strCmd.begin(), strCmd.end(), strCmd.begin(), (int (*)(int))toupper);
	if (strcmp(strCmd.c_str(), "-H") == 0)
	{
		usage();
		return true;
	}
	else if (strcmp(strCmd.c_str(), "LG") == 0)
	{
		printf("Program Log will save in the %s\r\n", g_pLogObject->GetLogSavePath().c_str());
		return true;
	}
	else if (strcmp(strCmd.c_str(), "-V") == 0)
	{
		string strVer;
		strVer = "Upgrade Tool ";
		strVer += APP_VERSION;
		strVer += "\r\n";
		printf(strVer.c_str());
		return true;
	}
	else if (strcmp(strCmd.c_str(), "SFI") == 0)
	{
		if (argc == 3)
		{
			bSuccess = show_firmware_info(argv[2]);
			return bSuccess;
		}
		else if (argc == 4)
		{
			strEntry = argv[3];
			char *pszEnd;
			uint32_t nEntry;
			nEntry = strtoul(argv[3], &pszEnd, 0);
			if (*pszEnd)
			{
				printf("EntryNo is invalid,please check!\r\n");
				return false;
			}
			bSuccess = show_firmware_info(argv[2], nEntry);
			return bSuccess;
		}
		else
		{
			printf("SFI command is invalid,please check help!\r\n");
			return false;
		}
	}
	else if (strcmp(strCmd.c_str(), "EXF") == 0)
	{
		if (argc == 4)
		{
			bSuccess = extract_firmware(argv[2], argv[3]);
			return bSuccess;
		}
		else
		{
			printf("EXF command is invalid,please check help!\r\n");
			return false;
		}
	}
	else if (strcmp(strCmd.c_str(), "GPT") == 0)
	{
		if (argc == 4)
		{
			bSuccess = create_gpt_by_parameter(argv[2], argv[3]);
			return bSuccess;
		}
		else
		{
			printf("GPT command is invalid,please check help!\r\n");
			return false;
		}
	}
	else if ((strcmp(strCmd.c_str(), "SLD") == 0) ||
			 (strcmp(strCmd.c_str(), "SLV") == 0) ||
			 (strcmp(strCmd.c_str(), "SLK") == 0) ||
			 (strcmp(strCmd.c_str(), "SCR") == 0) ||
			 (strcmp(strCmd.c_str(), "SVR") == 0) ||
			 (strcmp(strCmd.c_str(), "SSZ") == 0) ||
			 (strcmp(strCmd.c_str(), "SRM") == 0) ||
			 (strcmp(strCmd.c_str(), "SRS") == 0) ||
			 (strcmp(strCmd.c_str(), "SWS") == 0) ||
			 (strcmp(strCmd.c_str(), "SFT") == 0))
	{
		CRKDisk *pDisk = NULL;
		CmDiskBaseVec vecDisk;
		uint8_t current_disk;
		uint8_t *mbr;
		vecDisk.clear();
		if (strcmp(strCmd.c_str(), "SLD") == 0)
		{
			cnt = CRKDisk::GetAllDisk(vecDisk);
			if (cnt == -1)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:GetAllDisk failed,err=%d", CRKDisk::GetLastDiskError());
				}
				return false;
			}
			printf("List of removable disk connected\r\n");
			list_disk(vecDisk);
			return true;
		}

		cnt = CRKDisk::GetAllDisk(vecDisk);
		if (cnt == -1)
		{
			if (g_pLogObject)
			{
				g_pLogObject->Record("Error:GetAllDisk failed,err=%d", CRKDisk::GetLastDiskError());
			}
			return false;
		}
		if (cnt == 0)
		{
			printf("Not found any connected removable disk!\r\n");
			return false;
		}

		if (uiLocationID == 0)
		{
			for (i = 0; i < cnt; i++)
			{
				bRet = CRKDisk::CheckDiskRemovable(vecDisk[i].diskNum, bFound);
				if (!bRet)
				{
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:Disk %d failed to check removable.", vecDisk[i].diskNum);
					}
					printf("Disk %d failed to check removable.\r\n", vecDisk[i].diskNum);
					return false;
				}
				if (bFound)
				{
					current_disk = i;
					break;
				}
			}
			if (!bFound)
			{
				printf("Not found any connected removable disk!\r\n");
				return false;
			}
		}
		else
		{
			bFound = false;
			for (i = 0; i < cnt; i++)
			{
				if (uiLocationID == vecDisk[i].diskNum)
				{
					current_disk = i;
					bFound = true;
					break;
				}
			}
			if (!bFound)
			{
				printf("Disk %d is not connected!\r\n", uiLocationID);
				return false;
			}
		}
		printf("Select disk is %s\r\n", vecDisk[current_disk].diskDesc.c_str());

		if (strcmp(strCmd.c_str(), "SLV") == 0)
		{
			ret = CRKDisk::GetAllVolumeFromDisk(vecDisk[current_disk].diskNum, vecVolumes);
			if (ret <= 0)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Disk did not have any mounted volume.");
				}
				printf("Disk did not have any mounted volume.\r\n");
				return false;
			}
			printf("List of mounted volume on disk\r\n");
			for (i = 0; i < ret; i++)
			{
				double drive_size;
				drive_size = (double)CRKDisk::GetDriveSize(vecVolumes[i]) / 1024 / 1024 / 1024;
				printf("Volume %d \t %s \t %1.fG\r\n", i + 1, vecVolumes[i].c_str(), drive_size);
			}
			return true;
		}

		if (strcmp(strCmd.c_str(), "SLK") == 0)
		{
			pDisk = new CRKDisk(vecDisk[current_disk]);
			if (!pDisk)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to new disk object.");
				}
				printf("Disk failed to create\r\n");
				return false;
			}
			bRet = pDisk->OpenDisk();
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to open disk,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("Disk failed to open\r\n");
				return false;
			}
			bRet = pDisk->LockAndDismount();
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to loc disk,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("Disk failed to lock\r\n");
				delete pDisk;
				return false;
			}
			delete pDisk;
			printf("Disk lock ok\r\n");
			return true;
		}

		if (strcmp(strCmd.c_str(), "SCR") == 0)
		{
			pDisk = new CRKDisk(vecDisk[current_disk]);
			if (!pDisk)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to new disk object.");
				}
				printf("Disk failed to create\r\n");
				return false;
			}
			bRet = pDisk->OpenDisk();
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to open disk,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("Disk failed to open\r\n");
				return false;
			}
			bRet = pDisk->LockAndDismount();
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to lock disk,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("Disk failed to lock\r\n");
				delete pDisk;
				return false;
			}
			mbr = new uint8_t[1024];
			memset(mbr, 0, 1024);
			bRet = pDisk->WriteDisk(0, 1024, mbr);
			delete[] mbr;
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to clear mbr,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("Disk failed to clear mbr\r\n");
				delete pDisk;
				return false;
			}
			bRet = pDisk->UpdateDiskLayout();
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to remount disk,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("Disk failed to remount\r\n");
				delete pDisk;
				return false;
			}
			delete pDisk;
			printf("Disk clear mbr ok\r\n");
			return true;
		}

		if (strcmp(strCmd.c_str(), "SVR") == 0)
		{
			string strRoot;
			strRoot = CRKDisk::VolumeRoot(argv[2]);

			if (strRoot.size() == 0)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to get volume from disk,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("failed to get volume root\r\n");
				return false;
			}
			else
			{
				printf("%s\r\n", strRoot.c_str());
				return true;
			}
		}

		if (strcmp(strCmd.c_str(), "SSZ") == 0)
		{
			pDisk = new CRKDisk(vecDisk[current_disk]);
			if (!pDisk)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to new disk object.");
				}
				printf("Disk failed to create\r\n");
				return false;
			}
			bRet = pDisk->OpenDisk();
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to open disk,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("Disk failed to open\r\n");
				return false;
			}

			uint64_t disk_size;
			disk_size = pDisk->GetDiskSize();
			if (disk_size == 0)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to get disk size,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("Disk failed to get size\r\n");
				delete pDisk;
				return false;
			}
			delete pDisk;
			printf("Disk Size %llu\r\n", disk_size);
			return true;
		}

		if (strcmp(strCmd.c_str(), "SRM") == 0)
		{
			pDisk = new CRKDisk(vecDisk[current_disk]);
			if (!pDisk)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to new disk object.");
				}
				printf("Disk failed to create\r\n");
				return false;
			}
			bRet = pDisk->OpenDisk();
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to open disk,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("Disk failed to open\r\n");
				return false;
			}
			bRet = pDisk->UpdateDiskLayout();
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to remount disk,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("Disk failed to remount\r\n");
				delete pDisk;
				return false;
			}
			delete pDisk;
			printf("Disk remount ok\r\n");
			return true;
		}

		if (strcmp(strCmd.c_str(), "SRS") == 0)
		{
			bSuccess = false;
			pDisk = new CRKDisk(vecDisk[current_disk]);
			if (!pDisk)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to new disk object.");
				}
				printf("Disk failed to create\r\n");
				return false;
			}
			bRet = pDisk->OpenDisk();
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to open disk,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("Disk failed to open\r\n");
				return false;
			}

			uint32_t uiBegin, uiLen;
			char *pszEnd;
			uiBegin = strtoul(argv[2], &pszEnd, 0);
			if (*pszEnd)
				printf("Begin is invalid,please check!\r\n");
			else
			{
				uiLen = strtoul(argv[3], &pszEnd, 0);
				if (*pszEnd)
					printf("Len is invalid,please check!\r\n");
				else
				{
					bSuccess = read_disk(pDisk, uiBegin, uiLen, argv[4]);
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:Fail to read disk,err=%d.", CRKDisk::GetLastDiskError());
					}
				}
			}
			pDisk->CloseDisk();
			delete pDisk;
			if (bSuccess)
				printf("Disk read ok,file at %s.\r\n", argv[4]);
			return bSuccess;
		}

		if (strcmp(strCmd.c_str(), "SWS") == 0)
		{
			bSuccess = false;
			pDisk = new CRKDisk(vecDisk[current_disk]);
			if (!pDisk)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to new disk object.");
				}
				printf("Disk failed to create\r\n");
				return false;
			}
			bRet = pDisk->OpenDisk();
			if (!bRet)
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to open disk,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("Disk failed to open\r\n");
				return false;
			}

			uint32_t uiBegin;
			char *pszEnd;
			uiBegin = strtoul(argv[2], &pszEnd, 0);
			if (*pszEnd)
				printf("Begin is invalid,please check!\r\n");
			else
			{
				bSuccess = write_disk(pDisk, uiBegin, argv[3]);
				if (!bSuccess) {
					if (g_pLogObject)
					{
						g_pLogObject->Record("Error:Fail to write disk,err=%d.", CRKDisk::GetLastDiskError());
					}
				}
			}
			pDisk->CloseDisk();
			delete pDisk;
			if (bSuccess)
				printf("Disk write ok.\r\n");
			return bSuccess;
		}

		if (strcmp(strCmd.c_str(), "SFT") == 0)
		{
			printf("format volume %s...\r\n", argv[2]);
			bSuccess = CRKDisk::FormatVolume(argv[2]);

			if (bSuccess)
				printf("format volume ok.\r\n");
			else
			{
				if (g_pLogObject)
				{
					g_pLogObject->Record("Error:Fail to format disk,err=%d.", CRKDisk::GetLastDiskError());
				}
				printf("format failed,err=%d\r\n", CRKDisk::GetLastDiskError());
			}

			return bSuccess;
		}
	}
	ret = context_enter();
	if (ret < 0)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:context_enter failed,err=%d", ret);
		}
		printf("failed to call context_enter!\r\n");
		return false;
	}

	if (strcmp(strCmd.c_str(), "LD") == 0)
	{
		cnt = pScan->Search(RKUSB_MASKROM | RKUSB_LOADER | RKUSB_MSC);
		printf("List of rockusb connected(%d)\r\n", cnt);
		list_device(pScan);
		return true;
	}

	cnt = pScan->Search(RKUSB_MASKROM | RKUSB_LOADER | RKUSB_MSC);
	if (cnt < 1)
	{
		ERROR_COLOR_ATTR;
		printf("No found any rockusb device,please plug device in!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		return bSuccess;
	}

	bFound = false;
	if (strSerial.size() == 0)
	{
		bRet = pScan->GetDevice(dev, 0);
		bFound = true;
	}
	else
	{
		for (i = 0; i < cnt; i++)
		{
			bRet = pScan->GetDevice(dev, i);
			if (!bRet)
				break;
			if ((dev.uiLocationID == uiLocationID) || (stricmp(dev.strSerial.c_str(), strSerial.c_str()) == 0))
			{
				bFound = true;
				break;
			}
		}
	}
	if (!bRet)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:Fail to get info of rockusb!");
		}
		ERROR_COLOR_ATTR;
		printf("Getting information of rockusb device failed!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		return bSuccess;
	}
	if (!bFound)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:Fail to find specific rockusb!");
		}
		ERROR_COLOR_ATTR;
		printf("No found specific rockusb device!");
		NORMAL_COLOR_ATTR;
		printf("\r\n");
		return bSuccess;
	}
	if (g_pLogObject)
		g_pLogObject->Record("Current Device Location ID:%x", dev.uiLocationID);
	if (strcmp(strCmd.c_str(), "RD") == 0)
	{
		if ((argc != 2) && (argc != 3))
			printf("Parameter of [RD] command is invalid,please check help!\r\n");
		else
		{
			if (argc == 2)
				bSuccess = reset_device(dev);
			else
			{
				uint_32 uiSubCode;
				char *pszEnd;
				uiSubCode = strtoul(argv[2], &pszEnd, 0);
				if (*pszEnd)
					printf("Subcode is invalid,please check!\r\n");
				else
				{
					if (uiSubCode <= 6)
						bSuccess = reset_device(dev, uiSubCode);
					else
						printf("Subcode is invalid,please check!\r\n");
				}
			}
		}
	}
	else if (strcmp(strCmd.c_str(), "SD") == 0)
	{
		bSuccess = switch_device(dev);
	}
	else if (strcmp(strCmd.c_str(), "TD") == 0)
	{
		bSuccess = test_device(dev);
	}
	else if (strcmp(strCmd.c_str(), "RCB") == 0)
	{
		bSuccess = read_capability(dev, true);
	}
	else if (strcmp(strCmd.c_str(), "RFI") == 0)
	{
		bSuccess = read_flash_info(dev);
	}
	else if (strcmp(strCmd.c_str(), "RCI") == 0)
	{
		bSuccess = read_chip_info(dev);
	}
	else if (strcmp(strCmd.c_str(), "CPU") == 0)
	{
		string strOutFlag = "";
		string strOutFile = "";
		if (argc > 2) {
			strOutFlag = argv[2];
			transform(strOutFlag.begin(), strOutFlag.end(), strOutFlag.begin(), (int (*)(int))tolower);
			if (strOutFlag.compare("-out") == 0)
				strOutFile = get_program_dir() + PATH_DELIMITER + "cpuid.bin";
			else if (strOutFlag.compare("-textout") == 0)
				strOutFile = get_program_dir() + PATH_DELIMITER + "cpuid.txt";
		}
		bSuccess = read_otp_cpuid(dev, strOutFile);
	}
	else if (strcmp(strCmd.c_str(), "RSM") == 0)
	{
		bSuccess = read_secure_mode(dev);
	}
	else if (strcmp(strCmd.c_str(), "RID") == 0)
	{
		bSuccess = read_flash_id(dev);
	}
	else if (strcmp(strCmd.c_str(), "PL") == 0)
	{
		bSuccess = partition_list(dev);
	}
	else if (strcmp(strCmd.c_str(), "SSD") == 0)
	{
		if (argc < 3)
			bSuccess = switch_storage(dev);
		else
		{
			uint_32 nSelect;
			char *pszEnd;
			nSelect = strtoul(argv[2], &pszEnd, 0);
			if (*pszEnd)
				printf("storage no is invalid,please check!\r\n");
			else
				bSuccess = switch_storage(dev, nSelect);
		}
	}
	else if (strcmp(strCmd.c_str(), "SU3") == 0)
	{
		bSuccess = switch_usb3(dev);
	}
	else if (strcmp(strCmd.c_str(), "SN") == 0)
	{
		if (argc == 3)
		{
			bSuccess = write_vendor_sn(dev, argv[2]);
		}
		else
			printf("Parameter of [SN] command is invalid,please check help!\r\n");
	}
	else if (strcmp(strCmd.c_str(), "RSN") == 0)
	{
		bSuccess = read_vendor_sn(dev);
	}
	else if (strcmp(strCmd.c_str(), "LF") == 0)
	{
		bSuccess = lower_format(dev);
	}
	else if (strcmp(strCmd.c_str(), "DB") == 0)
	{
		if (argc > 2)
		{
			string strLoader;
			strLoader = argv[2];
			bSuccess = download_boot(dev, (char *)strLoader.c_str());
		}
		else if (argc == 2)
		{
			ret = find_config_item("loader");
			if (ret == -1)
				printf("No found loader item from config!\r\n");
			else
				bSuccess = download_boot(dev, g_ConfigItemVec[ret].szItemValue);
		}
		else
			printf("Parameter of [DB] command is invalid,please check help!\r\n");
	}
	else if (strcmp(strCmd.c_str(), "EF") == 0)
	{
		if (argc > 2)
		{
			string strLoader;
			strLoader = argv[2];
			bSuccess = erase_flash(dev, (char *)strLoader.c_str());
		}
		else if (argc == 2)
		{
			ret = find_config_item("loader");
			if (ret == -1)
				printf("No found loader item from config!\r\n");
			else
				bSuccess = erase_flash(dev, g_ConfigItemVec[ret].szItemValue);
		}
		else
			printf("Parameter of [EF] command is invalid,please check help!\r\n");
	}
	else if (strcmp(strCmd.c_str(), "RF") == 0)
	{
		if ((argc == 4) || (argc == 5))
		{
			uint_32 uiBegin, uiLen;
			char *pszEnd;
			uiBegin = strtoul(argv[2], &pszEnd, 0);
			if (*pszEnd)
				printf("Begin is invalid,please check!\r\n");
			else
			{
				uiLen = strtoul(argv[3], &pszEnd, 0);
				if (*pszEnd)
					printf("Length is invalid,please check!\r\n");
				if (argc == 4)
					bSuccess = read_efuse(dev, uiBegin, uiLen, NULL);
				else
					bSuccess = read_efuse(dev, uiBegin, uiLen, argv[4]);
			}
		}
		else
			printf("Parameter of [RF] command is invalid,please check help!\r\n");
	}
	else if (strcmp(strCmd.c_str(), "WL") == 0)
	{
		if (argc >= 4)
		{
			uint_32 uiBegin, uiSize, uiPageSize;
			char *pszEnd, *pszSize, *pszFile;
			bool bContinue = true;
			uiBegin = strtoul(argv[2], &pszEnd, 0);
			if (*pszEnd)
			{
				bContinue = false;
				printf("Begin is invalid,please check!\r\n");
			}

			if (bContinue)
			{
				if (argc > 4)
				{
					pszSize = argv[3];
					pszFile = argv[4];
					if (stricmp(pszSize, "-1") == 0)
					{
						uiSize = -1;
					}
					else
					{
						uiSize = strtoul(pszSize, &pszEnd, 0);
						if (*pszEnd)
						{
							bContinue = false;
							printf("Size is invalid,please check!\r\n");
						}
					}
				}
				else
				{
					pszSize = NULL;
					pszFile = argv[3];
					uiSize = 0;
				}
				if (bContinue)
				{
					if (is_sparse_image(pszFile))
					{
						if (uiSize == 0)
							printf("Image is sparse format,please tell the size!\r\n");
						else
							bSuccess = write_sparse_lba(dev, uiBegin, uiSize, pszFile);
					}
					else if (is_ubi_image(pszFile, &uiPageSize))
					{
						if (uiSize == 0)
							printf("Image is ubi format,please tell the size!\r\n");
						else
							bSuccess = write_ubi_lba(dev, uiBegin, uiSize, uiPageSize, pszFile);
					}
					else if (is_dhara_image(pszFile))
					{
						uiPageSize = -1;
						if (uiSize == 0)
							printf("Image is dhara format,please tell the size!\r\n");
						else
							bSuccess = write_ubi_lba(dev, uiBegin, uiSize, uiPageSize, pszFile);
					}
					else if (is_jffs_image(pszFile))
					{
						if (uiSize == 0)
							printf("Image is jffs format,please tell the size!\r\n");
						else
							bSuccess = write_jffs_lba(dev, uiBegin, uiSize, pszFile);
					}
					else {
						if (uiSize == 0)
							uiSize = -1;
						bSuccess = write_lba(dev, uiBegin, uiSize, pszFile);
					}
						
				}
			}
		}
		else
			printf("Parameter of [WL] command is invalid,please check help!\r\n");
	}
	else if (strcmp(strCmd.c_str(), "RL") == 0)
	{
		if (argc >= 4)
		{
			uint_32 uiBegin, uiLen;
			char *pszEnd;
			uiBegin = strtoul(argv[2], &pszEnd, 0);
			if (*pszEnd)
				printf("Begin is invalid,please check!\r\n");
			else
			{
				uiLen = strtoul(argv[3], &pszEnd, 0);
				if (*pszEnd)
					printf("Len is invalid,please check!\r\n");
				else
				{
					if (argc > 4)
						bSuccess = read_lba(dev, uiBegin, uiLen, argv[4]);
					else
						bSuccess = read_lba(dev, uiBegin, uiLen, NULL);
				}
			}
		}
		else
			printf("Parameter of [RL] command is invalid,please check help!\r\n");
	}
	else if (strcmp(strCmd.c_str(), "EL") == 0)
	{
		if (argc == 4)
		{
			uint_32 uiBegin;
			uint_32 uiLen;
			char *pszEnd;
			uiBegin = strtoul(argv[2], &pszEnd, 0);
			if (*pszEnd)
				printf("BeginSec is invalid,please check!\r\n");
			else
			{
				uiLen = strtoul(argv[3], &pszEnd, 0);
				if (*pszEnd)
					printf("EraseCount is invalid,please check!\r\n");
				else
					bSuccess = erase_lba(dev, uiBegin, uiLen);
			}
		}
		else
			printf("Parameter of [EL] command is invalid,please check help!\r\n");
	}
	// else if (strcmp(strCmd.c_str(), "WS") == 0)
	// {
	// 	if (argc == 6)
	// 	{
	// 		uint_32 uiBegin, uiPageK, uiSpareB;
	// 		char *pszEnd;
	// 		uiBegin = strtoul(argv[2], &pszEnd, 0);
	// 		if (*pszEnd)
	// 			printf("Begin is invalid,please check!\r\n");
	// 		else
	// 		{
	// 			uiPageK = strtoul(argv[3], &pszEnd, 0);
	// 			if (*pszEnd)
	// 				printf("PageSizeK is invalid,please check!\r\n");
	// 			else
	// 			{
	// 				uiSpareB = strtoul(argv[4], &pszEnd, 0);
	// 				if (*pszEnd)
	// 					printf("PageSpareB is invalid,please check!\r\n");
	// 				else
	// 					bSuccess = write_sector(dev, uiBegin, uiPageK, uiSpareB, argv[5]);
	// 			}
	// 		}
	// 	}
	// 	else
	// 		printf("Parameter of [WS] command is invalid,please check help!\r\n");
	// }
	else if (strcmp(strCmd.c_str(), "UL") == 0)
	{
		if (argc > 2)
		{
			string strLoader;
			string strParam;

			strLoader = argv[2];
			if (argc > 3)
			{
				bool bNoReset = false;
				uint_32 storage = 0;
				int i = 3;
				while (i < argc)
				{
					strParam = argv[i];
					transform(strParam.begin(), strParam.end(), strParam.begin(), (int (*)(int))tolower);
					if (strParam.compare("-noreset") == 0)
					{
						bNoReset = true;
					}
					else
					{
						storage = get_storage_value(strParam);
					}
					i++;
				}
				bSuccess = upgrade_loader(dev, (char *)strLoader.c_str(), bNoReset, storage);
			}
			else
				bSuccess = upgrade_loader(dev, (char *)strLoader.c_str());
		}
		else if (argc == 2)
		{
			ret = find_config_item("loader");
			if (ret == -1)
				printf("No found loader item from config!\r\n");
			else
				bSuccess = upgrade_loader(dev, g_ConfigItemVec[ret].szItemValue);
		}
		else
			printf("Parameter of [UL] command is invalid,please check help!\r\n");
	}
	else if (strcmp(strCmd.c_str(), "UF") == 0)
	{
		if (argc > 2)
		{
			string strFirmware;
			string strResetFlag;
			strFirmware = argv[2];

			if (argc > 3)
			{
				strResetFlag = argv[3];
			}
			else
				strResetFlag = "";
			init_process_mutex();
			if (strResetFlag.compare("-noreset") == 0)
				bSuccess = upgrade_firmware(dev, (char *)strFirmware.c_str(), true);
			else
				bSuccess = upgrade_firmware(dev, (char *)strFirmware.c_str());
			destroy_process_mutex();
		}
		else if (argc == 2)
		{
			ret = find_config_item("firmware");
			if (ret == -1)
				printf("No found firmware item from config!\r\n");
			else
			{
				init_process_mutex();
				bSuccess = upgrade_firmware(dev, g_ConfigItemVec[ret].szItemValue);
				destroy_process_mutex();
			}
		}
		else
			printf("Parameter of [UF] command is invalid,please check help!\r\n");
	}
	else if (strcmp(strCmd.c_str(), "RCL") == 0)
	{
		if (argc == 3)
		{
			bSuccess = read_com_log(dev, argv[2]);
		}
		else
			printf("Parameter of [RCL] command is invalid,please check help!\r\n");
	}
	else if (strcmp(strCmd.c_str(), "RUN") == 0)
	{
		if (argc == 8)
		{
			u32 ubootAddr, trustAddr, bootAddr;
			char *pszEnd;
			ubootAddr = strtoul(argv[2], &pszEnd, 0);
			if (*pszEnd)
				printf("Address of uboot is invalid,please check!\r\n");
			else
			{
				trustAddr = strtoul(argv[3], &pszEnd, 0);
				if (*pszEnd)
					printf("Address of trust is invalid,please check!\r\n");
				else
				{
					bootAddr = strtoul(argv[4], &pszEnd, 0);
					if (*pszEnd)
						printf("Address of boot is invalid,please check!\r\n");
					else
						bSuccess = run_system(dev, ubootAddr, trustAddr, bootAddr, argv[5], argv[6], argv[7]);
				}
			}
		}
		else
			printf("Parameter of [RUN] command is invalid,please check help!\r\n");
	}
	else if (strcmp(strCmd.c_str(), "DI") == 0)
	{
		if (argc > 2)
		{
			int i, iParamPos;
			STRING_VECTOR vecItemName, vecItemPath;
			string strItem;
			string::size_type pos_colon;
			uint_32 uiOffset = 0, uiPartSize = 0xFFFFFFFF;
			bool bDownload;
			PARAM_ITEM_VECTOR vecParam;
			STRUCT_PARAM_ITEM paramItem;
			vecParam.clear();
			vecItemName.clear();
			vecItemPath.clear();

			bDownload = false;
			iParamPos = -1;

			if (!get_download_item(&argv[2], argc - 2, vecItemName, vecItemPath, iParamPos))
			{
				printf("check download item failed!\r\n");
			}
			else
			{
				/*
				printf("paramPos=%d,name_size=%d,path_size=%d\r\n",iParamPos,vecItemName.size(),vecItemPath.size());
				for(i=0;i<vecItemName.size();i++)
					printf("name=%s,path=%s\r\n",vecItemName[i].c_str(),vecItemPath[i].c_str());
				*/
				if (iParamPos != -1)
				{
					bDownload = parse_parameter_file((char *)vecItemPath[iParamPos].c_str(), vecParam);
				}
				else
				{
					bDownload = get_parameter_device(dev, vecParam, g_bGpt, g_bEnv, g_bMcu);
				}
				if (bDownload)
				{
					read_capability(dev);
					if (g_pLogObject)
						g_pLogObject->Record("Info:start download,directlba=%d,first4access=%d,gpt=%d,env=%d", g_bDirectLBA, g_bFirst4Access, g_bGpt,g_bEnv);
					printf("directlba=%d,first4access=%d,gpt=%d,env=%d\r\n", g_bDirectLBA, g_bFirst4Access, g_bGpt,g_bEnv);
					int j;
					if (iParamPos != -1)
					{
						strcpy(paramItem.szItemName, "parameter");
						paramItem.uiItemOffset = 0;
						paramItem.uiItemSize = vecParam[0].uiItemOffset;
					}

					for (i = 0; i < vecItemName.size(); i++)
					{
						bDownload = false;
						bRet = true;
						if (iParamPos == i)
						{
							if (!g_bGpt)
								bRet = download_parameter(dev, (char *)vecItemPath[i].c_str(), paramItem);
							else
								bRet = download_gpt_parameter(dev, (char *)vecItemPath[i].c_str());
						}
						else
						{
							for (j = 0; j < vecParam.size(); j++)
							{
								strItem = vecParam[j].szItemName;
								pos_colon = strItem.find(':');
								if (pos_colon != string::npos)
									strItem = strItem.substr(0, pos_colon);
								if (vecItemName[i].compare(strItem) == 0)
								{
									uiOffset = vecParam[j].uiItemOffset;
									uiPartSize = vecParam[j].uiItemSize;
									bDownload = true;
									break;
								}
							}
							if (bDownload)
							{
								if (g_pLogObject)
									g_pLogObject->Record("Info:start download %s,offset=0x%08x,partsize=0x%08x", (char *)vecItemName[i].c_str(), uiOffset, uiPartSize);
								printf("Download %s start...(0x%08x)\r\n", (char *)vecItemName[i].c_str(), uiOffset);
								if (is_sparse_image((char *)vecItemPath[i].c_str()))
									bRet = download_sparse_image(dev, (char *)vecItemPath[i].c_str(), uiOffset, uiPartSize);
								else
									bRet = download_image(dev, (char *)vecItemPath[i].c_str(), uiOffset, uiPartSize);
							}
							else
								printf("No found %s in the parameter,please check!\r\n", vecItemName[i].c_str());
						}
						if (!bRet)
							break;
					}
					bSuccess = bRet;
				}
				else
					printf("parameter is invalid,please check!\r\n");
			}
		}
		else
			printf("Parameter of [DI] command is invalid,please check help!\r\n");
	}
	else
	{
		printf("command is invalid,please press upgrade_tool -h to check usage!\r\n");
	}
	if (g_pLogObject)
		g_pLogObject->Record("Command mode out");
	return bSuccess;
}

int main(int argc, char *argv[])
{
	STRUCT_RKDEVICE_DESC dev;
	CRKScan *pScan = NULL;
	bool bFoundConfig = false;
	int ret;
	char szBuf[256];

	string strLogDir, strConfigFile, strProgramDir, strLocaitonID, strUserDir;

	if (argc == 1)
	{
		usage();
		return 0;
	}

	g_ParamItemVec.clear();
	g_ConfigItemVec.clear();
	strProgramDir = get_program_dir();
	strUserDir = get_user_dir();

	if (strUserDir.size() == 0)
		strUserDir = strProgramDir;
	else
	{
		strUserDir += PATH_DELIMITER;
		strUserDir += "upgrade_tool";
		if (!exist_dir(strUserDir))
			create_dir(strUserDir);
	}

	sprintf(szBuf, "%s%c%s", strProgramDir.c_str(), PATH_DELIMITER, "config.ini");
	strConfigFile = szBuf;
	if (exist_file(strConfigFile))
	{
		printf("Using %s\r\n", strConfigFile.c_str());
		bFoundConfig = true;
		parse_config_file(strConfigFile.c_str(), g_ConfigItemVec);
		if (GetLogDir(strLogDir))
		{
			if (strLogDir.size() > 0)
			{
				if (strLogDir == ".")
				{
					strUserDir = strProgramDir;
				}
				else
					strUserDir = strLogDir;
			}
		}
	}

	sprintf(szBuf, "%s%c%s%c", strUserDir.c_str(), PATH_DELIMITER, "log", PATH_DELIMITER);
	strLogDir = szBuf;
	if (!exist_dir(strLogDir))
		create_dir(strLogDir);

	// printf("Program Log will save in the %s\r\n", strLogDir.c_str());

	strLocaitonID = "";
	if (argc > 3)
	{
		if ((strcmp(argv[1], "-s") == 0) || (strcmp(argv[1], "-S") == 0))
		{
			strLocaitonID = argv[2];
		}
	}
	if (strLocaitonID.size() > 0)
		sprintf(szBuf, "log_%s_", strLocaitonID.c_str());
	else
		memset(szBuf, 0, sizeof(szBuf));
	g_pLogObject = new CRKLog(strLogDir.c_str(), szBuf);
	if (bFoundConfig)
	{
		GetDeviceIDFromConfig();
		GetReadCheckFlag(g_bReadCheckOff);
		GetLogFlag(g_bLogOff);
		GetParityFlag(g_bCBW_parity);
		GetNorSingleIDBFlag(g_bNorSingleIDB);
		GetForceDataBand(g_uiForceBand);
		GetConsoleBufferFlag();
		GetUsb3TransferFlag(g_bUsb3Transfer);
		GetIdbCheckFlag(g_bCloseCheckIDB);
	}
	else
	{
		if (stricmp(argv[argc - 1], "ld") == 0)
		{ // to stop log
			g_bLogOff = true;
		}
	}
	if (g_pLogObject)
	{
		if (g_bLogOff)
			g_pLogObject->EnableLog = false;
		g_pLogObject->Record("Upgrade Tool %s", APP_VERSION);
		g_pLogObject->Record("Current process in %s", strProgramDir.c_str());
		if (bFoundConfig)
			g_pLogObject->Record("Using %s", strConfigFile.c_str());
		else
			g_pLogObject->Record("No found config.ini,in the %s", strConfigFile.c_str());
	}

	pScan = new CRKScan();
	if (!pScan)
	{
		if (g_pLogObject)
		{
			g_pLogObject->Record("Error:Create scan object failed");
			delete g_pLogObject;
		}
		return -1;
	}
	pScan->SetVidPid(g_usMscVid, g_usMscPid);
	pScan->AddRockusbVidPid(g_usDevNewVid, g_usDevNewPid, g_usDevOldVid, g_usDevOldPid);
	#ifdef _WIN32
		uint_32 major,minor;
		if (CRKScan::GetRockusbDriverVer(major,minor)) {
			if ((major > 5) || ((major == 5) && (minor >= 13)))
				g_bUsbDriver4m = true;
			else {
				printf("rockusb driver = %d.%02d,please upgrade it to enjoy fast download\r\n", major,minor);
			}
		}
	#else
		g_bUsbDriver4m = true;
	#endif

	if (handle_command(argc, argv, pScan))
		ret = 0;
	else
		ret = 0xFF;

	if (pScan)
		delete pScan;
	if (g_pLogObject)
		delete g_pLogObject;
	context_exit();
	return ret;
}
