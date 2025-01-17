// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/keyboard/chrome_keyboard_ui.h"

#include <memory>

#include "base/bind_helpers.h"
#include "chrome/browser/ui/ash/keyboard/chrome_keyboard_controller_client.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/test_chrome_web_ui_controller_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "ui/base/ui_base_features.h"
#include "url/gurl.h"

// NOTE: ChromeKeyboardUITest is not used with the Window Service.

namespace {

class ChromeKeyboardUITest : public ChromeRenderViewHostTestHarness {
 public:
  ChromeKeyboardUITest() = default;
  ~ChromeKeyboardUITest() override = default;

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    if (::features::IsUsingWindowService())
      return;
    chrome_keyboard_controller_client_ =
        ChromeKeyboardControllerClient::CreateForTest();
    chrome_keyboard_ui_ = std::make_unique<ChromeKeyboardUI>(profile());
  }

  void TearDown() override {
    chrome_keyboard_ui_.reset();
    chrome_keyboard_controller_client_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

 protected:
  std::unique_ptr<ChromeKeyboardControllerClient>
      chrome_keyboard_controller_client_;
  std::unique_ptr<ChromeKeyboardUI> chrome_keyboard_ui_;
};

}  // namespace

// Ensure ChromeKeyboardContentsDelegate is successfully constructed and has
// a valid aura::Window after calling LoadKeyboardWindow().
TEST_F(ChromeKeyboardUITest, ChromeKeyboardContentsDelegate) {
  if (::features::IsUsingWindowService())
    return;
  aura::Window* window =
      chrome_keyboard_ui_->LoadKeyboardWindow(base::DoNothing());
  EXPECT_TRUE(window);
}
