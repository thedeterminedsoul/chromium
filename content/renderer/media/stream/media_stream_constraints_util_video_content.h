// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_CONTENT_H_
#define CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_CONTENT_H_

#include <string>

#include "content/common/content_export.h"
#include "third_party/blink/public/common/mediastream/media_stream_request.h"
#include "third_party/blink/public/web/modules/mediastream/media_stream_constraints_util.h"

namespace blink {
class WebMediaConstraints;
}

namespace content {

CONTENT_EXPORT extern const int kMinScreenCastDimension;
CONTENT_EXPORT extern const int kMaxScreenCastDimension;
CONTENT_EXPORT extern const int kDefaultScreenCastWidth;
CONTENT_EXPORT extern const int kDefaultScreenCastHeight;

CONTENT_EXPORT extern const double kMaxScreenCastFrameRate;
CONTENT_EXPORT extern const double kDefaultScreenCastFrameRate;

// This function performs source, source-settings and track-settings selection
// for content video capture based on the given |constraints|.
blink::VideoCaptureSettings CONTENT_EXPORT
SelectSettingsVideoContentCapture(const blink::WebMediaConstraints& constraints,
                                  blink::mojom::MediaStreamType stream_type,
                                  int screen_width,
                                  int screen_height);

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_CONTENT_H_
