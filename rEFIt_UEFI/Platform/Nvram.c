/**
 *  Module for work with runtime (RT, NVRAM) vars,
 *  determining default boot volume (Startup disk)
 *  and (kid of) persistent RT support with nvram.plist on CloverEFI.
 *  dmazar, 2012
 */

#include "Platform.h"

#ifndef DEBUG_ALL
#define DEBUG_SET 1
#else
#define DEBUG_SET DEBUG_ALL
#endif

#if DEBUG_SET == 0
#define DBG(...)
#else
#define DBG(...) DebugLog (DEBUG_SET, __VA_ARGS__)
#endif


// for saving nvram.plist and it's data
TagPtr                   gNvramDict;

//
// vars filled after call to GetEfiBootDeviceFromNvram ()
//

// always contains original efi-boot-device-data
EFI_DEVICE_PATH_PROTOCOL *gEfiBootDeviceData;

// if gEfiBootDeviceData starts with MemoryMapped node, then gBootCampHD = "BootCampHD" var, otherwise == NULL
EFI_DEVICE_PATH_PROTOCOL *gBootCampHD;

// contains only volume dev path from gEfiBootDeviceData or gBootCampHD
EFI_DEVICE_PATH_PROTOCOL *gEfiBootVolume;

// contains file path from gEfiBootDeviceData or gBootCampHD (if exists)
CHAR16                   *gEfiBootLoaderPath;

// contains GPT GUID from gEfiBootDeviceData or gBootCampHD (if exists)
EFI_GUID                 *gEfiBootDeviceGuid;

// Lilu/Ozmosis GUID
EFI_GUID    mLiluNormalVariableGuid     = { 0x2660DD78, 0x81D2, 0x419D, { 0x81, 0x38, 0x7B, 0x1F, 0x36, 0x3F, 0x79, 0xA6 } };
EFI_GUID    mLiluReadOnlyVariableGuid   = { 0xE09B9297, 0x7928, 0x4440, { 0x9A, 0xAB, 0xD1, 0xF8, 0x53, 0x6F, 0xBF, 0x0A } };
EFI_GUID    mLiluWriteOnlyVariableGuid  = { 0xF0B9AF8F, 0x2222, 0x4840, { 0x8A, 0x37, 0xEC, 0xF7, 0xCC, 0x8C, 0x12, 0xE1 } };
EFI_GUID    mOzmosisProprietary1Guid    = { 0x4D1FDA02, 0x38C7, 0x4A6A, { 0x9C, 0xC6, 0x4B, 0xCC, 0xA8, 0xB3, 0x01, 0x02 } };
EFI_GUID    mOzmosisProprietary2Guid    = { 0x1F8E0C02, 0x58A9, 0x4E34, { 0xAE, 0x22, 0x2B, 0x63, 0x74, 0x5F, 0xA1, 0x01 } };
EFI_GUID    mOzmosisProprietary3Guid    = { 0x9480E8A1, 0x1793, 0x46C9, { 0x91, 0xD8, 0x11, 0x08, 0xDB, 0xA4, 0x73, 0x1C } };

APPLE_SMC_IO_PROTOCOL        *gAppleSmc = NULL;

typedef struct NVRAM_DATA
{
  CHAR16      *VariableName;
  EFI_GUID    *Guid;
} NVRAM_DATA;

CONST NVRAM_DATA ResetNvramData[] = {
  // Hibernationfixup Variables
  //{ L"Boot0082",           &gEfiGlobalVariableGuid }, { L"BootNext",       &gEfiGlobalVariableGuid },
  //{ L"IOHibernateRTCVariables", &gEfiAppleBootGuid }, { L"boot-image",     &gEfiAppleBootGuid },
  //{ L"boot-image-key",          &gEfiAppleBootGuid }, { L"boot-signature", &gEfiAppleBootGuid },
  //{ L"boot-switch-vars",        &gEfiAppleBootGuid },

  // Clover Variables
  //{ L"Clover.BackupDirOnDestVol", &gEfiAppleBootGuid }, { L"Clover.KeepBackupLimit", &gEfiAppleBootGuid },
  //{ L"Clover.LogEveryBoot",       &gEfiAppleBootGuid }, { L"Clover.LogLineCount",    &gEfiAppleBootGuid },
  //{ L"Clover.MountEFI",           &gEfiAppleBootGuid }, { L"Clover.NVRamDisk",       &gEfiAppleBootGuid },
  //{ L"Clover.Theme",              &gEfiAppleBootGuid },

  // Non-volatile Variables
  //{ L"BootOrder",       &gEfiGlobalVariableGuid }, { L"BootOptionSupport",       &gEfiGlobalVariableGuid },
  //{ L"backlight-level",      &gEfiAppleBootGuid }, { L"bootercfg",               &gEfiAppleBootGuid },
  //{ L"csr-active-config",    &gEfiAppleBootGuid }, { L"platform-uuid",           &gEfiAppleBootGuid },
  //{ L"prev-lang:kbd",        &gEfiAppleBootGuid }, { L"security-mode",           &gEfiAppleBootGuid },
  //{ L"UIScale",              &gEfiAppleBootGuid }, { L"nvda_drv",                &gEfiAppleBootGuid },
  //{ L"fmm-computer-name",    &gEfiAppleBootGuid }, { L"bluetoothActiveControllerInfo", &gEfiAppleBootGuid },
  //{ L"SystemAudioVolumeDB",  &gEfiAppleBootGuid }, { L"SystemAudioVolume",       &gEfiAppleBootGuid },
  //{ L"install-product-url",  &gEfiAppleBootGuid }, { L"previous-system-uuid",    &gEfiAppleBootGuid },
  //{ L"AAPL,PanicInfoLog",    &gEfiAppleBootGuid }, { L"AAPL,PathProperties0000", &gEfiAppleBootGuid },
  //{ L"boot-args",            &gEfiAppleBootGuid },

  // FakeSMC Variables
  //{ L"fakesmc-key-$Num-ui8",  &gEfiAppleBootGuid }, { L"fakesmc-key-$Adr-ui32", &gEfiAppleBootGuid },
  //{ L"fakesmc-key-RMde-char", &gEfiAppleBootGuid }, { L"fakesmc-key-RPlt-ch8*", &gEfiAppleBootGuid },
  //{ L"fakesmc-key-RBr -ch8*", &gEfiAppleBootGuid }, { L"fakesmc-key-EPCI-ui32", &gEfiAppleBootGuid },
  //{ L"fakesmc-key-REV -ch8*", &gEfiAppleBootGuid }, { L"fakesmc-key-BEMB-flag", &gEfiAppleBootGuid },
  //{ L"fakesmc-key-BATP-flag", &gEfiAppleBootGuid }, { L"fakesmc-key-BNum-ui8",  &gEfiAppleBootGuid },
  //{ L"fakesmc-key-BBIN-ui8",  &gEfiAppleBootGuid }, { L"fakesmc-key-MSTc-ui8",  &gEfiAppleBootGuid },
  //{ L"fakesmc-key-MSAc-ui16", &gEfiAppleBootGuid }, { L"fakesmc-key-MSWr-ui8",  &gEfiAppleBootGuid },
  //{ L"fakesmc-key-MSFW-ui8",  &gEfiAppleBootGuid }, { L"fakesmc-key-MSPS-ui16", &gEfiAppleBootGuid },
  //{ L"fakesmc-key-#KEY-ui32", &gEfiAppleBootGuid },
};

/** returns given time as miliseconds.
 *  assumes 31 days per month, so it's not correct,
 *  but is enough for basic checks.
 */
