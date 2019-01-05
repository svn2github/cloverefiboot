/*
 * File: AudioDxe.h
 *
 * Copyright (c) 2018 John Davis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _EFI_AUDIODXE_H_
#define _EFI_AUDIODXE_H_

//
// Common UEFI includes and library classes.
//
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Library/HdaVerbs.h>
#include <IndustryStandard/HdaCodec.h>
//
// Proctols that are consumed/produced.
//
#include <Protocol/AudioIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathUtilities.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/HdaIo.h>
#include <Protocol/HdaCodecInfo.h>
#include <Protocol/HdaControllerInfo.h>

// Driver version
#define AUDIODXE_VERSION        0xA
#define AUDIODXE_PKG_VERSION    1

#define MS_TO_MICROSECOND(a) ((a) * 1000)
#define MS_TO_NANOSECOND(a)  ((a) * 1000000)

// Driver Bindings.
extern EFI_DRIVER_BINDING_PROTOCOL gHdaControllerDriverBinding;
extern EFI_DRIVER_BINDING_PROTOCOL gHdaCodecDriverBinding;

#define GET_PCI_VENDOR_ID(a)    (a & 0xFFFF)
#define GET_PCI_DEVICE_ID(a)    ((a >> 16) & 0xFFFF)
#define GET_PCI_GENERIC_ID(a)   ((0xFFFF << 16) | a)
#define GET_CODEC_VENDOR_ID(a)  ((a >> 16) & 0xFFFF)
#define GET_CODEC_DEVICE_ID(a)  (a & 0xFFFF)
#define GET_CODEC_GENERIC_ID(a) (a | 0xFFFF)

// Controller name strings.
typedef struct {
    UINT32 Id;
    CHAR16 *Name;
} HDA_CONTROLLER_LIST_ENTRY;
extern HDA_CONTROLLER_LIST_ENTRY gHdaControllerList[];

// Codec name strings.
typedef struct {
    UINT32 Id;
    UINT16 Rev;
    CHAR16 *Name;
} HDA_CODEC_LIST_ENTRY;
extern HDA_CODEC_LIST_ENTRY gHdaCodecList[];

#endif
