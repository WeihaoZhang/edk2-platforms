/** @file
 *
 *  Copyright 2023 Cix Technology (Shanghai) Co., Ltd. All Rights Reserved. *
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <SltCommon.h>

/**
  Function for SLT Framework : Get SLT Cmd From Net connected PC tool

  @param[in]  SystemTable           Pointer to the System Table.
  @param[out] SltCurRxCmdPtr        Pointer to the CMD Buffer.

  @retval  SLT_SUCCESS              Completed successfully.
  @retval  Others                   Failed.

**/
EFI_STATUS
EFIAPI
SltRxCmd (
  IN  EFI_SYSTEM_TABLE  *SystemTable,
  OUT VOID              *SltCurRxCmdPtr
  )
{
  EFI_STATUS  Status = SLT_SUCCESS;

  // 1 从网口中获取到数据包

  // 2 将其放到 SltCurRxCmdPtr 指向的内存地址空间中
  return Status;
}

EFI_STATUS
EFIAPI
SltTxResult (
  IN  EFI_SYSTEM_TABLE  *SystemTable,
  OUT SLT_TESTCASE_INFO *SltIPTestCaseInfo
  )
{
  EFI_STATUS  Status = SLT_SUCCESS;

  VOID               *RxCmd             = SltIPTestCaseInfo->RxCmd;
  SLT_CMD_HEADER     *CmdHeader         = (SLT_CMD_HEADER *)RxCmd;
  CHAR16             *SltRxCaseName     = (CHAR16 *)((UINT64)RxCmd + CmdHeader->CaseNameOffset);
  VOID               *TxResult          = SltIPTestCaseInfo->IPCaseResult;
  SLT_RESULT_HEADER  *ResultHeader      = (SLT_RESULT_HEADER *)TxResult;
  SLT_CASE_RESULT    *CaseResult        = (SLT_CASE_RESULT *)((UINT64)TxResult + ResultHeader->CaseResultOffset);
  CHAR16             *ErrorMsg          = (CHAR16 *)((UINT64)CaseResult + 2 * sizeof(UINT32));

  DEBUG ((DEBUG_ERROR, "[SLT] ResultHeader->SiteID = 0x%x!\n", ResultHeader->SiteID));
  DEBUG ((DEBUG_ERROR, "[SLT] ResultHeader->CaseResultLen = 0x%x!\n", ResultHeader->CaseResultLen));
  DEBUG ((DEBUG_ERROR, "[SLT] ResultHeader->CaseResultOffset = 0x%x!\n", ResultHeader->CaseResultOffset));

  DEBUG ((DEBUG_ERROR, "[SLT] CaseResult->CaseResult = 0x%x!\n", CaseResult->CaseResult));
  DEBUG ((DEBUG_ERROR, "[SLT] CaseResult->ErrorNum = 0x%x!\n", CaseResult->ErrorNum));
  DEBUG ((DEBUG_ERROR, "[SLT] ErrorMsg = %s!\n", ErrorMsg));

  DEBUG ((DEBUG_ERROR, "[SLT] CmdHeader->CaseNameOffset = 0x%x!\n", CmdHeader->CaseNameOffset));
  DEBUG ((DEBUG_ERROR, "[SLT] RxCmd = 0x%x!\n", RxCmd));
  DEBUG ((DEBUG_ERROR, "[SLT] CmdHeader = 0x%x!\n", CmdHeader));
  DEBUG ((DEBUG_ERROR, "[SLT] SltRxCaseName = 0x%x!\n", SltRxCaseName));


  // 1 SltIPTestCaseInfo->IPCaseResult 通过网络发送出去

  return Status;
}


SLT_TESTCASE_INFO  *
EFIAPI
SltFindIpFromCmd (
  OUT SLT_TESTCASE_INFO *SltIPTestCaseInfo,
  OUT VOID              *SltCurRxCmdPtr
  )
{
  SLT_TESTCASE_INFO  *IpInfo            = SltIPTestCaseInfo;
  SLT_CMD_HEADER     *SltCmdHeader      = (SLT_CMD_HEADER *)SltCurRxCmdPtr;
  CHAR16             *SltCurRxIpName    = (CHAR16 *)((UINT64)SltCurRxCmdPtr + SltCmdHeader->IPNameOffset);

  // 1. find the ip by IP name
  while (IpInfo->IPName != NULL) {
    if (StrCmp (IpInfo->IPName, SltCurRxIpName) == 0) {
      DEBUG ((DEBUG_INFO, "[SLT] Firmework Got it [%s]!\n", SltCurRxIpName));
      break;
    } else {
      IpInfo++;
    }
  }

  if ((IpInfo->IPName == NULL)) {
    DEBUG ((DEBUG_ERROR, "[SLT] %a IP[%s] not found!\n", __FUNCTION__, SltCurRxIpName));
    return NULL;
  }

  // 2. init the Rx cmd Info
  IpInfo->RxCmd = (VOID *)SltCurRxCmdPtr;
  IpInfo->IPFoundedFlag = TRUE;

  return IpInfo;
}
