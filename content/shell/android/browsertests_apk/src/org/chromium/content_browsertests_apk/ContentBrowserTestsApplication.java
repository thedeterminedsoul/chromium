// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_browsertests_apk;

import android.content.Context;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.PathUtils;
import org.chromium.native_test.NativeBrowserTestApplication;

/**
 * A basic content_public.browser.tests {@link android.app.Application}.
 */
public class ContentBrowserTestsApplication extends NativeBrowserTestApplication {
    static final String PRIVATE_DATA_DIRECTORY_SUFFIX = "content_shell";

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);

        if (isBrowserProcess()) {
            ApplicationStatus.initialize(this);

            // Test-only stuff, see also NativeUnitTest.java.
            PathUtils.setPrivateDataDirectorySuffix(PRIVATE_DATA_DIRECTORY_SUFFIX);
        }
    }
}
