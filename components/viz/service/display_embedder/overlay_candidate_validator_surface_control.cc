// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/overlay_candidate_validator_surface_control.h"

#include "cc/base/math_util.h"
#include "components/viz/service/display/overlay_strategy_fullscreen.h"
#include "components/viz/service/display/overlay_strategy_single_on_top.h"
#include "components/viz/service/display/overlay_strategy_underlay.h"
#include "components/viz/service/display/renderer_utils.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/overlay_transform_utils.h"
#include "ui/gl/android/android_surface_control_compat.h"

namespace viz {
namespace {

gfx::RectF ClipFromOrigin(gfx::RectF input) {
  if (input.x() < 0.f) {
    input.set_width(input.width() + input.x());
    input.set_x(0.f);
  }

  if (input.y() < 0) {
    input.set_height(input.height() + input.y());
    input.set_y(0.f);
  }

  return input;
}

}  // namespace

OverlayCandidateValidatorSurfaceControl::
    OverlayCandidateValidatorSurfaceControl() = default;
OverlayCandidateValidatorSurfaceControl::
    ~OverlayCandidateValidatorSurfaceControl() = default;

void OverlayCandidateValidatorSurfaceControl::InitializeStrategies() {
  strategies_.push_back(std::make_unique<OverlayStrategyUnderlay>(
      this, OverlayStrategyUnderlay::OpaqueMode::AllowTransparentCandidates));
}

bool OverlayCandidateValidatorSurfaceControl::AllowCALayerOverlays() const {
  return false;
}

bool OverlayCandidateValidatorSurfaceControl::AllowDCLayerOverlays() const {
  return false;
}

bool OverlayCandidateValidatorSurfaceControl::NeedsSurfaceOccludingDamageRect()
    const {
  return true;
}

void OverlayCandidateValidatorSurfaceControl::CheckOverlaySupport(
    OverlayCandidateList* surfaces) {
  DCHECK(!surfaces->empty());

  for (auto& candidate : *surfaces) {
    if (!gl::SurfaceControl::SupportsColorSpace(candidate.color_space)) {
      DCHECK(!candidate.use_output_surface_for_resource)
          << "The main overlay must only use color space supported by the "
             "device";
      candidate.overlay_handled = false;
      return;
    }

    if (candidate.use_output_surface_for_resource) {
      AdjustOutputSurfaceOverlay(&candidate);
      continue;
    }

    if (candidate.transform != display_transform_) {
      candidate.overlay_handled = false;
      return;
    }
    candidate.transform = gfx::OVERLAY_TRANSFORM_NONE;

    gfx::RectF orig_display_rect = candidate.display_rect;
    gfx::RectF display_rect = orig_display_rect;
    if (candidate.is_clipped)
      display_rect.Intersect(gfx::RectF(candidate.clip_rect));
    // The framework doesn't support display rects positioned at a negative
    // offset.
    display_rect = ClipFromOrigin(display_rect);
    if (display_rect.IsEmpty()) {
      candidate.overlay_handled = false;
      return;
    }

    // The display rect above includes the |display_transform_| while the rects
    // sent to the platform API need to be in the logical screen space.
    const gfx::Transform display_inverse = gfx::OverlayTransformToTransform(
        gfx::InvertOverlayTransform(display_transform_), viewport_size_);
    display_inverse.TransformRect(&orig_display_rect);
    display_inverse.TransformRect(&display_rect);

    candidate.display_rect = gfx::RectF(gfx::ToEnclosingRect(display_rect));
    candidate.uv_rect = cc::MathUtil::ScaleRectProportional(
        candidate.uv_rect, orig_display_rect, candidate.display_rect);
    candidate.overlay_handled = true;
  }
}

void OverlayCandidateValidatorSurfaceControl::AdjustOutputSurfaceOverlay(
    OverlayCandidate* candidate) {
  DCHECK_EQ(candidate->transform, gfx::OVERLAY_TRANSFORM_NONE);
  DCHECK(!candidate->is_clipped);
  DCHECK(candidate->display_rect == ClipFromOrigin(candidate->display_rect));

  candidate->transform = display_transform_;
  const gfx::Transform display_inverse = gfx::OverlayTransformToTransform(
      gfx::InvertOverlayTransform(display_transform_), viewport_size_);
  display_inverse.TransformRect(&candidate->display_rect);
  candidate->display_rect =
      gfx::RectF(gfx::ToEnclosingRect(candidate->display_rect));
  candidate->overlay_handled = true;
}

void OverlayCandidateValidatorSurfaceControl::SetDisplayTransform(
    gfx::OverlayTransform transform) {
  display_transform_ = transform;
}

void OverlayCandidateValidatorSurfaceControl::SetViewportSize(
    const gfx::Size& viewport_size) {
  viewport_size_ = viewport_size;
}

}  // namespace viz
