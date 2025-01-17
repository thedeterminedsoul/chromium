// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/web_content_area/java_script_dialogs/java_script_confirmation_overlay_mediator.h"

#include "base/strings/sys_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#include "components/url_formatter/elide_url.h"
#import "ios/chrome/browser/overlays/public/overlay_request.h"
#import "ios/chrome/browser/overlays/public/overlay_response.h"
#import "ios/chrome/browser/overlays/public/web_content_area/java_script_confirmation_overlay.h"
#import "ios/chrome/browser/ui/alert_view_controller/alert_action.h"
#import "ios/chrome/browser/ui/alert_view_controller/test/fake_alert_consumer.h"
#import "ios/chrome/browser/ui/overlays/web_content_area/java_script_dialogs/java_script_dialog_overlay_mediator.h"
#import "ios/chrome/browser/ui/overlays/web_content_area/java_script_dialogs/test/java_script_dialog_overlay_mediator_test.h"
#include "ios/chrome/grit/ios_strings.h"
#include "testing/gtest_mac.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class JavaScriptConfirmationOverlayMediatorTest
    : public JavaScriptDialogOverlayMediatorTest {
 public:
  JavaScriptConfirmationOverlayMediatorTest()
      : url_("https://chromium.test"), message_("Message") {}

  // Creates a mediator and sets it for testing.
  void CreateMediator() {
    request_ = OverlayRequest::CreateWithConfig<
        JavaScriptConfirmationOverlayRequestConfig>(
        url_, /*is_main_frame=*/true, message_);
    SetMediator([[JavaScriptConfirmationOverlayMediator alloc]
        initWithRequest:request_.get()]);
  }

  const GURL& url() const { return url_; }
  const std::string& message() const { return message_; }
  const OverlayRequest* request() const { return request_.get(); }

 private:
  const GURL url_;
  const std::string message_;
  std::unique_ptr<OverlayRequest> request_;
};

// Tests that the consumer values are set correctly for confirmations.
TEST_F(JavaScriptConfirmationOverlayMediatorTest, ConfirmationSetup) {
  CreateMediator();

  // Verify the consumer values.
  EXPECT_NSEQ(base::SysUTF8ToNSString(message()), consumer().message);
  EXPECT_EQ(0U, consumer().textFieldConfigurations.count);
  ASSERT_EQ(2U, consumer().actions.count);
  EXPECT_EQ(UIAlertActionStyleDefault, consumer().actions[0].style);
  EXPECT_NSEQ(l10n_util::GetNSString(IDS_OK), consumer().actions[0].title);
  EXPECT_EQ(UIAlertActionStyleCancel, consumer().actions[1].style);
  EXPECT_NSEQ(l10n_util::GetNSString(IDS_CANCEL), consumer().actions[1].title);
}

// Tests that the correct response is provided for the confirm action.
TEST_F(JavaScriptConfirmationOverlayMediatorTest, ConfirmResponse) {
  CreateMediator();
  ASSERT_EQ(2U, consumer().actions.count);
  ASSERT_FALSE(!!request()->response());

  // Execute the confirm action and verify the response.
  AlertAction* confirm_action = consumer().actions[0];
  confirm_action.handler(confirm_action);
  OverlayResponse* confirm_response = request()->response();
  ASSERT_TRUE(!!confirm_response);
  JavaScriptConfirmationOverlayResponseInfo* confirm_response_info =
      confirm_response->GetInfo<JavaScriptConfirmationOverlayResponseInfo>();
  ASSERT_TRUE(confirm_response_info);
  EXPECT_TRUE(confirm_response_info->dialog_confirmed());
}

// Tests that the correct response is provided for the cancel action.
TEST_F(JavaScriptConfirmationOverlayMediatorTest, CancelResponse) {
  CreateMediator();
  ASSERT_EQ(2U, consumer().actions.count);
  ASSERT_FALSE(!!request()->response());

  // Execute the cancel action and verify the response.
  AlertAction* cancel_action = consumer().actions[1];
  cancel_action.handler(cancel_action);
  OverlayResponse* cancel_response = request()->response();
  ASSERT_TRUE(!!cancel_response);
  JavaScriptConfirmationOverlayResponseInfo* cancel_response_info =
      cancel_response->GetInfo<JavaScriptConfirmationOverlayResponseInfo>();
  ASSERT_TRUE(cancel_response_info);
  EXPECT_FALSE(cancel_response_info->dialog_confirmed());
}
