// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_THREAD_TYPE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_THREAD_TYPE_H_

#include "third_party/blink/public/platform/web_common.h"

namespace blink {

enum class WebThreadType {
  kMainThread = 0,
  kUnspecifiedWorkerThread = 1,
  kCompositorThread = 2,
  kDedicatedWorkerThread = 3,
  kSharedWorkerThread = 4,
  kAnimationAndPaintWorkletThread = 5,
  kServiceWorkerThread = 6,
  kAudioWorkletThread = 7,
  kFileThread = 8,
  kDatabaseThread = 9,
  // 10 was kWebAudioThread, which was deleted (crbug.com/965093)
  // 11 was kScriptStreamerThread, which was deleted
  kOfflineAudioRenderThread = 12,
  kReverbConvolutionBackgroundThread = 13,
  kHRTFDatabaseLoaderThread = 14,
  kTestThread = 15,
  kAudioEncoderThread = 16,

  kCount = 17
};

BLINK_PLATFORM_EXPORT const char* GetNameForThreadType(WebThreadType);

}  // namespace blink

#endif  // ThreadType_h
