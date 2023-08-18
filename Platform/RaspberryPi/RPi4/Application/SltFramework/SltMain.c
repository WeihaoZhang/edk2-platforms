/** @file
 *
 *  Copyright 2023 Cix Technology (Shanghai) Co., Ltd. All Rights Reserved. *
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <SltCommon.h>
#include "Mcf/mcf.h"
#include "SPECLib/Common.h"

VOID               *mSltCurRxCmdPtr     = NULL;
VOID               *mSltResultPtr       = NULL;
SLT_TESTCASE_INFO  *mSltIPTestCaseInfo  = NULL;

STATIC
VOID
SLTRegisterOneIP (
  UINT32    IPIndex,
  CHAR16    *IPName,
  CHAR16    *IPEfiFileName
  )
{
  SLT_TESTCASE_INFO  *TempIpInfo        = mSltIPTestCaseInfo;
  UINT32             IpCounts           = 0;

  while (TempIpInfo->IPName != NULL) {
    // Reject the same IPName
    if (StrCmp (TempIpInfo->IPName, IPName)) {
      DEBUG ((DEBUG_ERROR, "[SLT] You can't register with the same name [%s], Please change it!\n", IPName));
      return;
    }
    // Keep the last one is NULL
    IpCounts++;
    if (IpCounts >= (SLT_IP_INDEX_MAX - 1)) {
      DEBUG ((DEBUG_ERROR, "[SLT] IPs more then SLT_IP_INDEX_MAX(%d) Break!!!\n", SLT_IP_INDEX_MAX));
      return;
    }
    TempIpInfo++;
  }

  // Fill the Fix Information
  TempIpInfo->IPIndex       = IPIndex;
  TempIpInfo->IPName        = IPName;
  TempIpInfo->IPEfiFileName = IPEfiFileName;

  DEBUG ((DEBUG_INFO, "[SLT] You register one IP[%s] to the gloable buffer\n", TempIpInfo->IPName));

  return;
}

STATIC
VOID
SLTInitIP (
  VOID
  )
{
  // Allocate the gloable buffer
  mSltIPTestCaseInfo = AllocateRuntimeZeroPool (sizeof(SLT_TESTCASE_INFO) * SLT_IP_INDEX_MAX);
  if (mSltIPTestCaseInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "[SLT] %a:Allocate Buffer Fail !\n", __FUNCTION__));
    return;
  }

  // add this function when add one IP eg:
  SLTRegisterOneIP(SLT_IP_INDEX_DEMO,SLT_DEMOIP_NAME,SLT_DEMOIP_EFI_FILE_NAME);
  //  SLTRegisterOneIP(SLT_IP_INDEX_DEMO,SLT_DEMOIP_NAME,SLT_DEMOIP_EFI_FILE_NAME);

  return;
}


#if (SLT_DEBUG)
STATIC
VOID
SltForTestWithOutReceiveCmd (
  VOID
  )
{
  VOID               *SltCmdPkg         = NULL;
  SLT_CMD_HEADER     *SltCmdHeader      = NULL;
  VOID               *SltResultPkg      = NULL;

  SltCmdPkg    = AllocateRuntimeZeroPool (CMD_PKG_MAX * 4);
  SltResultPkg = AllocateRuntimeZeroPool (RESULT_PKG_MAX * SLT_CORE_ID_MAX);
  if (SltCmdPkg == NULL || SltResultPkg == NULL) {
    DEBUG ((DEBUG_ERROR, "[SLT] %a:Allocate Buffer Fail !\n", __FUNCTION__));
    return;
  }

  // 先手动填充一个命令形式 该命令需要在SLT PC Tools中组合
  SltCmdHeader = (SLT_CMD_HEADER *)SltCmdPkg;
  SltCmdHeader->SiteID = 0x6;
  SltCmdHeader->IPNameOffset = sizeof(SLT_CMD_HEADER);
  SltCmdHeader->IPNameLen = sizeof(SLT_DEMOIP_NAME);
  SltCmdHeader->CaseNameOffset = SltCmdHeader->IPNameOffset + ALIGN_VALUE(SltCmdHeader->IPNameLen, SLT_ALIGN_VALUE);
  SltCmdHeader->CaseNameLen = sizeof(SLT_DEMOCASE_NAME);
  SltCmdHeader->ParameterOffset = SltCmdHeader->CaseNameOffset + ALIGN_VALUE(SltCmdHeader->CaseNameLen, SLT_ALIGN_VALUE);

  CopyMem (((VOID *)(UINT64)SltCmdPkg + SltCmdHeader->IPNameOffset), SLT_DEMOIP_NAME, SltCmdHeader->IPNameLen);
  CopyMem (((VOID *)(UINT64)SltCmdPkg + SltCmdHeader->CaseNameOffset), SLT_DEMOCASE_NAME, SltCmdHeader->CaseNameLen);

  WriteBackDataCacheRange (SltCmdPkg, CMD_PKG_MAX * 4);
  InvalidateInstructionCacheRange (SltCmdPkg, CMD_PKG_MAX * 4);

  WriteBackDataCacheRange (SltResultPkg, RESULT_PKG_MAX * SLT_CORE_ID_MAX);
  InvalidateInstructionCacheRange (SltResultPkg, RESULT_PKG_MAX * SLT_CORE_ID_MAX);

  mSltCurRxCmdPtr  = SltCmdPkg;
  mSltResultPtr    = SltResultPkg;

  return;
}
#endif

// dispatch by Mpservices
STATIC
EFI_STATUS
SltProcseeIPWorker (
  OUT SLT_TESTCASE_INFO *SltIPTestCaseInfo
  )
{
  EFI_STATUS   Status                   = SLT_SUCCESS;

  // 1. initial the load option
  Status = SltInitLoadOptionInfo(SltIPTestCaseInfo);
  if (Status != SLT_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "[SLT] %a: Initial the LoadOption Fail!\n", __FUNCTION__));
    return Status;
  }

  // 2. load and start the efi image
  Status = SltStartEfi(SltIPTestCaseInfo);
  if (Status != SLT_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "[SLT] %a: Start the Efi Fail!\n", __FUNCTION__));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "[SLT] %a: Run the Efi Pass!\n", __FUNCTION__));
  return Status;
}

STATIC
VOID
SltInitAuxCoreInfo (
  IN  UINT32            CoreID,
  OUT SLT_TESTCASE_INFO *SltIPTestCaseInfo
)
{
  SLT_TESTCASE_INFO  *IpInfo            = SltIPTestCaseInfo;
  IpInfo->AuxCoreId = CoreID;
  IpInfo->IPCaseResult = (VOID *)((UINT64)mSltResultPtr + RESULT_PKG_MAX * CoreID);
}

EFI_STATUS
SltRunEfi (
  IN  EFI_SYSTEM_TABLE  *SystemTable,
  OUT SLT_TESTCASE_INFO *IpInfo
  )
{
  EFI_STATUS         Status             = SLT_SUCCESS;
  UINT32             CoreID;

  if (IpInfo->IPName == NULL || IpInfo->IPFoundedFlag != TRUE) {
    return SLT_NOT_FOUND_IP;
  }

  // MPservices -- todo
  CoreID = SLT_CORE_ID_1;
  SltInitAuxCoreInfo(CoreID, IpInfo);

  // SltProcseeIPWorker 是从核执行的函数，参数为当前的 IpInfo
  DEBUG ((DEBUG_INFO, "[SLT] SltRunEfi do SltProcseeIPWorker!\n"));
  Status = SltProcseeIPWorker(IpInfo);
  // wait the CMD result
  // Status = SltWaitResult ();
  // if (Status != SLT_SUCCESS) {
  //   DEBUG ((DEBUG_ERROR, "[SLT] SltWaitResult Fail!\n"));
  // }

  return Status;
}


/**
  Function for SLT Framework Application Entry

  @param[in]  ImageHandle           Handle to the Image.
  @param[in]  SystemTable           Pointer to the System Table.

  @retval  SLT_SUCCESS              Completed successfully.
  @retval  Others                   Something Failed.

**/
EFI_STATUS
SltFrameworkMain (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS         Status             = SLT_SUCCESS;
  //SLT_TESTCASE_INFO  *IpInfo            = NULL;

  DEBUG ((DEBUG_INFO, "[SLT] %a:(%d) Start............\n", __FUNCTION__, __LINE__));

  //
  // 1. SLT will disable the Watchdog
  //
  gBS->SetWatchdogTimer (0, 0, 0, NULL);

  //
  // 2. SLT Register IP one by one
  //
  SLTInitIP ();

  //
  // 2. clean the memory for receive cmd and fill result
  //
  if (SLT_DEBUG) {
    SltForTestWithOutReceiveCmd ();
  } else {
    ZeroMem ((VOID *)SLT_RECEIVED_CMD_BASE_ADDR, SLT_RECEIVED_CMD_ADDR_SIZE);
    ZeroMem ((VOID *)SLT_IP_CASE_RESULT_BASE_ADDR, SLT_IP_CASE_RESULT_ADDR_SIZE);
    mSltCurRxCmdPtr = (VOID *)SLT_RECEIVED_CMD_BASE_ADDR;
  }
  char *argv = "\\EFI\\BOOT\\inp.in";//"inp.in"; 
  do {
    UINT64 flag;
    flag=LocateFileRoot();
    Print(L"ethanzhang flag=%x\n",flag);
    // test_read_file();
     test_main(2, argv);
    // 3. get PC CMD fill the buffer mSltCurRxCmdPtr
    // 如果是并发测试，则一次性传输所有的测试命令下来，在获取时依次填充接收的命令
    // Status = SltRxCmd (
    //             SystemTable,
    //             mSltCurRxCmdPtr
    //           );
    // if (Status != SLT_SUCCESS) {
    //   DEBUG ((DEBUG_ERROR, "[SLT] SltGetCmd Fail Try Again!\n"));
    //   continue;
    // }

    // // 4. find the cmd and init the mSltIPTestCaseInfo[ip]
    // IpInfo = SltFindIpFromCmd (
    //             mSltIPTestCaseInfo,
    //             mSltCurRxCmdPtr
    //           );
    // if (IpInfo == NULL) {
    //   DEBUG ((DEBUG_INFO, "[SLT] FindIpFromCmd IP not found Try Again!\n"));
    //   continue;
    // }

    // // 5. then exe the CMD and wait the CMD result
    // // 这个命令分阻塞和非阻塞，如果是单核就是阻塞执行，并发就是非阻塞，主控core将任务分发下来就退出了
    // // 如果是并发，那么反馈结果，需要给每个并行队列都加上一个结果buffer
    // // 当前还只考虑单核情况
    // Status = SltRunEfi (
    //             SystemTable,
    //             IpInfo
    //           );
    // if (Status != SLT_SUCCESS) {
    //   DEBUG ((DEBUG_ERROR, "[SLT] SltRunEfi Fail Try Again!\n"));
    //   // continue;
    // }

    // // 4 send the CMD result to PC  --- 定义相同的结构和协议
    // Status = SltTxResult (SystemTable, IpInfo);
    // if (Status != SLT_SUCCESS) {
    //   DEBUG ((DEBUG_ERROR, "[SLT] SltSendResult Fail!\n"));
    // }
  } while (0);

  //
  // SLT Framework should never return
  //
  DEBUG ((DEBUG_ERROR, "[SLT] Framework should never return!\n"));
  CpuDeadLoop ();

  return Status;
}
