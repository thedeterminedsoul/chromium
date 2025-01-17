// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/srt_chrome_prompt_impl_win.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/srt_field_trial_win.h"
#include "components/crx_file/id_util.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/uninstall_reason.h"

namespace safe_browsing {

using chrome_cleaner::mojom::ChromePrompt;
using chrome_cleaner::mojom::ChromePromptRequest;
using chrome_cleaner::mojom::PromptAcceptance;
using content::BrowserThread;

ChromePromptImpl::ChromePromptImpl(
    extensions::ExtensionService* extension_service,
    ChromePromptRequest request,
    base::OnceClosure on_connection_closed,
    OnPromptUser on_prompt_user)
    : binding_(this, std::move(request)),
      extension_service_(extension_service),
      on_prompt_user_(std::move(on_prompt_user)) {
  DCHECK(on_prompt_user_);
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  binding_.set_connection_error_handler(std::move(on_connection_closed));
}

ChromePromptImpl::~ChromePromptImpl() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void ChromePromptImpl::PromptUser(
    const std::vector<base::FilePath>& files_to_delete,
    const base::Optional<std::vector<base::string16>>& registry_keys,
    const base::Optional<std::vector<base::string16>>& extension_ids,
    ChromePrompt::PromptUserCallback callback) {
  using FileCollection = ChromeCleanerScannerResults::FileCollection;
  using RegistryKeyCollection =
      ChromeCleanerScannerResults::RegistryKeyCollection;
  using ExtensionCollection = ChromeCleanerScannerResults::ExtensionCollection;

  DCHECK(on_prompt_user_);
  if (base::FeatureList::IsEnabled(kChromeCleanupExtensionsFeature) &&
      extension_ids) {
    extension_ids_ = extension_ids.value();
  } else {
    extension_ids_.clear();
  }

  ChromeCleanerScannerResults scanner_results(
      FileCollection(files_to_delete.begin(), files_to_delete.end()),
      registry_keys
          ? RegistryKeyCollection(registry_keys->begin(), registry_keys->end())
          : RegistryKeyCollection(),
      extension_ids_.empty()
          ? ExtensionCollection()
          : ExtensionCollection(extension_ids_.begin(), extension_ids_.end()));

  std::move(on_prompt_user_)
      .Run(std::move(scanner_results), std::move(callback));
}

// The |extensions_ids| passed to this function are a subset of the
// |extension_ids| passed to PromptUser because the extensions are not all
// disabled at the same time.
void ChromePromptImpl::DisableExtensions(
    const std::vector<base::string16>& extension_ids,
    ChromePrompt::DisableExtensionsCallback callback) {
  if (extension_service_ == nullptr || extension_ids_.empty()) {
    std::move(callback).Run(false);
    return;
  }
  // Clear the stored extension_ids by moving it onto this stack frame,
  // so subsequent calls will fail.
  std::vector<base::string16> verified_extension_ids{};
  extension_ids_.swap(verified_extension_ids);
  bool ids_are_valid = std::all_of(
      extension_ids.begin(), extension_ids.end(),
      [this, &verified_extension_ids](const base::string16& id) {
        std::string id_utf8 = base::UTF16ToUTF8(id);
        return crx_file::id_util::IdIsValid(id_utf8) &&
               base::Contains(verified_extension_ids, id) &&
               extension_service_->GetInstalledExtension(id_utf8) != nullptr;
      });
  if (!ids_are_valid) {
    std::move(callback).Run(false);
    return;
  }

  // This only uninstalls extensions that have been displayed to the user on
  // the cleanup page.
  extensions::UninstallReason reason =
      extensions::UNINSTALL_REASON_USER_INITIATED;
  bool result = true;
  for (const base::string16& extension_id : extension_ids) {
    result = extension_service_->UninstallExtension(
                 base::UTF16ToUTF8(extension_id), reason, nullptr) &&
             result;
  }
  std::move(callback).Run(result);
}

}  // namespace safe_browsing
