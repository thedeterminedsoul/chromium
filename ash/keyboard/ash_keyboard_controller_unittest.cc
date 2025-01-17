// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/keyboard/ash_keyboard_controller.h"

#include <memory>
#include <vector>

#include "ash/keyboard/ui/container_behavior.h"
#include "ash/keyboard/ui/keyboard_controller.h"
#include "ash/keyboard/ui/test/keyboard_test_util.h"
#include "ash/public/cpp/keyboard/keyboard_controller.h"
#include "ash/public/cpp/test/test_keyboard_controller_observer.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "base/bind.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/window.h"
#include "ui/display/manager/display_manager.h"

using keyboard::KeyboardConfig;
using keyboard::KeyboardEnableFlag;

namespace ash {

namespace {

// TODO(shend): Remove this as AshKeyboardController no longer uses mojo.
class TestClient {
 public:
  explicit TestClient()
      : keyboard_controller_(Shell::Get()->ash_keyboard_controller()) {}

  ~TestClient() = default;

  bool IsKeyboardEnabled() {
    keyboard_controller_->IsKeyboardEnabled(base::BindOnce(
        &TestClient::OnIsKeyboardEnabled, base::Unretained(this)));
    return is_enabled_;
  }

  void GetKeyboardConfig() {
    keyboard_controller_->GetKeyboardConfig(base::BindOnce(
        &TestClient::OnGetKeyboardConfig, base::Unretained(this)));
  }

  void SetKeyboardConfig(KeyboardConfig config) {
    keyboard_controller_->SetKeyboardConfig(std::move(config));
  }

  void SetEnableFlag(KeyboardEnableFlag flag) {
    keyboard_controller_->SetEnableFlag(flag);
  }

  void ClearEnableFlag(KeyboardEnableFlag flag) {
    keyboard_controller_->ClearEnableFlag(flag);
  }

  std::vector<keyboard::KeyboardEnableFlag> GetEnableFlags() {
    std::vector<keyboard::KeyboardEnableFlag> enable_flags;
    base::RunLoop run_loop;
    keyboard_controller_->GetEnableFlags(base::BindOnce(
        [](std::vector<keyboard::KeyboardEnableFlag>* enable_flags,
           base::OnceClosure callback,
           const std::vector<keyboard::KeyboardEnableFlag>& flags) {
          *enable_flags = flags;
          std::move(callback).Run();
        },
        &enable_flags, run_loop.QuitClosure()));
    run_loop.Run();
    return enable_flags;
  }

  void RebuildKeyboardIfEnabled() {
    keyboard_controller_->RebuildKeyboardIfEnabled();
  }

  bool IsKeyboardVisible() {
    keyboard_controller_->IsKeyboardVisible(base::BindOnce(
        &TestClient::OnIsKeyboardVisible, base::Unretained(this)));
    return is_visible_;
  }

  void ShowKeyboard() { keyboard_controller_->ShowKeyboard(); }

  void HideKeyboard() { keyboard_controller_->HideKeyboard(HideReason::kUser); }

  bool SetContainerType(keyboard::ContainerType container_type,
                        const base::Optional<gfx::Rect>& target_bounds) {
    bool result;
    base::RunLoop run_loop;
    keyboard_controller_->SetContainerType(
        container_type, target_bounds,
        base::BindOnce(
            [](bool* result_ptr, base::OnceClosure callback, bool result) {
              *result_ptr = result;
              std::move(callback).Run();
            },
            &result, run_loop.QuitClosure()));
    run_loop.Run();
    return result;
  }

  void SetKeyboardLocked(bool locked) {
    keyboard_controller_->SetKeyboardLocked(locked);
  }

  void SetOccludedBounds(const std::vector<gfx::Rect>& bounds) {
    keyboard_controller_->SetOccludedBounds(bounds);
  }

  void SetHitTestBounds(const std::vector<gfx::Rect>& bounds) {
    keyboard_controller_->SetHitTestBounds(bounds);
  }

  void SetDraggableArea(const gfx::Rect& bounds) {
    keyboard_controller_->SetDraggableArea(bounds);
  }

  int got_keyboard_config_count() const { return got_keyboard_config_count_; }
  const KeyboardConfig& keyboard_config() const { return keyboard_config_; }

 private:
  void OnIsKeyboardEnabled(bool enabled) { is_enabled_ = enabled; }
  void OnIsKeyboardVisible(bool visible) { is_visible_ = visible; }

  void OnGetKeyboardConfig(const KeyboardConfig& config) {
    ++got_keyboard_config_count_;
    keyboard_config_ = config;
  }