UINT64
GetEfiTimeInMs (
  IN  EFI_TIME *T
  )
{
    UINT64 TimeMs;
    
    TimeMs = T->Year - 1900;
    // is 64bit multiply workign in 32 bit?
    TimeMs = MultU64x32 (TimeMs, 12)   + T->Month;
    TimeMs = MultU64x32 (TimeMs, 31)   + T->Day; // counting with 31 day
    TimeMs = MultU64x32 (TimeMs, 24)   + T->Hour;
    TimeMs = MultU64x32 (TimeMs, 60)   + T->Minute;
    TimeMs = MultU64x32 (TimeMs, 60)   + T->Second;
    TimeMs = MultU64x32 (TimeMs, 1000) + DivU64x32(T->Nanosecond, 1000000);
    
    return TimeMs;
}

/** Reads and returns value of NVRAM variable. */
VOID *GetNvramVariable (
	IN      CHAR16   *VariableName,
	IN      EFI_GUID *VendorGuid,
	   OUT  UINT32   *Attributes    OPTIONAL,
	   OUT  UINTN    *DataSize      OPTIONAL)
{
  EFI_STATUS Status;
  VOID       *Data       = NULL;
  //
  // Pass in a zero size buffer to find the required buffer size.
  //
  UINTN      IntDataSize = 0;
  
  Status = gRT->GetVariable (VariableName,     VendorGuid, Attributes, &IntDataSize, NULL);
  if (IntDataSize == 0) {
    return NULL;
  }

  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Allocate the buffer to return
    //
    Data = AllocateZeroPool (IntDataSize + 1);
    if (Data != NULL) {
      //
      // Read variable into the allocated buffer.
      //
      Status = gRT->GetVariable (VariableName, VendorGuid, Attributes, &IntDataSize, Data);
      if (EFI_ERROR (Status)) {
        FreePool (Data);
        IntDataSize = 0;
        Data = NULL;
      }
    }
  }
  if (DataSize != NULL) {
    *DataSize = IntDataSize;
  }
  return Data;
}


/** Sets NVRAM variable. Does nothing if variable with the same data and attributes already exists. */
EFI_STATUS
SetNvramVariable (
    IN  CHAR16   *VariableName,
    IN  EFI_GUID *VendorGuid,
    IN  UINT32   Attributes,
    IN  UINTN    DataSize,
    IN  VOID     *Data
  )
{
  //EFI_STATUS Status;
  VOID   *OldData;
  UINTN  OldDataSize = 0;
  UINT32 OldAttributes = 0;
  
  //DBG ("SetNvramVariable (%s, guid, 0x%x, %d):", VariableName, Attributes, DataSize);
  OldData = GetNvramVariable (VariableName, VendorGuid, &OldAttributes, &OldDataSize);
  if (OldData != NULL) {
    // var already exists - check if it equal to new value
    //DBG (" exists(0x%x, %d)", OldAttributes, OldDataSize);
    if ((OldAttributes == Attributes) &&
        (OldDataSize == DataSize) &&
        (CompareMem (OldData, Data, DataSize) == 0)) {
      // it's the same - do nothing
      //DBG (", equal -> not writing again.\n");
      FreePool (OldData);
      return EFI_SUCCESS;
    }
    //DBG (", not equal\n");
    
    FreePool (OldData);
    
    // not the same - delete previous one if attributes are different
    if (OldAttributes != Attributes) {
      DeleteNvramVariable (VariableName, VendorGuid);
      //DBG (", diff. attr: deleting old (%r)", Status);
    }
  }
  //DBG ("\n"); // for debug without Status
  //DBG (" -> writing new (%r)\n", Status);
  //return Status;
 
  return gRT->SetVariable (VariableName, VendorGuid, Attributes, DataSize, Data);
  
}

/** Sets NVRAM variable. Does nothing if variable with the same name already exists. */
EFI_STATUS
AddNvramVariable (
	IN  CHAR16   *VariableName,
	IN  EFI_GUID *VendorGuid,
	IN  UINT32   Attributes,
	IN  UINTN    DataSize,
	IN  VOID     *Data
  )
{
  VOID       *OldData;

  //DBG ("SetNvramVariable (%s, guid, 0x%x, %d):\n", VariableName, Attributes, DataSize);
  OldData = GetNvramVariable (VariableName, VendorGuid, NULL, NULL);
  if (OldData == NULL)
  {
    // set new value
    return gRT->SetVariable (VariableName, VendorGuid, Attributes, DataSize, Data);
//  DBG (" -> writing new (%r)\n", Status);
	} else {
		FreePool (OldData);
    return EFI_ABORTED;
	}
}


/** Deletes NVRAM variable. */
EFI_STATUS
DeleteNvramVariable (
  IN  CHAR16 *VariableName,
  IN  EFI_GUID *VendorGuid
  )
{
  EFI_STATUS Status;
    
  // Delete: attributes and data size = 0
  Status = gRT->SetVariable (VariableName, VendorGuid, 0, 0, NULL);
  //DBG ("DeleteNvramVariable (%s, guid = %r):\n", VariableName, Status);
    
  return Status;
}

// Reset EmuVariable NVRAM, implemented by Sherlocks
EFI_STATUS
ResetEmuNvram ()
{
  EFI_STATUS      Status = EFI_NOT_FOUND;
  UINTN           VolumeIndex;
  //UINTN           Index, ResetNvramDataCount = ARRAY_SIZE (ResetNvramData);
  REFIT_VOLUME    *Volume;
  EFI_FILE_HANDLE FileHandle;

  //DbgHeader("ResetEmuNvram: cleanup NVRAM variables");
  //DBG("searching volumes for nvram.plist\n");

  for (VolumeIndex = 0; VolumeIndex < VolumesCount; ++VolumeIndex) {
     Volume = Volumes[VolumeIndex];
        
     if (!Volume->RootDir) {
       continue;
     }

     Status = Volume->RootDir->Open (Volume->RootDir, &FileHandle, L"nvram.plist", EFI_FILE_MODE_READ, 0);
     if (EFI_ERROR(Status)) {
       //DBG("- [%02d]: '%s' - no nvram.plist - skipping!\n", VolumeIndex, Volume->VolName);
       continue;
     }
     
     // find the partition where nvram.plist can be deleted and delete it
     if (Volume != NULL) {
       if (StriStr(Volume->VolName, L"EFI") != NULL) {
         //DBG("- [%02d]: '%s' - found nvram.plist and deleted it\n", VolumeIndex, Volume->VolName);
         Status = DeleteFile (Volume->RootDir, L"nvram.plist");
       } else {
         //DBG("- [%02d]: '%s' - found nvram.plist but can't delete it\n", VolumeIndex, Volume->VolName);
       }
     }
  }

  //DBG("ResetEmuNvram: cleanup NVRAM variables\n");
    
  /*for (Index = 0; Index < ResetNvramDataCount; Index++) {
    Status = DeleteNvramVariable(ResetNvramData[Index].VariableName, ResetNvramData[Index].Guid);
    if (EFI_ERROR(Status)) {
      DBG("- [%02d]: '%s' - not exists\n", Index, ResetNvramData[Index].VariableName);
    } else {
      DBG("- [%02d]: '%s' - deleted it\n", Index, ResetNvramData[Index].VariableName);
    }
  }*/

  return Status;
}

BOOLEAN
IsDeletableVariable (
  IN CHAR16    *Name,
  IN EFI_GUID  *Guid
  )
{

  // Apple GUIDs
  if (CompareGuid (Guid, &gEfiAppleVendorGuid) ||
      CompareGuid (Guid, &gEfiAppleBootGuid)) {
    return TRUE;

  // Global variable boot options
  } else if (CompareGuid (Guid, &gEfiGlobalVariableGuid)) {
    // Only erase boot and driver entries for BDS
    // I.e. BootOrder, Boot####, DriverOrder, Driver####
    if (!StrnCmp (Name, L"Boot", StrLen(L"Boot")) ||
        !StrnCmp (Name, L"Driver", StrLen(L"Driver"))) {
      return TRUE;
    }

  // Lilu extensions
  } else if (CompareGuid (Guid, &mLiluNormalVariableGuid) ||
      CompareGuid (Guid, &mLiluReadOnlyVariableGuid) ||
      CompareGuid (Guid, &mLiluWriteOnlyVariableGuid)) {
    return TRUE;

  // Ozmozis extensions
  } else if (CompareGuid (Guid, &mOzmosisProprietary1Guid) ||
      CompareGuid (Guid, &mOzmosisProprietary2Guid) ||
      CompareGuid (Guid, &mOzmosisProprietary3Guid)) {
    return TRUE;
  }

  return FALSE;
}

