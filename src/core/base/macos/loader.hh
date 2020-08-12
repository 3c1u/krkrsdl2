/**
 * @file        loader.hh
 * @author      Hikaru Terazono <3c1u@vulpesgames.tokyo>
 * @brief       Handles a native plugin on macOS.
 * @date        2020-08-12
 *
 * @copyright   Copyright (c) 2020 Hikaru Terazono (3c1u). All rights reserved.
 *
 */

#pragma once

#if defined(__APPLE__)

#include "tjsCommHead.h"

#include "MsgIntf.h"

#include <CoreFoundation/CoreFoundation.h>

#include <dlfcn.h>
#include <mach-o/getsect.h>

enum {
  kTVPSharedLibraryObjCStatusUnavailable    = -1,
  kTVPSharedLibraryObjCStatusObjCNotPresent = 0,
  kTVPSharedLibraryObjCStatusObjCPresent    = 1,
};

class tTVPSharedLibrary_Mac {
public:
  tTVPSharedLibrary_Mac();
  ~tTVPSharedLibrary_Mac();

  /**
   * @brief      Loads a Mach-O format .dylib as a plugin.
   *
   * This function loads a .dylib file and returns the loader.
   * Returns `nullptr` when failed.
   *
   * @param path Path to the .dylib file.
   * @return     Loader.
   */
  void Open(const ttstr &path);

  /**
   * @brief       Loads the pointer to a symbol refers to `name`.
   *
   * This function finds a symbol whose name matches to `name`,
   * and return the pointer of it. Returns `nullptr` if missing.
   *
   * @param  name Name of symbol.
   * @return      The pointer to a symbol.
   */
  void *GetFunction(const ttstr &name);

  /**
   * @brief       Unloads the plugin.
   *
   * This function tries to unload the plugin.
   */
  void Close();

  /**
   * @brief       Returns if the plugin is unloadable.
   */
  bool CanUnload();

private:
  int32_t     m_objCStaus;
  void *      m_dlHandle;
  CFBundleRef m_bundle;
};

#endif
