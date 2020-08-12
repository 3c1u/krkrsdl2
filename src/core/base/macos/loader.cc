/**
 * @file        loader.cc
 * @author      Hikaru Terazono <3c1u@vulpesgames.tokyo>
 * @brief       Handles a native plugin on macOS.
 * @date        2020-08-12
 *
 * @copyright   Copyright (c) 2020 Hikaru Terazono (3c1u). All rights reserved.
 *
 */

#include "loader.hh"

#if defined(__APPLE__)

tTVPSharedLibrary_Mac::tTVPSharedLibrary_Mac()
    : m_objCStaus(kTVPSharedLibraryObjCStatusUnavailable), m_dlHandle(nullptr),
      m_bundle(nullptr) {}

tTVPSharedLibrary_Mac::~tTVPSharedLibrary_Mac() {
  if (!CanUnload()) {
    return; // this leak is intentional
  }

  if (m_dlHandle) {
    dlclose(m_dlHandle);
  }

  if (m_bundle) {
    CFRelease(m_bundle);
  }
}

void tTVPSharedLibrary_Mac::Open(const ttstr &path) {
  auto allocator = CFAllocatorGetDefault();

  auto pathStr = CFStringCreateWithCString(
      allocator, reinterpret_cast<char const *>(path.c_str()),
      kCFStringEncodingUTF16LE);

  auto pathUrl   = CFURLCreateWithString(allocator, pathStr, nullptr);
  auto extension = CFURLCopyPathExtension(pathUrl);

  // try dlopen loader if the extension is .dylib or .dll
  if (CFStringCompare(extension, CFSTR("dylib"), kCFCompareCaseInsensitive) ==
          kCFCompareEqualTo ||
      CFStringCompare(extension, CFSTR("dll"), kCFCompareCaseInsensitive) ==
          kCFCompareEqualTo) {

    // converts the path into dlopen()-compliant string (UTF-8).
    auto pathUtf8 = CFStringGetCStringPtr(pathStr, kCFStringEncodingUTF8);
    if (!pathUtf8) {
      TVPThrowExceptionMessage(TVPIllegalCharacterConversionUTF16toUTF8);
    }

    // Try opening with dlopen()
    auto hdl = dlopen(pathUtf8, RTLD_LAZY);

    if (hdl) {
      CFRelease(extension);
      CFRelease(pathUrl);
      CFRelease(pathStr);

      this->m_dlHandle = hdl;
      return;
    }
  }

  CFRelease(extension);

  // try CFBundle loader.
  auto bundle = CFBundleCreate(allocator, pathUrl);

  CFRelease(pathUrl);

  if (!bundle) {
    return;
  }

  this->m_bundle = bundle;
}

void *tTVPSharedLibrary_Mac::GetFunction(const ttstr &name) {
  auto allocator = CFAllocatorGetDefault();

  auto symbolName = CFStringCreateWithCString(
      allocator, reinterpret_cast<char const *>(name.c_str()),
      kCFStringEncodingUTF16LE);

  void *sym = nullptr;

  if (m_dlHandle) {
    auto symNameUtf8 = CFStringGetCStringPtr(symbolName, kCFStringEncodingUTF8);

    if (!symNameUtf8) {
      CFRelease(symbolName);
      TVPThrowExceptionMessage(TVPIllegalCharacterConversionUTF16toUTF8);
    }

    sym = dlsym(m_dlHandle, symNameUtf8);
  }

  if (m_bundle) {
    sym = CFBundleGetFunctionPointerForName(m_bundle, symbolName);
  }

  CFRelease(symbolName);

  // Unloading a library with ObjC symbols will fuck up the ObjC runtime caches.
  // https://chromium.googlesource.com/chromium/src/+/master/base/native_library_mac.mm
  if (sym && m_objCStaus == kTVPSharedLibraryObjCStatusUnavailable) {
    Dl_info info;
    if (dladdr(sym, &info)) {
      const section_64 *section = getsectbynamefromheader_64(
          reinterpret_cast<const struct mach_header_64 *>(info.dli_fbase),
          SEG_DATA, "__objc_imageinfo");

      m_objCStaus = section ? kTVPSharedLibraryObjCStatusObjCPresent
                            : kTVPSharedLibraryObjCStatusObjCNotPresent;
    }
  }

  return sym;
}

void tTVPSharedLibrary_Mac::Close() {
  if (!CanUnload()) {
    TVPThrowExceptionMessage(
        TJS_W("tTVPSharedLibrary_Mac: failed to unload a plugin"));
  }

  delete this;
}

bool tTVPSharedLibrary_Mac::CanUnload() {
  return m_objCStaus == kTVPSharedLibraryObjCStatusObjCNotPresent;
}

#endif