// Reset Native NVRAM by vit9696, implemented by Sherlocks
EFI_STATUS
ResetNativeNvram ()
{
  EFI_STATUS   Status = EFI_NOT_FOUND;
  EFI_GUID     CurrentGuid;
  CHAR16       *Buffer = NULL;
  CHAR16       *TmpBuffer;
  UINTN        BufferSize = 0;
  UINTN        RequestedSize = 1024;
  //UINTN         Index, ResetNvramDataCount = ARRAY_SIZE (ResetNvramData);
  BOOLEAN      Restart = TRUE;
  BOOLEAN      ForceRescan = FALSE;
    
  //DbgHeader("ResetNativeNvram: cleanup NVRAM variables");

  do {
    if (RequestedSize > BufferSize) {
      TmpBuffer = AllocateZeroPool (RequestedSize);
      if (TmpBuffer) {
        if (Buffer) {
          CopyMem (TmpBuffer, Buffer, BufferSize);
          FreePool (Buffer);
        }
        Buffer = TmpBuffer;
        BufferSize = RequestedSize;
      } else {
        //DBG ("Failed to allocate variable name buffer of %u bytes\n", (UINT32)RequestedSize);
        break;
      }
    }
        
    if (Restart) {
      ZeroMem (&CurrentGuid, sizeof(CurrentGuid));
      ZeroMem (Buffer, BufferSize);
      Restart = FALSE;
    }
        
    Status = gRT->GetNextVariableName (&RequestedSize, Buffer, &CurrentGuid);
        
    if (!EFI_ERROR (Status)) {
      if (IsDeletableVariable (Buffer, &CurrentGuid)) {
        //DBG ("Deleting %g:%s...", &CurrentGuid, Buffer);
        Status = DeleteNvramVariable(Buffer, &CurrentGuid);

        if (!EFI_ERROR (Status)) {
          //DBG ("OK\n");
          Restart = TRUE;
        } else {
          //DBG ("FAIL (%r)\n", Status);
          break;
        }
      }
    } else if (Status != EFI_BUFFER_TOO_SMALL && Status != EFI_NOT_FOUND) {
      if (!ForceRescan) {
        //DBG ("Unexpected error (%r), trying to rescan\n", Status);
        ForceRescan = TRUE;
      } else {
        //DBG ("Unexpected error (%r), aborting\n", Status);
        break;
      }
    }
  } while (Status != EFI_NOT_FOUND);
    
  if (Buffer) {
    FreePool (Buffer);
  }

  // Leave for the future
  //DBG("ResetNativeNvram: cleanup NVRAM variables\n");

  /*for (Index = 0; Index < ResetNvramDataCount; Index++) {
    Status = DeleteNvramVariable(ResetNvramData[Index].VariableName, ResetNvramData[Index].Guid);
    if (EFI_ERROR(Status)) {
      //DBG("- [%02d]: '%s' - not exists\n", Index, ResetNvramData[Index].VariableName);
    } else {
      //DBG("- [%02d]: '%s' - deleted it\n", Index, ResetNvramData[Index].VariableName);
    }
  }*/

  return Status;
}

///
//  Print all fakesmc variables, i.e. SMC keys
///
UINT32 KeyFromName(CHAR16 *Name)
{
  //fakesmc-key-CLKT-ui32: Size = 4, Data: 00 00 8C BE
  UINT32 Key;
  Key = ((Name[12] & 0xFF) << 24) + ((Name[13] & 0xFF) << 16) +
  ((Name[14] & 0xFF) << 8) + ((Name[15] & 0xFF) << 0);
  return Key;
}

UINT32 TypeFromName(CHAR16 *Name)
{
  //fakesmc-key-CLKT-ui32: Size = 4, Data: 00 00 8C BE
  UINT32 Key;
  Key = ((Name[17] & 0xFF) << 24) + ((Name[18] & 0xFF) << 16) +
  ((Name[19] & 0xFF) << 8) + ((Name[20] & 0xFF) << 0);
  if (Name[20] == L'\0') {
    Key += 0x20; //' '
  }
  return Key;
}


UINT32 FourCharKey(CHAR8 *Name)
{
  return (Name[0] << 24) + (Name[1] << 16) + (Name[2] << 8) + Name[3]; //Big Endian
}

INT8 NKey[4] = {0, 0, 0, 0};
INT8 SAdr[4] = {0, 0, 3, 0};
INT8 SNum[1] = {1};

VOID
GetSmcKeys (BOOLEAN WriteToSMC)
{
  EFI_STATUS                  Status;
  CHAR16                      *Name;
  EFI_GUID                    Guid;
  UINTN                       NameSize;
  UINTN                       NewNameSize;
  UINT8                       *Data;
  UINTN                       DataSize;
  INTN                        NumKey = 0;
  STATIC UINTN Once = 0;
  if (Once++) {
    return;
  }
  

  NameSize = sizeof (CHAR16);
  Name     = AllocateZeroPool (NameSize);
  if (Name == NULL) {
    return;
  }
  DbgHeader("Dump SMC keys from NVRAM");
  Status = gBS->LocateProtocol(&gAppleSMCProtocolGuid, NULL, (VOID**)&gAppleSmc);
  if (!EFI_ERROR(Status)) {
    DBG("found AppleSMC protocol\n");    
  } else {
    DBG("no AppleSMC protocol\n");
    gAppleSmc = NULL;
  }

  while (TRUE) {
    NewNameSize = NameSize;
    Status = gRT->GetNextVariableName (&NewNameSize, Name, &Guid);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Name = ReallocatePool (NameSize, NewNameSize, Name);
      if (Name == NULL) {
        return; //if something wrong then just do nothing
      }

      Status = gRT->GetNextVariableName (&NewNameSize, Name, &Guid);
      NameSize = NewNameSize;
    }

    if (EFI_ERROR (Status)) {
      break;  //no more variables
    }

    if (!StrStr(Name, L"fakesmc-key")) {
      continue; //the variable is not interesting for us
    }

    Data = GetNvramVariable (Name, &Guid, NULL, &DataSize);
    if (Data) {
 /*     UINTN                       Index;
      DBG("   %s:", Name);
      for (Index = 0; Index < DataSize; Index++) {
        DBG("%02x ", *((UINT8*)Data + Index));
      }
      DBG("\n"); */
      if (gAppleSmc && WriteToSMC) {
        Status = gAppleSmc->SmcAddKey(gAppleSmc, KeyFromName(Name), (SMC_DATA_SIZE)DataSize, TypeFromName(Name), 0xC0);
        if (!EFI_ERROR(Status)) {
          Status = gAppleSmc->SmcWriteValue(gAppleSmc, KeyFromName(Name), (SMC_DATA_SIZE)DataSize, Data);
          //       DBG("Write to AppleSMC status=%r\n", Status);
        }
        NumKey++;
      }
      FreePool (Data);
    }
  }
  if (WriteToSMC && gAppleSmc  && (gAppleSmc->Signature == NON_APPLE_SMC_SIGNATURE)) {
    CHAR8 Mode = SMC_MODE_APPCODE;
    NKey[3] = NumKey & 0xFF;
    NKey[2] = (NumKey >> 8) & 0xFF; //key, size, type, attr
    DBG("Registered %d SMC keys\n", NumKey);
    Status = gAppleSmc->SmcAddKey(gAppleSmc, FourCharKey("#KEY"), 4, SmcKeyTypeUint32, 0xC0);
    if (!EFI_ERROR(Status)) {
      Status = gAppleSmc->SmcWriteValue(gAppleSmc, FourCharKey("#KEY"), 4, (SMC_DATA *)&NKey);
    }
    Status = gAppleSmc->SmcAddKey(gAppleSmc, FourCharKey("$Adr"), 4, SmcKeyTypeUint32, 0x08);
    if (!EFI_ERROR(Status)) {
      Status = gAppleSmc->SmcWriteValue(gAppleSmc, FourCharKey("$Adr"), 4, (SMC_DATA *)&SAdr);
    }
    Status = gAppleSmc->SmcAddKey(gAppleSmc, FourCharKey("$Num"), 1, SmcKeyTypeUint8, 0x08);
    if (!EFI_ERROR(Status)) {
      Status = gAppleSmc->SmcWriteValue(gAppleSmc, FourCharKey("$Num"), 1, (SMC_DATA *)&SNum);
    }
    Status = gAppleSmc->SmcAddKey(gAppleSmc, FourCharKey("RMde"), 1, SmcKeyTypeChar,  0xC0);
    if (!EFI_ERROR(Status)) {
      Status = gAppleSmc->SmcWriteValue(gAppleSmc, FourCharKey("RMde"), 1, (SMC_DATA *)&Mode);
    }
  }
  FreePool (Name);
}
/*
VOID DumpSmcKeys()
{
  if (!gAppleSmc || !gAppleSmc->DumpData) {
    return;
  }
  gAppleSmc->DumpData(gAppleSmc);
}
*/

