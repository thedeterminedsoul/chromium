// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/native_file_system/native_file_system_handle.h"

#include "third_party/blink/public/mojom/native_file_system/native_file_system_error.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/fileapi/file_error.h"
#include "third_party/blink/renderer/modules/native_file_system/file_system_handle_permission_descriptor.h"
#include "third_party/blink/renderer/modules/native_file_system/native_file_system_directory_handle.h"
#include "third_party/blink/renderer/modules/native_file_system/native_file_system_file_handle.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {
using mojom::blink::NativeFileSystemEntryPtr;
using mojom::blink::NativeFileSystemErrorPtr;

NativeFileSystemHandle::NativeFileSystemHandle(const String& name)
    : name_(name) {}

// static
NativeFileSystemHandle* NativeFileSystemHandle::CreateFromMojoEntry(
    mojom::blink::NativeFileSystemEntryPtr e) {
  if (e->entry_handle->is_file()) {
    return MakeGarbageCollected<NativeFileSystemFileHandle>(
        e->name, mojom::blink::NativeFileSystemFileHandlePtr(
                     std::move(e->entry_handle->get_file())));
  }
  return MakeGarbageCollected<NativeFileSystemDirectoryHandle>(
      e->name, mojom::blink::NativeFileSystemDirectoryHandlePtr(
                   std::move(e->entry_handle->get_directory())));
}

ScriptPromise NativeFileSystemHandle::remove(ScriptState* script_state) {
  auto* resolver = MakeGarbageCollected<ScriptPromiseResolver>(script_state);
  ScriptPromise result = resolver->Promise();

  RemoveImpl(WTF::Bind(
      [](ScriptPromiseResolver* resolver, NativeFileSystemErrorPtr result) {
        if (result->error_code == base::File::FILE_OK) {
          resolver->Resolve();
        } else {
          resolver->Reject(file_error::CreateDOMException(result->error_code));
        }
      },
      WrapPersistent(resolver)));

  return result;
}

namespace {
String MojoPermissionStatusToString(mojom::blink::PermissionStatus status) {
  switch (status) {
    case mojom::blink::PermissionStatus::GRANTED:
      return "granted";
    case mojom::blink::PermissionStatus::DENIED:
      return "denied";
    case mojom::blink::PermissionStatus::ASK:
      return "prompt";
  }
  NOTREACHED();
  return "denied";
}
}  // namespace

ScriptPromise NativeFileSystemHandle::queryPermission(
    ScriptState* script_state,
    const FileSystemHandlePermissionDescriptor* descriptor) {
  auto* resolver = MakeGarbageCollected<ScriptPromiseResolver>(script_state);
  ScriptPromise result = resolver->Promise();

  QueryPermissionImpl(
      descriptor->writable(),
      WTF::Bind(
          [](ScriptPromiseResolver* resolver,
             mojom::blink::PermissionStatus result) {
            resolver->Resolve(MojoPermissionStatusToString(result));
          },
          WrapPersistent(resolver)));

  return result;
}

ScriptPromise NativeFileSystemHandle::requestPermission(
    ScriptState* script_state,
    const FileSystemHandlePermissionDescriptor* descriptor) {
  auto* resolver = MakeGarbageCollected<ScriptPromiseResolver>(script_state);
  ScriptPromise result = resolver->Promise();

  RequestPermissionImpl(
      descriptor->writable(),
      WTF::Bind(
          [](ScriptPromiseResolver* resolver,
             mojom::blink::PermissionStatus result) {
            resolver->Resolve(MojoPermissionStatusToString(result));
          },
          WrapPersistent(resolver)));

  return result;
}

}  // namespace blink
