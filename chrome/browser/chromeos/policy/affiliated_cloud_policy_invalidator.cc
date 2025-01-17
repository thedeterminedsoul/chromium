// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/affiliated_cloud_policy_invalidator.h"

#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "chrome/browser/policy/cloud/cloud_policy_invalidator.h"

namespace policy {

AffiliatedCloudPolicyInvalidator::AffiliatedCloudPolicyInvalidator(
    enterprise_management::DeviceRegisterRequest::Type type,
    CloudPolicyCore* core,
    AffiliatedInvalidationServiceProvider* invalidation_service_provider,
    bool is_fcm_enabled)
    : is_fcm_enabled_(is_fcm_enabled),
      type_(type),
      core_(core),
      invalidation_service_provider_(invalidation_service_provider),
      highest_handled_invalidation_version_(0) {
  invalidation_service_provider_->RegisterConsumer(this);
}

AffiliatedCloudPolicyInvalidator::~AffiliatedCloudPolicyInvalidator() {
  DestroyInvalidator();
  invalidation_service_provider_->UnregisterConsumer(this);
}

void AffiliatedCloudPolicyInvalidator::OnInvalidationServiceSet(
    invalidation::InvalidationService* invalidation_service) {
  DestroyInvalidator();
  if (invalidation_service)
    CreateInvalidator(invalidation_service);
}

CloudPolicyInvalidator*
AffiliatedCloudPolicyInvalidator::GetInvalidatorForTest() const {
  return invalidator_.get();
}

void AffiliatedCloudPolicyInvalidator::CreateInvalidator(
    invalidation::InvalidationService* invalidation_service) {
  DCHECK(!invalidator_);
  invalidator_.reset(new CloudPolicyInvalidator(
      type_, core_, base::ThreadTaskRunnerHandle::Get(),
      base::DefaultClock::GetInstance(), highest_handled_invalidation_version_,
      is_fcm_enabled_));
  invalidator_->Initialize(invalidation_service);
}

void AffiliatedCloudPolicyInvalidator::DestroyInvalidator() {
  if (!invalidator_)
    return;

  highest_handled_invalidation_version_ =
      invalidator_->highest_handled_invalidation_version();
  invalidator_->Shutdown();
  invalidator_.reset();
}

}  // namespace policy