/** Searches for GPT HDD dev path node and return pointer to partition GUID or NULL. */
EFI_GUID
*FindGPTPartitionGuidInDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL *DevicePath
  )
{
  HARDDRIVE_DEVICE_PATH *HDDDevPath;
  EFI_GUID              *Guid = NULL;
  
  if (DevicePath == NULL) {
    return NULL;
  }
  
  while (!IsDevicePathEndType(DevicePath) &&
         !(DevicePathType(DevicePath) == MEDIA_DEVICE_PATH && DevicePathSubType (DevicePath) == MEDIA_HARDDRIVE_DP)) {
    DevicePath = NextDevicePathNode(DevicePath);
  }
  
  if (DevicePathType(DevicePath) == MEDIA_DEVICE_PATH && DevicePathSubType (DevicePath) == MEDIA_HARDDRIVE_DP) {
    HDDDevPath = (HARDDRIVE_DEVICE_PATH*)DevicePath;
    if (HDDDevPath->SignatureType == SIGNATURE_TYPE_GUID) {
      Guid = (EFI_GUID*)HDDDevPath->Signature;
    }
  }

  return Guid;
}


/** detailed debug for BootVolumeDevicePathEqual */
#define DBG_DP(...)
//#define DBG_DP(...) DBG (__VA_ARGS__)

/** Returns TRUE if dev paths are equal. Ignores some differences. */
BOOLEAN
BootVolumeDevicePathEqual (
  IN  EFI_DEVICE_PATH_PROTOCOL *DevicePath1,
  IN  EFI_DEVICE_PATH_PROTOCOL *DevicePath2
  )
{
  BOOLEAN          Equal;
  UINT8            Type1;
  UINT8            SubType1;
  UINT8            Type2;
  UINTN            Len1;
  UINT8            SubType2;
  UINTN            Len2;
  SATA_DEVICE_PATH *SataNode1;
  SATA_DEVICE_PATH *SataNode2;
  BOOLEAN          ForceEqualNodes;


  DBG_DP ("   BootVolumeDevicePathEqual:\n    %s\n    %s\n", FileDevicePathToStr (DevicePath1), FileDevicePathToStr (DevicePath2));
  DBG_DP ("    N1: (Type, Subtype, Len) N2: (Type, Subtype, Len)\n");
  
  Equal = FALSE;
  while (TRUE) {
    Type1    = DevicePathType (DevicePath1);
    SubType1 = DevicePathSubType (DevicePath1);
    Len1     = DevicePathNodeLength (DevicePath1);
    
    Type2    = DevicePathType (DevicePath2);
    SubType2 = DevicePathSubType (DevicePath2);
    Len2     = DevicePathNodeLength (DevicePath2);
    
    ForceEqualNodes = FALSE;
    
    DBG_DP ("    N1: (%d, %d, %d)", Type1, SubType1, Len1);
    DBG_DP (" N2: (%d, %d, %d)", Type2, SubType2, Len2);
    /*
     DBG_DP ("%s\n", DevicePathToStr (DevicePath1));
     DBG_DP ("%s\n", DevicePathToStr (DevicePath2));
     */
    
    //
    // Some eSata device can have path:
    //  PciRoot(0x0)/Pci(0x1C,0x5)/Pci(0x0,0x0)/VenHw(CF31FAC5-C24E-11D2-85F3-00A0C93EC93B,80)
    // while macOS can set it as
    //  PciRoot(0x0)/Pci(0x1C,0x5)/Pci(0x0,0x0)/Sata(0x0,0x0,0x0)
    // we'll assume VenHw and Sata nodes to be equal to cover that
    // as well add NVME to this comparison
    //
    if (Type1 == MESSAGING_DEVICE_PATH && SubType1 == MSG_SATA_DP) {
      if ((Type2 == HARDWARE_DEVICE_PATH && SubType2 == HW_VENDOR_DP)
          || (Type2 == MESSAGING_DEVICE_PATH && SubType2 == MSG_NVME_NAMESPACE_DP)) {
        ForceEqualNodes = TRUE;
      }
    } else if (Type2 == MESSAGING_DEVICE_PATH && SubType2 == MSG_SATA_DP &&
              ((Type1 == HARDWARE_DEVICE_PATH && SubType1 == HW_VENDOR_DP)
                || (Type1 == MESSAGING_DEVICE_PATH && SubType1 == MSG_NVME_NAMESPACE_DP))) {
      ForceEqualNodes = TRUE;
    }
    
    //
    // UEFI can see it as PcieRoot, while macOS could generate PciRoot
    // we'll assume Acpi dev path nodes to be equal to cover that
    //
    if (Type1 == ACPI_DEVICE_PATH && Type2 == ACPI_DEVICE_PATH) {
      ForceEqualNodes = TRUE;
    }
    
    if (ForceEqualNodes) {
      // assume equal nodes
      DBG_DP (" - forcing equal nodes\n");
      DevicePath1 = NextDevicePathNode (DevicePath1);
      DevicePath2 = NextDevicePathNode (DevicePath2);
      continue;
    }
    
    if (Type1 != Type2 || SubType1 != SubType2 || Len1 != Len2) {
      // Not equal
      DBG_DP (" - not equal\n");
      break;
    }
    
    //
    // Same type/subtype/len ...
    //
    if (IsDevicePathEnd (DevicePath1)) {
      // END node - they are the same
      Equal = TRUE;
      DBG_DP (" - END = equal\n");
      break;
    }
    
    //
    // Do mem compare of nodes or special compare for selected types/subtypes
    //
    if (Type1 == MESSAGING_DEVICE_PATH && SubType1 == MSG_SATA_DP) {
      //
      // Ignore 
      //
      SataNode1 = (SATA_DEVICE_PATH *)DevicePath1;
      SataNode2 = (SATA_DEVICE_PATH *)DevicePath2;
      if (SataNode1->HBAPortNumber != SataNode2->HBAPortNumber) {
        // not equal
        DBG_DP (" - not equal SataNode.HBAPortNumber\n");
        break;
      }

      if (SataNode1->Lun != SataNode2->Lun) {
        // not equal
        DBG_DP (" - not equal SataNode.Lun\n");
        break;
      }
      DBG_DP (" - forcing equal nodes");
    } else if (CompareMem (DevicePath1, DevicePath2, DevicePathNodeLength (DevicePath1)) != 0) {
        // Not equal
        DBG_DP (" - not equal\n");
        break;
    }
    
    DBG_DP ("\n");
    //
    // Advance to next node
    //
    DevicePath1 = NextDevicePathNode (DevicePath1);
    DevicePath2 = NextDevicePathNode (DevicePath2);
  }
  
  return Equal;
}


