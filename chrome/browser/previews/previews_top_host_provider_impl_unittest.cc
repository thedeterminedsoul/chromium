// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/previews/previews_top_host_provider_impl.h"

#include "base/values.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/optimization_guide/optimization_guide_prefs.h"
#include "components/prefs/pref_service.h"
#include "components/previews/content/previews_hints_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace previews {

// Class to test the TopHostProvider and the HintsFetcherTopHostBlacklist.
class PreviewsTopHostProviderImplTest : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    top_host_provider_ =
        std::make_unique<PreviewsTopHostProviderImpl>(profile());
  }

  void AddEngagedHosts(size_t num_hosts) {
    SiteEngagementService* service = SiteEngagementService::Get(profile());
    for (size_t i = 1; i <= num_hosts; i++) {
      GURL url = GURL(base::StringPrintf("https://domain%zu.com", i));
      service->AddPointsForTesting(url, int(i));
    }
  }

  void PopulateTopHostBlacklist(size_t num_hosts) {
    PrefService* pref_service = profile()->GetPrefs();

    std::unique_ptr<base::DictionaryValue> top_host_filter =
        pref_service
            ->GetDictionary(
                optimization_guide::prefs::kHintsFetcherTopHostBlacklist)
            ->CreateDeepCopy();

    for (size_t i = 1; i <= num_hosts; i++) {
      top_host_filter->SetBoolKey(
          HashHostForDictionary(base::StringPrintf("domain%zu.com", i)), true);
    }
    pref_service->Set(optimization_guide::prefs::kHintsFetcherTopHostBlacklist,
                      *top_host_filter);
  }

  void RemoveHostsFromBlacklist(size_t num_hosts_navigated) {
    PrefService* pref_service = profile()->GetPrefs();

    std::unique_ptr<base::DictionaryValue> top_host_filter =
        pref_service
            ->GetDictionary(
                optimization_guide::prefs::kHintsFetcherTopHostBlacklist)
            ->CreateDeepCopy();

    for (size_t i = 1; i <= num_hosts_navigated; i++) {
      top_host_filter->RemoveKey(
          HashHostForDictionary(base::StringPrintf("domain%zu.com", i)));
    }
    pref_service->Set(optimization_guide::prefs::kHintsFetcherTopHostBlacklist,
                      *top_host_filter);
  }

  void SetTopHostBlacklistState(
      optimization_guide::prefs::HintsFetcherTopHostBlacklistState
          blacklist_state) {
    profile()->GetPrefs()->SetInteger(
        optimization_guide::prefs::kHintsFetcherTopHostBlacklistState,
        static_cast<int>(blacklist_state));
  }

  optimization_guide::prefs::HintsFetcherTopHostBlacklistState
  GetCurrentTopHostBlacklistState() {
    PrefService* pref_service = profile()->GetPrefs();
    return static_cast<
        optimization_guide::prefs::HintsFetcherTopHostBlacklistState>(
        pref_service->GetInteger(
            optimization_guide::prefs::kHintsFetcherTopHostBlacklistState));
  }

  void TearDown() override { ChromeRenderViewHostTestHarness::TearDown(); }

  PreviewsTopHostProviderImpl* top_host_provider() {
    return top_host_provider_.get();
  }

 private:
  std::unique_ptr<PreviewsTopHostProviderImpl> top_host_provider_;
};

TEST_F(PreviewsTopHostProviderImplTest, GetTopHostsMaxSites) {
  SetTopHostBlacklistState(optimization_guide::prefs::
                               HintsFetcherTopHostBlacklistState::kInitialized);
  size_t engaged_hosts = 5;
  size_t max_top_hosts = 3;
  AddEngagedHosts(engaged_hosts);

  std::vector<std::string> hosts =
      top_host_provider()->GetTopHosts(max_top_hosts);

  EXPECT_EQ(max_top_hosts, hosts.size());
}

