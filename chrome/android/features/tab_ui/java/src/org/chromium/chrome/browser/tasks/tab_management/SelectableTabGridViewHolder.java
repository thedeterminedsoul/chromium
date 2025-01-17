// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tasks.tab_management;

import android.content.res.ColorStateList;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.InsetDrawable;
import android.support.graphics.drawable.AnimatedVectorDrawableCompat;
import android.support.v4.content.res.ResourcesCompat;
import android.support.v7.content.res.AppCompatResources;

import org.chromium.chrome.R;

/**
 * A {@link TabGridViewHolder} with a checkable button. This is used in the manual selection mode.
 */
class SelectableTabGridViewHolder extends TabGridViewHolder {
    public final int defaultLevel;
    public final int selectedLevel;
    public AnimatedVectorDrawableCompat mCheckDrawable;
    public ColorStateList iconColorList;

    SelectableTabGridViewHolder(TabGridView itemView) {
        super(itemView);

        defaultLevel = itemView.getResources().getInteger(
                org.chromium.chrome.R.integer.list_item_level_default);
        selectedLevel = itemView.getResources().getInteger(
                org.chromium.chrome.R.integer.list_item_level_selected);
        mCheckDrawable = AnimatedVectorDrawableCompat.create(
                itemView.getContext(), R.drawable.ic_check_googblue_24dp_animated);
        iconColorList = AppCompatResources.getColorStateList(
                itemView.getContext(), org.chromium.chrome.R.color.default_icon_color_inverse);

        Drawable selectedDrawable = new InsetDrawable(
                ResourcesCompat.getDrawable(itemView.getResources(),
                        org.chromium.chrome.tab_ui.R.drawable.tab_grid_selection_list_icon,
                        itemView.getContext().getTheme()),
                (int) itemView.getResources().getDimension(
                        org.chromium.chrome.tab_ui.R.dimen.selection_tab_grid_toggle_button_inset));
        actionButton.setBackground(selectedDrawable);
        actionButton.getBackground().setLevel(defaultLevel);
    }
}
