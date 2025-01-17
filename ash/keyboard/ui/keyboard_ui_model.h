// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_KEYBOARD_UI_KEYBOARD_UI_MODEL_H_
#define ASH_KEYBOARD_UI_KEYBOARD_UI_MODEL_H_

#include <string>

#include "ash/keyboard/ui/keyboard_export.h"
#include "ash/public/cpp/keyboard/keyboard_config.h"
#include "base/macros.h"

namespace keyboard {

// Represents the current state of the keyboard UI.
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class KeyboardUIState {
  kUnknown = 0,
  // Keyboard has never been shown.
  kInitial = 1,
  // Waiting for an extension to be loaded. Will move to HIDDEN if this is
  // loading pre-emptively, otherwise will move to SHOWN.
  kLoading = 2,
  // kShowing = 3,  // no longer used
  // Keyboard is shown.
  kShown = 4,
  // Keyboard is still shown, but will move to HIDDEN in a short period, or if
  // an input element gets focused again, will move to SHOWN.
  kWillHide = 5,
  // kHiding = 6,  // no longer used
  // Keyboard is hidden, but has shown at least once.
  kHidden = 7,
  kMaxValue = kHidden
};

// Returns the string representation of a keyboard UI state.
std::string StateToStr(KeyboardUIState state);

// Model for the virtual keyboard UI.
class KEYBOARD_EXPORT KeyboardUIModel {
 public:
  KeyboardUIModel();

  // Get the current state of the keyboard UI.
  KeyboardUIState state() const { return state_; }

  // Changes the current state to another. Only accepts valid state transitions.
  void ChangeState(KeyboardUIState new_state);

 private:
  // Current state of the keyboard UI.
  KeyboardUIState state_ = KeyboardUIState::kInitial;

  DISALLOW_COPY_AND_ASSIGN(KeyboardUIModel);
};

}  // namespace keyboard

#endif  // ASH_KEYBOARD_UI_KEYBOARD_UI_MODEL_H_
