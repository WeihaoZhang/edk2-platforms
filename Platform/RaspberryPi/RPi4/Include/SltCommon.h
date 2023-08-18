/** @file
 *
 *  Copyright 2023 Cix Technology (Shanghai) Co., Ltd. All Rights Reserved. *
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef _SLT_COMMON_H_
#define _SLT_COMMON_H_

#include <PiDxe.h>
#include <Uefi.h>
#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/LoadFile.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/DxeServicesLib.h>
#include <Library/CacheMaintenanceLib.h>

extern EFI_SYSTEM_TABLE                 *gST;
extern EFI_BOOT_SERVICES                *gBS;
extern EFI_HANDLE                       gImageHandle;


#define SLT_DEBUG                       1

// define efi file path -- demo ip
#define SLT_DEMOIP_NAME                 L"SLTDEMOIP"
#define SLT_DEMOIP_EFI_FILE_NAME        L"\\EFI\\BOOT\\SLTDEMOIP.EFI"

#define SLT_DEMOCASE_NAME               L"SLTDEMOTESTCASE1"


#define SLT_IP_CASE_MAX                 32

// IP index
typedef enum _SLT_IP_INDEX {
  SLT_IP_INDEX_DEMO  = 1,
  SLT_IP_INDEX_AUDIO = 0,
  SLT_IP_INDEX_CPU,
  SLT_IP_INDEX_DDR,
  SLT_IP_INDEX_DISPLAY,
  SLT_IP_INDEX_ISP,
  SLT_IP_INDEX_GPU,
  SLT_IP_INDEX_NPU,
  SLT_IP_INDEX_SLC,
  SLT_IP_INDEX_VPU,
  SLT_IP_INDEX_PCIE,
  SLT_IP_INDEX_USB,
  SLT_IP_INDEX_MIPI,
  SLT_IP_INDEX_DP,
  SLT_IP_INDEX_GMAC,
  SLT_IP_INDEX_MAX = 32,
} SLT_IP_INDEX;

// CPU index
typedef enum _SLT_CORE_ID {
  SLT_CORE_ID_0  = 0,
  SLT_CORE_ID_1,
  SLT_CORE_ID_2,
  SLT_CORE_ID_3,
  SLT_CORE_ID_4,
  SLT_CORE_ID_5,
  SLT_CORE_ID_6,
  SLT_CORE_ID_7,
  SLT_CORE_ID_8,
  SLT_CORE_ID_9,
  SLT_CORE_ID_A,
  SLT_CORE_ID_B,
  SLT_CORE_ID_MAX = 0xC,
} SLT_CORE_ID;


// SLT test case used memory
// 从SLT FW 的固定地址分配 共16M
#define SLT_RECEIVED_CMD_BASE_ADDR      0x90000000   // PCDget
#define SLT_RECEIVED_CMD_ADDR_SIZE      0x00400000   // 4MB

#define SLT_IP_CASE_RESULT_BASE_ADDR    (SLT_RECEIVED_CMD_BASE_ADDR + SLT_RECEIVED_CMD_ADDR_SIZE)
#define SLT_IP_CASE_RESULT_ADDR_SIZE    0x00C00000   // 12MB


// define the slt error num
#define SLT_RESULT_PASS                 0
#define SLT_RESULT_FAIL                 1

#define SLT_SUCCESS                     0
#define SLT_NOT_FOUND_IP                ENCODE_ERROR(101)
#define SLT_NOT_FOUND_CASE              ENCODE_ERROR(201)
#define SLT_INVALID_PARAMETER           ENCODE_ERROR(301)
#define SLT_MALLOC_FAIL                 ENCODE_ERROR(401)
#define SLT_FILEPATH_FAIL               ENCODE_ERROR(501)
#define SLT_NOT_FOUND_CASE_FUNC         ENCODE_ERROR(601)
#define SLT_RUN_CASE_FAIL               ENCODE_ERROR(701)


#define SLT_ALIGN_VALUE                 4
// command limit
#define SLT_CMD_HEADER_MAX              32
#define IP_NAME_LEN_MAX                 32
#define CASE_NAME_LEN_MAX               64
#define PARAMETER_LEN_MAX               128
#define CMD_PKG_MAX                     256

// Result limit
#define SLT_RESULT_HEADER_MAX           32
#define IP_NAME_LEN_MAX                 32
#define CASE_NAME_LEN_MAX               64
#define CASE_RESULT_LEN_MAX             384
#define RESULT_PKG_MAX                  512

// define the cmd pkg formate:
// header + ip name + case name + parameters
// eg: header vpu h265_1080p timeout=500ms,times=3
typedef struct _SLT_CMD_HEADER {
  UINT32    SiteID;
  UINT32    IPNameOffset;
  UINT32    IPNameLen;
  UINT32    CaseNameOffset;
  UINT32    CaseNameLen;
  UINT32    ParameterOffset;
  UINT32    ParameterLen;
  UINT32    CheckSum;
} SLT_CMD_HEADER;

typedef struct _SLT_CMD_LAYOUT {
  SLT_CMD_HEADER    *pSltCmdHeader;
  CHAR16            *pIPName;
  CHAR16            *pCaseName;
  CHAR16            *pCaseParameter;
} SLT_CMD_LAYOUT;

// IP TestCase Result
typedef struct _SLT_RESULT_HEADER {
  UINT32    SiteID;
  UINT32    IPNameOffset;
  UINT32    IPNameLen;
  UINT32    CaseNameOffset;
  UINT32    CaseNameLen;
  UINT32    CaseResultOffset;
  UINT32    CaseResultLen;
  UINT32    CheckSum;
} SLT_RESULT_HEADER;

typedef struct _SLT_CASE_RESULT {
  UINT32    CaseResult; // Pass or Fail
  UINT32    ErrorNum;   // Pass == 0 , Fail == error num
  // CHAR16    *pMsgStr;   // error key message
} SLT_CASE_RESULT;

typedef struct _SLT_RESULT_LAYOUT {
  SLT_RESULT_HEADER    *pSltResultHeader;
  CHAR16               *pIPName;
  CHAR16               *pCaseName;
  SLT_CASE_RESULT      *pResultMsg;
} SLT_RESULT_LAYOUT;



typedef struct _SLT_LOADEFI_OPTION {
  CHAR16                    *Description;     // Load EFI Option Description
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;        // Load EFI Option Device Path
  VOID                      *ParameterData;    // Load EFI Option optional data to pass into image
  UINT32                    ParameterDataSize; // Load EFI Option size of ParameterData

  VOID                      *ExitData;       // Exit data returned from gBS->StartImage ()
  UINT32                    ExitDataSize;    // Size of ExitData
  EFI_STATUS                Status;          // Status returned from boot attempt gBS->StartImage ()
} SLT_LOADEFI_OPTION;




// SLT testcase info struct
// include all the information needed by SLT_FW , SLT_IP and IPTestCase 
typedef struct _SLT_TESTCASE_INFO {
  // Static information at IP register
  UINT32             IPIndex;
  CHAR16             *IPName;
  CHAR16             *IPEfiFileName;
  EFI_GUID           *IPEfiGuid;

  // Dynamic information fill at runtime
  VOID               *RxCmd;
  VOID               *IPCaseResult;
  VOID               *IPTestCase;
  UINT32             AuxCoreId;
  BOOLEAN            IPFoundedFlag;
  BOOLEAN            CaseFoundedFlag;
  SLT_LOADEFI_OPTION AuxCoreLoadEfiOption;
} SLT_TESTCASE_INFO;



EFI_STATUS
EFIAPI
SltRxCmd (
  IN  EFI_SYSTEM_TABLE  *SystemTable,
  OUT VOID              *SltCurRxCmdPtr
  );

EFI_STATUS
EFIAPI
SltTxResult (
  IN  EFI_SYSTEM_TABLE  *SystemTable,
  OUT SLT_TESTCASE_INFO *SltIPTestCaseInfo
  );

SLT_TESTCASE_INFO  *
EFIAPI
SltFindIpFromCmd (
  OUT SLT_TESTCASE_INFO *SltIPTestCaseInfo,
  OUT VOID              *SltCurRxCmdPtr
  );

EFI_STATUS
EFIAPI
SltInitLoadOptionInfo(
  OUT  SLT_TESTCASE_INFO *SltIPTestCaseInfo
  );

EFI_STATUS
EFIAPI
SltStartEfi (
  OUT  SLT_TESTCASE_INFO *SltIPTestCaseInfo
  );




#endif