  KeyboardController* keyboard_controller_ = nullptr;
  bool is_enabled_ = false;
  bool is_visible_ = false;
  int got_keyboard_config_count_ = 0;
  KeyboardConfig keyboard_config_;
};

class TestContainerBehavior : public keyboard::ContainerBehavior {
 public:
  TestContainerBehavior() : keyboard::ContainerBehavior(nullptr) {}
  ~TestContainerBehavior() override = default;

  // keyboard::ContainerBehavior
  void DoShowingAnimation(
      aura::Window* window,
      ui::ScopedLayerAnimationSettings* animation_settings) override {}

  void DoHidingAnimation(
      aura::Window* window,
      wm::ScopedHidingAnimationSettings* animation_settings) override {}

  void InitializeShowAnimationStartingState(aura::Window* container) override {}

  gfx::Rect AdjustSetBoundsRequest(
      const gfx::Rect& display_bounds,
      const gfx::Rect& requested_bounds_in_screen_coords) override {
    return gfx::Rect();
  }

  void SetCanonicalBounds(aura::Window* container,
                          const gfx::Rect& display_bounds) override {}

  bool IsOverscrollAllowed() const override { return true; }

  void SavePosition(const gfx::Rect& keyboard_bounds_in_screen,
                    const gfx::Size& screen_size) override {}

  bool HandlePointerEvent(const ui::LocatedEvent& event,
                          const display::Display& current_display) override {
    return false;
  }

  keyboard::ContainerType GetType() const override { return type_; }

  bool TextBlurHidesKeyboard() const override { return false; }

  void SetOccludedBounds(const gfx::Rect& bounds) override {
    occluded_bounds_ = bounds;
  }

  gfx::Rect GetOccludedBounds(
      const gfx::Rect& visual_bounds_in_window) const override {
    return occluded_bounds_;
  }

  bool OccludedBoundsAffectWorkspaceLayout() const override { return false; }

  void SetDraggableArea(const gfx::Rect& rect) override {
    draggable_area_ = rect;
  }

  const gfx::Rect& occluded_bounds() const { return occluded_bounds_; }
  const gfx::Rect& draggable_area() const { return draggable_area_; }

 private:
  keyboard::ContainerType type_ = keyboard::ContainerType::kFullWidth;
  gfx::Rect occluded_bounds_;
  gfx::Rect draggable_area_;
};

class AshKeyboardControllerTest : public AshTestBase {
 public:
  AshKeyboardControllerTest() = default;
  ~AshKeyboardControllerTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();

    test_client_ = std::make_unique<TestClient>();

    // Set the initial observer config to the client (default) config.
    test_observer()->set_config(test_client()->keyboard_config());
  }

  void TearDown() override {
    test_client_.reset();
    AshTestBase::TearDown();
  }

  keyboard::KeyboardController* keyboard_controller() {
    return Shell::Get()->ash_keyboard_controller()->keyboard_controller();
  }
  TestClient* test_client() { return test_client_.get(); }
  TestKeyboardControllerObserver* test_observer() {
    return ash_test_helper()->test_keyboard_controller_observer();
  }

 private:
  std::unique_ptr<TestClient> test_client_;

  DISALLOW_COPY_AND_ASSIGN(AshKeyboardControllerTest);
};

}  // namespace

TEST_F(AshKeyboardControllerTest, GetKeyboardConfig) {
  test_client()->GetKeyboardConfig();
  EXPECT_EQ(1, test_client()->got_keyboard_config_count());
}

TEST_F(AshKeyboardControllerTest, SetKeyboardConfig) {
  // Enable the keyboard so that config changes trigger observer events.
  test_client()->SetEnableFlag(KeyboardEnableFlag::kExtensionEnabled);

  test_client()->GetKeyboardConfig();
  EXPECT_EQ(1, test_client()->got_keyboard_config_count());
  KeyboardConfig config = test_client()->keyboard_config();
  // Set the observer config to the client (default) config.
  test_observer()->set_config(config);

  // Change the keyboard config.
  bool old_auto_complete = config.auto_complete;
  config.auto_complete = !config.auto_complete;
  test_client()->SetKeyboardConfig(std::move(config));

  // Test that the config changes.
  test_client()->GetKeyboardConfig();
  EXPECT_NE(old_auto_complete, test_client()->keyboard_config().auto_complete);

  // Test that the test observer received the change.
  EXPECT_NE(old_auto_complete, test_observer()->config().auto_complete);
}