/** Returns TRUE if dev paths contain the same MEDIA_DEVICE_PATH. */
BOOLEAN
BootVolumeMediaDevicePathNodesEqual (
  IN  EFI_DEVICE_PATH_PROTOCOL *DevicePath1,
  IN  EFI_DEVICE_PATH_PROTOCOL *DevicePath2
  )
{
    DevicePath1 = FindDevicePathNodeWithType (DevicePath1, MEDIA_DEVICE_PATH, 0);
    if (DevicePath1 == NULL) {
        return FALSE;
    }

    DevicePath2 = FindDevicePathNodeWithType (DevicePath2, MEDIA_DEVICE_PATH, 0);
    if (DevicePath2 == NULL) {
        return FALSE;
    }
    
    return (DevicePathNodeLength (DevicePath1) == DevicePathNodeLength (DevicePath1))
            && (CompareMem (DevicePath1, DevicePath2, DevicePathNodeLength (DevicePath1)) == 0);
}


/** Reads gEfiAppleBootGuid:efi-boot-device-data and BootCampHD NVRAM variables and parses them
 *  into gEfiBootVolume, gEfiBootLoaderPath and gEfiBootDeviceGuid.
 *  Vars after this call:
 *   gEfiBootDeviceData - original efi-boot-device-data
 *   gBootCampHD - if gEfiBootDeviceData starts with MemoryMapped node, then BootCampHD variable (device path), NULL otherwise
 *   gEfiBootVolume - volume device path (from efi-boot-device-data or BootCampHD)
 *   gEfiBootLoaderPath - file path (from efi-boot-device-data or BootCampHD) or NULL
 *   gEfiBootDeviceGuid - GPT volume GUID if gEfiBootVolume or NULL
 */
EFI_STATUS
GetEfiBootDeviceFromNvram ()
{
  UINTN                Size = 0;
  EFI_GUID             *Guid;
  FILEPATH_DEVICE_PATH *FileDevPath;
  
  
  DbgHeader("GetEfiBootDeviceFromNvram");
//  DBG ("GetEfiBootDeviceFromNvram:");
  
  if (gEfiBootDeviceData != NULL) {
//    DBG (" - [!] already parsed\n");
    return EFI_SUCCESS;
  }

  gEfiBootDeviceData = GetNvramVariable (L"efi-boot-next-data", &gEfiAppleBootGuid, NULL, &Size);
  if (gEfiBootDeviceData != NULL) {
//    DBG("Got efi-boot-next-data size=%d\n", Size);
    if (IsDevicePathValid(gEfiBootDeviceData, Size)) {
//      DBG(" - efi-boot-next-data: %s\n", FileDevicePathToStr (gEfiBootDeviceData));
    } else {
//      DBG(" - device path for efi-boot-next-data is invalid\n");
      FreePool(gEfiBootDeviceData);
      gEfiBootDeviceData = NULL;
    }
  }
  if (gEfiBootDeviceData == NULL) {
    VOID *Value;
    UINTN Size2=0;
    EFI_STATUS Status;
    Status = GetVariable2 (L"aptiofixflag", &gEfiAppleBootGuid, &Value, &Size2);
    if (EFI_ERROR(Status)) {
      gEfiBootDeviceData = GetNvramVariable (L"efi-boot-device-data", &gEfiAppleBootGuid, NULL, &Size);
    } else {
      gEfiBootDeviceData = GetNvramVariable (L"specialbootdevice", &gEfiAppleBootGuid, NULL, &Size);
    }
    
    if (gEfiBootDeviceData != NULL) {
      //      DBG("Got efi-boot-device-data size=%d\n", Size);
      if (!IsDevicePathValid(gEfiBootDeviceData, Size)) {
        //        DBG(" - device path for efi-boot-device-data is invalid\n");
        FreePool(gEfiBootDeviceData);
        gEfiBootDeviceData = NULL;
      }
    }
  }
  if (gEfiBootDeviceData == NULL) {
//    DBG (" - [!] efi-boot-device-data not found\n");
    return EFI_NOT_FOUND;
  }
  
//  DBG ("\n");
  DBG (" - efi-boot-device-data: %s\n", FileDevicePathToStr (gEfiBootDeviceData));
  
  gEfiBootVolume = gEfiBootDeviceData;
  
  //
  // if gEfiBootDeviceData starts with MemoryMapped node,
  // then Startup Disk sets BootCampHD to Win disk dev path.
  //
  if (DevicePathType(gEfiBootDeviceData) == HARDWARE_DEVICE_PATH && DevicePathSubType (gEfiBootDeviceData) == HW_MEMMAP_DP) {
    gBootCampHD = GetNvramVariable (L"BootCampHD", &gEfiAppleBootGuid, NULL, &Size);
    gEfiBootVolume = gBootCampHD;

    if (gBootCampHD == NULL) {
//      DBG ("  - [!] Error: BootCampHD not found\n");
      return EFI_NOT_FOUND;
    }

    if (!IsDevicePathValid(gBootCampHD, Size)) {
//      DBG (" Error: BootCampHD device path is invalid\n");
      FreePool(gBootCampHD);
      gEfiBootVolume = gBootCampHD = NULL;
      return EFI_NOT_FOUND;
    }

    DBG ("  - BootCampHD: %s\n", FileDevicePathToStr (gBootCampHD));
  }
  
  //
  // if gEfiBootVolume contains FilePathNode, then split them into gEfiBootVolume dev path and gEfiBootLoaderPath
  //
  gEfiBootLoaderPath = NULL;
  FileDevPath = (FILEPATH_DEVICE_PATH *)FindDevicePathNodeWithType (gEfiBootVolume, MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP);
  if (FileDevPath != NULL) {
    gEfiBootLoaderPath = AllocateCopyPool (StrSize(FileDevPath->PathName), FileDevPath->PathName);
    // copy DevPath and write end of path node after in place of file path node
    gEfiBootVolume = DuplicateDevicePath (gEfiBootVolume);
    FileDevPath = (FILEPATH_DEVICE_PATH *)FindDevicePathNodeWithType (gEfiBootVolume, MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP);
    SetDevicePathEndNode (FileDevPath);
    // gEfiBootVolume now contains only Volume path
  }

  DBG ("  - Volume: '%s'\n", FileDevicePathToStr (gEfiBootVolume));
  DBG ("  - LoaderPath: '%s'\n", gEfiBootLoaderPath);
  
  //
  // if this is GPT disk, extract GUID
  // gEfiBootDeviceGuid can be used as a flag for GPT disk then
  //
  Guid = FindGPTPartitionGuidInDevicePath (gEfiBootVolume);
  if (Guid != NULL) {
    gEfiBootDeviceGuid = AllocatePool (sizeof(EFI_GUID));
    if (gEfiBootDeviceGuid != NULL) {
      CopyMem (gEfiBootDeviceGuid, Guid, sizeof(EFI_GUID));
      DBG ("  - Guid = %g\n", gEfiBootDeviceGuid);
    }
  }
  
  return EFI_SUCCESS;
}