TEST_F(PreviewsTopHostProviderImplTest,
       GetTopHostsFiltersPrivacyBlackedlistedHosts) {
  SetTopHostBlacklistState(optimization_guide::prefs::
                               HintsFetcherTopHostBlacklistState::kInitialized);
  size_t engaged_hosts = 5;
  size_t max_top_hosts = 5;
  size_t num_hosts_blacklisted = 2;
  AddEngagedHosts(engaged_hosts);

  PopulateTopHostBlacklist(num_hosts_blacklisted);

  std::vector<std::string> hosts =
      top_host_provider()->GetTopHosts(max_top_hosts);

  EXPECT_EQ(hosts.size(), engaged_hosts - num_hosts_blacklisted);
}

TEST_F(PreviewsTopHostProviderImplTest, GetTopHostsInitializeBlacklistState) {
  EXPECT_EQ(GetCurrentTopHostBlacklistState(),
            optimization_guide::prefs::HintsFetcherTopHostBlacklistState::
                kNotInitialized);
  size_t engaged_hosts = 5;
  size_t max_top_hosts = 5;
  AddEngagedHosts(engaged_hosts);

  std::vector<std::string> hosts =
      top_host_provider()->GetTopHosts(max_top_hosts);
  // On initialization, GetTopHosts should always return zero hosts.
  EXPECT_EQ(hosts.size(), 0u);
  EXPECT_EQ(GetCurrentTopHostBlacklistState(),
            optimization_guide::prefs::HintsFetcherTopHostBlacklistState::
                kInitialized);
}

TEST_F(PreviewsTopHostProviderImplTest,
       GetTopHostsBlacklistStateNotInitializedToInitialized) {
  size_t engaged_hosts = 5;
  size_t max_top_hosts = 5;
  size_t num_hosts_blacklisted = 5;
  AddEngagedHosts(engaged_hosts);
  // TODO(mcrouse): Remove once the blacklist is populated on initialization.
  // The expected behavior will be that all hosts in the engagement service will
  // be blacklisted on initialization.
  PopulateTopHostBlacklist(num_hosts_blacklisted);

  std::vector<std::string> hosts =
      top_host_provider()->GetTopHosts(max_top_hosts);
  EXPECT_EQ(hosts.size(), 0u);

  // Blacklist should now have items removed.
  size_t num_navigations = 2;
  RemoveHostsFromBlacklist(num_navigations);

  hosts = top_host_provider()->GetTopHosts(max_top_hosts);
  EXPECT_EQ(hosts.size(),
            engaged_hosts - (num_hosts_blacklisted - num_navigations));
  EXPECT_EQ(GetCurrentTopHostBlacklistState(),
            optimization_guide::prefs::HintsFetcherTopHostBlacklistState::
                kInitialized);
}

TEST_F(PreviewsTopHostProviderImplTest,
       GetTopHostsBlacklistStateNotInitializedToEmpty) {
  size_t engaged_hosts = 5;
  size_t max_top_hosts = 5;
  size_t num_hosts_blacklisted = 5;
  AddEngagedHosts(engaged_hosts);
  // TODO(mcrouse): Remove once the blacklist is populated on initialization.
  // The expected behavior will be that all hosts in the engagement service will
  // be blacklisted on initialization.
  PopulateTopHostBlacklist(num_hosts_blacklisted);

  std::vector<std::string> hosts =
      top_host_provider()->GetTopHosts(max_top_hosts);
  EXPECT_EQ(hosts.size(), 0u);

  // Blacklist should now have items removed.
  size_t num_navigations = 5;
  RemoveHostsFromBlacklist(num_navigations);

  hosts = top_host_provider()->GetTopHosts(max_top_hosts);
  EXPECT_EQ(hosts.size(),
            engaged_hosts - (num_hosts_blacklisted - num_navigations));
  EXPECT_EQ(
      GetCurrentTopHostBlacklistState(),
      optimization_guide::prefs::HintsFetcherTopHostBlacklistState::kEmpty);
}

}  // namespace previews