TEST_F(AshKeyboardControllerTest, EnableFlags) {
  EXPECT_FALSE(test_client()->IsKeyboardEnabled());
  // Enable the keyboard.
  test_client()->SetEnableFlag(KeyboardEnableFlag::kExtensionEnabled);
  std::vector<keyboard::KeyboardEnableFlag> enable_flags =
      test_client()->GetEnableFlags();
  EXPECT_TRUE(
      base::Contains(enable_flags, KeyboardEnableFlag::kExtensionEnabled));
  EXPECT_EQ(enable_flags, test_observer()->enable_flags());
  EXPECT_TRUE(test_client()->IsKeyboardEnabled());

  // Set the enable override to disable the keyboard.
  test_client()->SetEnableFlag(KeyboardEnableFlag::kPolicyDisabled);
  enable_flags = test_client()->GetEnableFlags();
  EXPECT_TRUE(
      base::Contains(enable_flags, KeyboardEnableFlag::kExtensionEnabled));
  EXPECT_TRUE(
      base::Contains(enable_flags, KeyboardEnableFlag::kPolicyDisabled));
  EXPECT_EQ(enable_flags, test_observer()->enable_flags());
  EXPECT_FALSE(test_client()->IsKeyboardEnabled());

  // Clear the enable override; should enable the keyboard.
  test_client()->ClearEnableFlag(KeyboardEnableFlag::kPolicyDisabled);
  enable_flags = test_client()->GetEnableFlags();
  EXPECT_TRUE(
      base::Contains(enable_flags, KeyboardEnableFlag::kExtensionEnabled));
  EXPECT_FALSE(
      base::Contains(enable_flags, KeyboardEnableFlag::kPolicyDisabled));
  EXPECT_EQ(enable_flags, test_observer()->enable_flags());
  EXPECT_TRUE(test_client()->IsKeyboardEnabled());
}

TEST_F(AshKeyboardControllerTest, RebuildKeyboardIfEnabled) {
  EXPECT_EQ(0, test_observer()->destroyed_count());

  // Enable the keyboard.
  test_client()->SetEnableFlag(KeyboardEnableFlag::kExtensionEnabled);
  EXPECT_EQ(0, test_observer()->destroyed_count());

  // Enable the keyboard again; this should not reload the keyboard.
  test_client()->SetEnableFlag(KeyboardEnableFlag::kExtensionEnabled);
  EXPECT_EQ(0, test_observer()->destroyed_count());

  // Rebuild the keyboard. This should destroy the previous keyboard window.
  test_client()->RebuildKeyboardIfEnabled();
  EXPECT_EQ(1, test_observer()->destroyed_count());

  // Disable the keyboard. The keyboard window should be destroyed.
  test_client()->ClearEnableFlag(KeyboardEnableFlag::kExtensionEnabled);
  EXPECT_EQ(2, test_observer()->destroyed_count());
}

TEST_F(AshKeyboardControllerTest, ShowAndHideKeyboard) {
  // Enable the keyboard. This will create the keyboard window but not show it.
  test_client()->SetEnableFlag(KeyboardEnableFlag::kExtensionEnabled);

  ASSERT_TRUE(keyboard_controller()->GetKeyboardWindow());
  EXPECT_FALSE(keyboard_controller()->GetKeyboardWindow()->IsVisible());

  // The keyboard needs to be in a loaded state before being shown.
  ASSERT_TRUE(keyboard::test::WaitUntilLoaded());

  test_client()->ShowKeyboard();
  EXPECT_TRUE(keyboard_controller()->GetKeyboardWindow()->IsVisible());

  test_client()->HideKeyboard();
  EXPECT_FALSE(keyboard_controller()->GetKeyboardWindow()->IsVisible());

  // TODO(stevenjb): Also use TestKeyboardControllerObserver and
  // IsKeyboardVisible to test visibility changes. https://crbug.com/849995.
}

TEST_F(AshKeyboardControllerTest, SetContainerType) {
  // Enable the keyboard.
  test_client()->SetEnableFlag(KeyboardEnableFlag::kExtensionEnabled);
  const auto default_behavior = keyboard::ContainerType::kFullWidth;
  EXPECT_EQ(default_behavior, keyboard_controller()->GetActiveContainerType());

  gfx::Rect target_bounds(0, 0, 10, 10);
  // Set the container type to kFloating.
  EXPECT_TRUE(test_client()->SetContainerType(
      keyboard::ContainerType::kFloating, target_bounds));
  EXPECT_EQ(keyboard::ContainerType::kFloating,
            keyboard_controller()->GetActiveContainerType());
  // Ensure that the window size is correct (position is determined by Ash).
  EXPECT_EQ(
      target_bounds.size(),
      keyboard_controller()->GetKeyboardWindow()->GetTargetBounds().size());

  // Setting the container type to the current type should fail.
  EXPECT_FALSE(test_client()->SetContainerType(
      keyboard::ContainerType::kFloating, base::nullopt));
  EXPECT_EQ(keyboard::ContainerType::kFloating,
            keyboard_controller()->GetActiveContainerType());
}