/** Loads and parses nvram.plist into gNvramDict. */
EFI_STATUS
LoadNvramPlist (
  IN  EFI_FILE *RootDir,
  IN  CHAR16* NVRAMPlistPath
  )
{
    EFI_STATUS Status;
    CHAR8      *NvramPtr;
    UINTN      Size;
    
    
    //
    // skip loading if already loaded
    //
    if (gNvramDict != NULL) {
        return EFI_SUCCESS;
    }
    
    //
    // load nvram.plist
    //
    Status = egLoadFile (RootDir, NVRAMPlistPath, (UINT8**)&NvramPtr, &Size);
    if(EFI_ERROR(Status)) {
//        DBG (" not present\n");
        return Status;
    }

    DBG (" loaded, size=%d\n", Size);
    
    //
    // parse it into gNvramDict 
    //
    Status = ParseXML ((const CHAR8*)NvramPtr, &gNvramDict, (UINT32)Size);
//    if(Status != EFI_SUCCESS) {
//        DBG (" parsing error\n");
//    }
    
    FreePool (NvramPtr);
    // we will leave nvram.plist loaded and parsed for later processing
    //FreeTag(gNvramDict);
    
    return Status;
}


/** Searches all volumes for the most recent nvram.plist and loads it into gNvramDict. */
EFI_STATUS
LoadLatestNvramPlist ()
{
  EFI_STATUS      Status;
  UINTN           Index;
  REFIT_VOLUME    *Volume;
//  EFI_GUID        *Guid;
  EFI_FILE_HANDLE FileHandle;
  EFI_FILE_INFO   *FileInfo;
  UINT64          LastModifTimeMs;
  UINT64          ModifTimeMs;
  REFIT_VOLUME    *VolumeWithLatestNvramPlist;
  
//there are debug messages not needed for users
//  DBG ("Searching volumes for latest nvram.plist ...");
  
  //
  // skip loading if already loaded
  //
  if (gNvramDict != NULL) {
 //   DBG (" already loaded\n");
    return EFI_SUCCESS;
  }
//  DBG ("\n");
  
  //
  // find latest nvram.plist
  //
  
  LastModifTimeMs = 0;
  VolumeWithLatestNvramPlist = NULL;
  
  // search all volumes
  for (Index = 0; Index < VolumesCount; ++Index) {
    Volume = Volumes[Index];
    
    if (!Volume->RootDir) {
      continue;
    }
    
/*    Guid = FindGPTPartitionGuidInDevicePath (Volume->DevicePath);
    
    DBG (" %2d. Volume '%s', GUID = %g", Index, Volume->VolName, Guid);
    if (Guid == NULL) {
      // not a GUID partition
      DBG (" - not GPT");
    } */
    
    // check if nvram.plist exists
    Status = Volume->RootDir->Open (Volume->RootDir, &FileHandle, L"nvram.plist", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) {
//      DBG (" - no nvram.plist - skipping!\n");
      continue;
    }
    
    if (GlobalConfig.FastBoot) {
      VolumeWithLatestNvramPlist = Volume;
      break;
    }
    
    // get nvram.plist modification date
    FileInfo = EfiLibFileInfo(FileHandle);
    if (FileInfo == NULL) {
//      DBG (" - no nvram.plist file info - skipping!\n");
      FileHandle->Close(FileHandle);
      continue;
    }
    
//    DBG (" Modified = ");
    ModifTimeMs = GetEfiTimeInMs (&FileInfo->ModificationTime);
/*    DBG ("%d-%d-%d %d:%d:%d (%ld ms)",
        FileInfo->ModificationTime.Year, FileInfo->ModificationTime.Month, FileInfo->ModificationTime.Day,
        FileInfo->ModificationTime.Hour, FileInfo->ModificationTime.Minute, FileInfo->ModificationTime.Second,
        ModifTimeMs); */
    FreePool (FileInfo);
    FileHandle->Close(FileHandle);
    
    // check if newer
    if (LastModifTimeMs < ModifTimeMs) {
//      DBG (" - newer - will use this one\n");
      VolumeWithLatestNvramPlist = Volume;
      LastModifTimeMs = ModifTimeMs;
    } else {
//      DBG (" - older - skipping!\n");
    }
  }
  
  Status = EFI_NOT_FOUND;
  
  //
  // if we have nvram.plist - load it
  //
  if (VolumeWithLatestNvramPlist != NULL) {
    DBG ("Loading nvram.plist from Vol '%s' -", VolumeWithLatestNvramPlist->VolName);
    Status = LoadNvramPlist (VolumeWithLatestNvramPlist->RootDir, L"nvram.plist");
    
  } else {
 //   DBG (" nvram.plist not found!\n");
  }
  
  return Status;
}


/** Puts all vars from nvram.plist to RT vars. Should be used in CloverEFI only
 *  or if some UEFI boot uses EmuRuntimeDxe driver.
 */
VOID
PutNvramPlistToRtVars ()
{
  EFI_STATUS Status;
  TagPtr     Tag;
  TagPtr     ValTag;
  INTN       Size, i;
  CHAR16     KeyBuf[128];
  VOID       *Value;
  
  if (gNvramDict == NULL) {
    Status = LoadLatestNvramPlist ();
    if (gNvramDict == NULL) {
      DBG ("PutNvramPlistToRtVars: nvram.plist not found\n");
      return;
    }
  }
  
  DbgHeader("PutNvramPlistToRtVars");
//  DBG ("PutNvramPlistToRtVars ...\n");
  // iterate over dict elements
  for (Tag = gNvramDict->tag; Tag != NULL; Tag = Tag->tagNext) {
    EFI_GUID *VendorGuid = &gEfiAppleBootGuid;
    Value  = NULL;
    ValTag = (TagPtr)Tag->tag;

    // process only valid <key> tags
    if (Tag->type != kTagTypeKey || ValTag == NULL) {
      DBG (" ERROR: Tag is not <key>, type = %d\n", Tag->type);
      continue;
    }
//    DBG("tag: %a\n", Tag->string);
    // skip OsxAptioFixDrv-RelocBase - appears and causes trouble
    // in kernel and kext patcher when mixing UEFI and CloverEFI boot
    if (AsciiStrCmp (Tag->string, "OsxAptioFixDrv-RelocBase") == 0) {
      DBG (" Skipping OsxAptioFixDrv-RelocBase\n");
      continue;
    } else if (AsciiStrCmp (Tag->string, "OsxAptioFixDrv-ErrorExitingBootServices") == 0) {
      DBG (" Skipping OsxAptioFixDrv-ErrorExitingBootServices\n");
      continue;
    } else if (AsciiStrCmp (Tag->string, "EmuVariableUefiPresent") == 0) {
      DBG (" Skipping EmuVariableUefiPresent\n");
      continue;
    } else if (AsciiStrCmp (Tag->string, "aapl,panic-info") == 0) {
        DBG (" Skipping aapl,panic-info\n");
        continue;
    }

    // key to unicode; check if key buffer is large enough
    if (AsciiStrLen (Tag->string) > (sizeof(KeyBuf) / 2 - 1)) {
      DBG (" ERROR: Skipping too large key %s\n", Tag->string);
      continue;
    }

    if (AsciiStrCmp (Tag->string, "Boot0082") == 0 || AsciiStrCmp (Tag->string, "BootNext") == 0) {
      VendorGuid = &gEfiGlobalVariableGuid;
      // it may happen only in this case
      GlobalConfig.HibernationFixup = TRUE;
    }

    AsciiStrToUnicodeStrS(Tag->string, KeyBuf, 128);
    if (!GlobalConfig.DebugLog) {
      DBG (" Adding Key: %s: ", KeyBuf);
    }
    // process value tag
    
    if (ValTag->type == kTagTypeString) {
      
      // <string> element
      Value = ValTag->string;
      Size  = AsciiStrLen (Value);
      if (!GlobalConfig.DebugLog) {
        DBG ("String: Size = %d, Val = '%a'\n", Size, Value);
      }
      
    } else if (ValTag->type == kTagTypeData) {
      
      // <data> element
      Size  = ValTag->dataLen;
      Value = ValTag->data;
      if (!GlobalConfig.DebugLog) {
        DBG ("Size = %d, Data: ", Size);
        for (i = 0; i < Size; i++) {
          DBG("%02x ", *((UINT8*)Value + i));
        }
      }
      if (!GlobalConfig.DebugLog) {
       DBG ("\n");
      }
    } else {
      DBG ("ERROR: Unsupported tag type: %d\n", ValTag->type);
      continue;
    }
    
    if (Size == 0 || !Value) {
      continue;
    }
    
    // set RT var: all vars visible in nvram.plist are gEfiAppleBootGuid
/*   Status = gRS->SetVariable (
                    KeyBuf,
                    VendorGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    Size,
                    Value
                    ); */

    SetNvramVariable (
                      KeyBuf,
                      VendorGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      Size,
                      Value
                      );
  }
}


/** Performs detailed search for Startup Disk or last Clover boot volume
 *  by looking for gEfiAppleBootGuid:efi-boot-device-data and BootCampHD RT vars.
 *  Returns MainMenu entry index or -1 if not found.
 */
INTN
FindStartupDiskVolume (
  REFIT_MENU_SCREEN *MainMenu
  )
{
  INTN         Index;
  LEGACY_ENTRY *LegacyEntry;
  LOADER_ENTRY *LoaderEntry;
  REFIT_VOLUME *Volume;
  REFIT_VOLUME *DiskVolume;
  BOOLEAN      IsPartitionVolume;
  CHAR16       *LoaderPath;
  CHAR16       *EfiBootVolumeStr;
  
  
//  DBG ("FindStartupDiskVolume ...\n");
  
  
  //
  // search RT vars for efi-boot-device-data
  // and try to find that volume
  //
  GetEfiBootDeviceFromNvram ();
  if (gEfiBootVolume == NULL) {
//    DBG (" - [!] EfiBootVolume not found\n");
    return -1;
  }
  
  DbgHeader("FindStartupDiskVolume");
//  DBG ("FindStartupDiskVolume searching ...\n");
  
  //
  // Check if gEfiBootVolume is disk or partition volume
  //
  EfiBootVolumeStr  = FileDevicePathToStr (gEfiBootVolume);
  IsPartitionVolume = NULL != FindDevicePathNodeWithType (gEfiBootVolume, MEDIA_DEVICE_PATH, 0);
  DBG ("  - Volume: %s = %s\n", IsPartitionVolume ? L"partition" : L"disk", EfiBootVolumeStr);
  
  //
  // 1. gEfiBootVolume + gEfiBootLoaderPath
  // PciRoot(0x0)/.../Sata(...)/HD(...)/\EFI\BOOT\XXX.EFI - set by Clover
  //
  if (gEfiBootLoaderPath != NULL) {
    DBG ("   - searching for that partition and loader '%s'\n", gEfiBootLoaderPath);
    for (Index = 0; ((Index < (INTN)MainMenu->EntryCount) && (MainMenu->Entries[Index]->Row == 0)); ++Index) {
      if (MainMenu->Entries[Index]->Tag == TAG_LOADER) {
        LoaderEntry = (LOADER_ENTRY *)MainMenu->Entries[Index];
        Volume = LoaderEntry->Volume;
        LoaderPath = LoaderEntry->LoaderPath;
        if (Volume != NULL && BootVolumeDevicePathEqual(gEfiBootVolume, Volume->DevicePath)) {
          //DBG ("  checking '%s'\n", DevicePathToStr (Volume->DevicePath));
          //DBG ("   '%s'\n", LoaderPath);
          // case insensitive cmp
          if (LoaderPath != NULL && StriCmp(gEfiBootLoaderPath, LoaderPath) == 0) {
            // that's the one
            DBG ("    - found entry %d. '%s', Volume '%s', '%s'\n", Index, LoaderEntry->me.Title, Volume->VolName, LoaderPath);
            return Index;
          }
        }
      }
    }
    DBG ("    - [!] not found\n");
    //
    // search again, but compare only Media dev path nodes
    // (in case of some dev path differences we do not cover)
    //
    DBG ("   - searching again, but comparing Media dev path nodes\n");
    for (Index = 0; ((Index < (INTN)MainMenu->EntryCount) && (MainMenu->Entries[Index]->Row == 0)); ++Index) {
      if (MainMenu->Entries[Index]->Tag == TAG_LOADER) {
        LoaderEntry = (LOADER_ENTRY *)MainMenu->Entries[Index];
        Volume = LoaderEntry->Volume;
        LoaderPath = LoaderEntry->LoaderPath;
        if (Volume != NULL && BootVolumeMediaDevicePathNodesEqual (gEfiBootVolume, Volume->DevicePath)) {
          //DBG ("  checking '%s'\n", DevicePathToStr (Volume->DevicePath));
          //DBG ("   '%s'\n", LoaderPath);
          // case insensitive cmp
          if (LoaderPath != NULL && StriCmp(gEfiBootLoaderPath, LoaderPath) == 0) {
            // that's the one
            DBG ("   - found entry %d. '%s', Volume '%s', '%s'\n", Index, LoaderEntry->me.Title, Volume->VolName, LoaderPath);
            return Index;
          }
        }
      }
    }
    DBG ("    - [!] not found\n");
  }
  
  //
  // 2. gEfiBootVolume - partition volume
  // PciRoot(0x0)/.../Sata(...)/HD(...) - set by Clover or macOS
  //
  if (IsPartitionVolume) {
    DBG ("   - searching for that partition\n");
    for (Index = 0; ((Index < (INTN)MainMenu->EntryCount) && (MainMenu->Entries[Index]->Row == 0)); ++Index) {
      Volume = NULL;
      if (MainMenu->Entries[Index]->Tag == TAG_LEGACY) {
        LegacyEntry = (LEGACY_ENTRY *)MainMenu->Entries[Index];
        Volume = LegacyEntry->Volume;
      } else if (MainMenu->Entries[Index]->Tag == TAG_LOADER) {
        LoaderEntry = (LOADER_ENTRY *)MainMenu->Entries[Index];
        Volume = LoaderEntry->Volume;
      }
      if (Volume != NULL && BootVolumeDevicePathEqual (gEfiBootVolume, Volume->DevicePath)) {
        DBG ("    - found entry %d. '%s', Volume '%s'\n", Index, MainMenu->Entries[Index]->Title, Volume->VolName);
        return Index;
      }
    }
    DBG ("    - [!] not found\n");
    //
    // search again, but compare only Media dev path nodes
    //
    DBG ("   - searching again, but comparing Media dev path nodes\n");
    for (Index = 0; ((Index < (INTN)MainMenu->EntryCount) && (MainMenu->Entries[Index]->Row == 0)); ++Index) {
      Volume = NULL;
      if (MainMenu->Entries[Index]->Tag == TAG_LEGACY) {
        LegacyEntry = (LEGACY_ENTRY *)MainMenu->Entries[Index];
        Volume = LegacyEntry->Volume;
      } else if (MainMenu->Entries[Index]->Tag == TAG_LOADER) {
        LoaderEntry = (LOADER_ENTRY *)MainMenu->Entries[Index];
        Volume = LoaderEntry->Volume;
      }
      if (Volume != NULL && BootVolumeMediaDevicePathNodesEqual (gEfiBootVolume, Volume->DevicePath)) {
        DBG ("    - found entry %d. '%s', Volume '%s'\n", Index, MainMenu->Entries[Index]->Title, Volume->VolName);
        return Index;
      }
    }
    DBG ("    - [!] not found\n");
    return -1;
  }
  
  //
  // 3. gEfiBootVolume - disk volume
  // PciRoot(0x0)/.../Sata(...) - set by macOS for Win boot
  //
  // 3.1 First find disk volume in Volumes[]
  //
  DiskVolume = NULL;
  DBG ("   - searching for that disk\n");
  for (Index = 0; Index < (INTN)VolumesCount; ++Index) {
    Volume = Volumes[Index];
    if (BootVolumeDevicePathEqual (gEfiBootVolume, Volume->DevicePath)) {
      // that's the one
      DiskVolume = Volume;
      DBG ("    - found disk as volume %d. '%s'\n", Index, Volume->VolName);
      break;
    }
  }
  if (DiskVolume == NULL) {
    DBG ("    - [!] not found\n");
    return -1;
  }
  
  //
  // 3.2 DiskVolume
  // search for first entry with win loader or win partition on that disk
  //
  DBG ("   - searching for first entry with win loader or win partition on that disk\n");
  for (Index = 0; ((Index < (INTN)MainMenu->EntryCount) && (MainMenu->Entries[Index]->Row == 0)); ++Index) {
    if (MainMenu->Entries[Index]->Tag == TAG_LEGACY) {
      LegacyEntry = (LEGACY_ENTRY *)MainMenu->Entries[Index];
      Volume = LegacyEntry->Volume;
      if (Volume != NULL && Volume->WholeDiskBlockIO == DiskVolume->BlockIO) {
        // check for Win
        //DBG ("  checking legacy entry %d. %s\n", Index, LegacyEntry->me.Title);
        //DBG ("   %s\n", DevicePathToStr (Volume->DevicePath));
        //DBG ("   OSType = %d\n", Volume->OSType);
        if (Volume->LegacyOS->Type == OSTYPE_WIN) {
          // that's the one - legacy win partition
          DBG ("    - found legacy entry %d. '%s', Volume '%s'\n", Index, LegacyEntry->me.Title, Volume->VolName);
          return Index;
        }
      }
    } else if (MainMenu->Entries[Index]->Tag == TAG_LOADER) {
      LoaderEntry = (LOADER_ENTRY *)MainMenu->Entries[Index];
      Volume = LoaderEntry->Volume;
      if (Volume != NULL && Volume->WholeDiskBlockIO == DiskVolume->BlockIO) {
        // check for Win
        //DBG ("  checking loader entry %d. %s\n", Index, LoaderEntry->me.Title);
        //DBG ("   %s\n", DevicePathToStr (Volume->DevicePath));
        //DBG ("   LoaderPath = %s\n", LoaderEntry->LoaderPath);
        //DBG ("   LoaderType = %d\n", LoaderEntry->LoaderType);
        if (LoaderEntry->LoaderType == OSTYPE_WINEFI) {
          // that's the one - win loader entry
          DBG ("    - found loader entry %d. '%s', Volume '%s', '%s'\n", Index, LoaderEntry->me.Title, Volume->VolName, LoaderEntry->LoaderPath);
          return Index;
        }
      }
    }
  }
  DBG ("    - [!] not found\n");
  
  //
  // 3.3 DiskVolume, but no Win entry
  // PciRoot(0x0)/.../Sata(...)
  // just find first menu entry on that disk?
  //
  DBG ("   - searching for any entry from disk '%s'\n", DiskVolume->VolName);
  for (Index = 0; ((Index < (INTN)MainMenu->EntryCount) && (MainMenu->Entries[Index]->Row == 0)); ++Index) {
    if (MainMenu->Entries[Index]->Tag == TAG_LEGACY) {
      LegacyEntry = (LEGACY_ENTRY *)MainMenu->Entries[Index];
      Volume = LegacyEntry->Volume;
      if (Volume != NULL && Volume->WholeDiskBlockIO == DiskVolume->BlockIO) {
        // that's the one
        DBG ("    - found legacy entry %d. '%s', Volume '%s'\n", Index, LegacyEntry->me.Title, Volume->VolName);

        return Index;
      }
    } else if (MainMenu->Entries[Index]->Tag == TAG_LOADER) {
      LoaderEntry = (LOADER_ENTRY *)MainMenu->Entries[Index];
      Volume = LoaderEntry->Volume;
      if (Volume != NULL && Volume->WholeDiskBlockIO == DiskVolume->BlockIO) {
        // that's the one
        DBG ("    - found loader entry %d. '%s', Volume '%s', '%s'\n", Index, LoaderEntry->me.Title, Volume->VolName, LoaderEntry->LoaderPath);

        return Index;
      }
    }
  }
  
  DBG ("    - [!] not found\n");
  return -1;
}