TEST_F(AshKeyboardControllerTest, SetKeyboardLocked) {
  ASSERT_FALSE(keyboard_controller()->keyboard_locked());
  test_client()->SetKeyboardLocked(true);
  EXPECT_TRUE(keyboard_controller()->keyboard_locked());
  test_client()->SetKeyboardLocked(false);
  EXPECT_FALSE(keyboard_controller()->keyboard_locked());
}

TEST_F(AshKeyboardControllerTest, SetOccludedBounds) {
  // Enable the keyboard.
  test_client()->SetEnableFlag(KeyboardEnableFlag::kExtensionEnabled);

  // Override the container behavior.
  auto scoped_behavior = std::make_unique<TestContainerBehavior>();
  TestContainerBehavior* behavior = scoped_behavior.get();
  keyboard_controller()->set_container_behavior_for_test(
      std::move(scoped_behavior));

  gfx::Rect bounds(10, 20, 30, 40);
  test_client()->SetOccludedBounds({bounds});
  EXPECT_EQ(bounds, behavior->occluded_bounds());
}

TEST_F(AshKeyboardControllerTest, SetHitTestBounds) {
  // Enable the keyboard.
  test_client()->SetEnableFlag(KeyboardEnableFlag::kExtensionEnabled);
  ASSERT_FALSE(keyboard_controller()->GetKeyboardWindow()->targeter());

  // Setting the hit test bounds should set a WindowTargeter.
  test_client()->SetHitTestBounds({gfx::Rect(10, 20, 30, 40)});
  ASSERT_TRUE(keyboard_controller()->GetKeyboardWindow()->targeter());
}

TEST_F(AshKeyboardControllerTest, SetDraggableArea) {
  // Enable the keyboard.
  test_client()->SetEnableFlag(KeyboardEnableFlag::kExtensionEnabled);

  // Override the container behavior.
  auto scoped_behavior = std::make_unique<TestContainerBehavior>();
  TestContainerBehavior* behavior = scoped_behavior.get();
  keyboard_controller()->set_container_behavior_for_test(
      std::move(scoped_behavior));

  gfx::Rect bounds(10, 20, 30, 40);
  test_client()->SetDraggableArea(bounds);
  EXPECT_EQ(bounds, behavior->draggable_area());
}

TEST_F(AshKeyboardControllerTest, ChangingSessionRebuildsKeyboard) {
  // Enable the keyboard.
  test_client()->SetEnableFlag(KeyboardEnableFlag::kExtensionEnabled);

  // LOGGED_IN_NOT_ACTIVE session state needs to rebuild keyboard for supervised
  // user profile.
  Shell::Get()->ash_keyboard_controller()->OnSessionStateChanged(
      session_manager::SessionState::LOGGED_IN_NOT_ACTIVE);
  EXPECT_EQ(1, test_observer()->destroyed_count());

  // ACTIVE session state also needs to rebuild keyboard for guest user profile.
  Shell::Get()->ash_keyboard_controller()->OnSessionStateChanged(
      session_manager::SessionState::ACTIVE);
  EXPECT_EQ(2, test_observer()->destroyed_count());
}

TEST_F(AshKeyboardControllerTest, VisualBoundsInMultipleDisplays) {
  UpdateDisplay("800x600,1024x768");

  test_client()->SetEnableFlag(KeyboardEnableFlag::kExtensionEnabled);

  // Show the keyboard in the second display.
  keyboard_controller()->ShowKeyboardInDisplay(
      Shell::Get()->display_manager()->GetSecondaryDisplay());
  ASSERT_TRUE(keyboard::WaitUntilShown());

  gfx::Rect root_bounds = keyboard_controller()->visual_bounds_in_root();
  EXPECT_EQ(0, root_bounds.x());

  gfx::Rect screen_bounds = keyboard_controller()->GetVisualBoundsInScreen();
  EXPECT_EQ(800, screen_bounds.x());
}

TEST_F(AshKeyboardControllerTest, OccludedBoundsInMultipleDisplays) {
  UpdateDisplay("800x600,1024x768");

  test_client()->SetEnableFlag(KeyboardEnableFlag::kExtensionEnabled);

  // Show the keyboard in the second display.
  keyboard_controller()->ShowKeyboardInDisplay(
      Shell::Get()->display_manager()->GetSecondaryDisplay());
  ASSERT_TRUE(keyboard::WaitUntilShown());

  gfx::Rect screen_bounds =
      keyboard_controller()->GetWorkspaceOccludedBoundsInScreen();
  EXPECT_EQ(800, screen_bounds.x());
}

}  // namespace ash