/** Sets efi-boot-device-data RT var to currently selected Volume and LoadePath. */
EFI_STATUS SetStartupDiskVolume (
  IN  REFIT_VOLUME *Volume,
  IN  CHAR16       *LoaderPath
  )
{
  EFI_STATUS               Status;
  EFI_DEVICE_PATH_PROTOCOL *DevPath;
  EFI_DEVICE_PATH_PROTOCOL *FileDevPath;
  EFI_GUID                 *Guid;
  CHAR8                    *EfiBootDevice;
  CHAR8                    *EfiBootDeviceTmpl;
  UINTN                    Size;
  UINT32                   Attributes;
  
  
  DBG ("SetStartupDiskVolume:\n");
  DBG ("  * Volume: '%s'\n",     Volume->VolName);
  DBG ("  * LoaderPath: '%s'\n", LoaderPath);
  
  //
  // construct dev path for Volume/LoaderPath
  //
  DevPath = Volume->DevicePath;
  if (LoaderPath != NULL) {
    FileDevPath = FileDevicePath (NULL, LoaderPath);
    DevPath     = AppendDevicePathNode (DevPath, FileDevPath);
  }
  DBG ("  * DevPath: %s\n", Volume->VolName, FileDevicePathToStr (DevPath));
  
  Guid = FindGPTPartitionGuidInDevicePath (Volume->DevicePath);
  DBG ("  * GUID = %g\n", Guid);
  
  //
  // let's save it without EFI_VARIABLE_NON_VOLATILE in CloverEFI like other vars so far
  //
  if (!gFirmwareClover && !gDriversFlags.EmuVariableLoaded) {
    Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
  } else {
    Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
  }
  
  //
  // set efi-boot-device-data to volume dev path
  //
  Status = SetNvramVariable (L"efi-boot-device-data", &gEfiAppleBootGuid, Attributes, GetDevicePathSize(DevPath), DevPath);

  if (EFI_ERROR(Status)) {
    return Status;
  }
  
  //
  // set efi-boot-device to XML string
  // (probably not needed at all)
  //
  if (Guid != NULL) {
    EfiBootDeviceTmpl =
      "<array><dict>"
	    "<key>IOMatch</key>"
	    "<dict>"
	    "<key>IOProviderClass</key><string>IOMedia</string>"
	    "<key>IOPropertyMatch</key>"
	    "<dict><key>UUID</key><string>%g</string></dict>"
	    "</dict>"
	    "</dict></array>";

    Size          = AsciiStrLen (EfiBootDeviceTmpl) + 36;
    EfiBootDevice = AllocateZeroPool(AsciiStrLen (EfiBootDeviceTmpl) + 36);
    AsciiSPrint (EfiBootDevice, Size, EfiBootDeviceTmpl, Guid);
    Size          = AsciiStrLen (EfiBootDevice);
    DBG ("  * efi-boot-device: %a\n", EfiBootDevice);
    
    Status        = SetNvramVariable (L"efi-boot-device", &gEfiAppleBootGuid, Attributes, Size, EfiBootDevice);

    FreePool (EfiBootDevice);
  }
  
  return Status;
}


/** Deletes Startup disk vars: efi-boot-device, efi-boot-device-data, BootCampHD. */
VOID
RemoveStartupDiskVolume ()
{
//    EFI_STATUS Status;
    
//    DBG ("RemoveStartupDiskVolume:\n");
    
    /*Status =*/ DeleteNvramVariable (L"efi-boot-device", &gEfiAppleBootGuid);
//    DBG ("  * efi-boot-device = %r\n", Status);
    
    /*Status =*/ DeleteNvramVariable (L"efi-boot-device-data", &gEfiAppleBootGuid);
//    DBG ("  * efi-boot-device-data = %r\n", Status);
    
    /*Status =*/ DeleteNvramVariable (L"BootCampHD", &gEfiAppleBootGuid);
//    DBG ("  * BootCampHD = %r\n", Status);
//    DBG ("Removed efi-boot-device-data variable: %r\n", Status);
}
