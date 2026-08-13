// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#include "ui/settings/SettingsView.h"

#include "settings/ISettingsProvider.h"
#include "settings/SettingsSchema.h"
#include "ui/theme/IThemeProvider.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/views/background.h"

namespace veor {

BEGIN_METADATA(SettingsView)
END_METADATA

namespace {

constexpr int kCategoryWidth = 160;
constexpr int kRowHeight = 44;
constexpr int kToggleWidth = 36;
constexpr int kToggleHeight = 18;
constexpr int kPadding = 20;

void DrawToggle(gfx::Canvas* canvas,
                const gfx::Rect& bounds,
                bool on,
                SkColor on_color,
                SkColor off_color,
                SkColor thumb_color) {
  SkPaint bg;
  bg.setColor(on ? on_color : off_color);
  bg.setAntiAlias(true);
  canvas->sk_canvas()->drawRoundRect(
      SkRect::MakeXYWH(bounds.x(), bounds.y(), bounds.width(), bounds.height()),
      bounds.height() / 2.0f, bounds.height() / 2.0f, bg);

  SkPaint thumb;
  thumb.setColor(thumb_color);
  thumb.setAntiAlias(true);
  int thumb_radius = bounds.height() / 2 - 2;
  int thumb_x = on ? bounds.right() - thumb_radius - 3
                   : bounds.x() + thumb_radius + 3;
  canvas->sk_canvas()->drawCircle(
      thumb_x, bounds.y() + bounds.height() / 2, thumb_radius, thumb);
}

}  // namespace

SettingsView::SettingsView(IThemeProvider* theme, ISettingsProvider* provider)
    : theme_(theme), provider_(provider) {
  DCHECK(theme_);
  categories_ = SettingsSchema::GetInstance().GetCategories();
  if (!categories_.empty()) {
    current_settings_ = SettingsSchema::GetInstance().GetByCategory(
        categories_[0]);
  }
  SetBackground(views::CreateSolidBackground(
      theme_->GetColor(ColorRole::kSurface)));
}

SettingsView::~SettingsView() = default;

void SettingsView::Show() {
  SetVisible(true);
  SchedulePaint();
}

void SettingsView::Hide() {
  SetVisible(false);
  SchedulePaint();
}

bool SettingsView::IsVisible() const {
  return views::View::GetVisible();
}

gfx::Size SettingsView::CalculatePreferredSize() const {
  return gfx::Size(640, 480);
}

void SettingsView::Layout() {
  // Two-pane layout handled in OnPaint
}

void SettingsView::OnPaint(gfx::Canvas* canvas) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  gfx::Rect bounds = GetLocalBounds();
  canvas->FillRect(bounds, theme_->GetColor(ColorRole::kSurface));

  // Top edge — thin crimson line
  SkPaint edge_paint;
  edge_paint.setColor(theme_->GetColor(ColorRole::kAccentCrimson));
  edge_paint.setStrokeWidth(1);
  canvas->sk_canvas()->drawLine(0, 0, bounds.width(), 0, edge_paint);

  // Vertical separator
  SkPaint sep;
  sep.setColor(theme_->GetColor(ColorRole::kEdge));
  sep.setStrokeWidth(1);
  canvas->sk_canvas()->drawLine(kCategoryWidth, 0, kCategoryWidth,
                                bounds.height(), sep);

  // Left: category list
  DrawCategoryList(canvas, gfx::Rect(0, 0, kCategoryWidth, bounds.height()));

  // Right: settings panel
  DrawSettingsPanel(
      canvas,
      gfx::Rect(kCategoryWidth + 1, 0, bounds.width() - kCategoryWidth - 1,
                bounds.height()));
}

void SettingsView::DrawCategoryList(gfx::Canvas* canvas,
                                    const gfx::Rect& bounds) {
  gfx::FontList font("Inter, 12px");
  SkColor text_color = theme_->GetColor(ColorRole::kTextPrimary);
  SkColor muted = theme_->GetColor(ColorRole::kTextQuaternary);

  int y = kPadding;
  for (size_t i = 0; i < categories_.size(); ++i) {
    const std::string& cat = categories_[i];
    gfx::Rect row(0, y, bounds.width(), 32);

    if (static_cast<int>(i) == hovered_category_) {
      SkPaint hover;
      hover.setColor(theme_->GetColor(ColorRole::kButtonHoverBackground));
      canvas->DrawRect(row, hover);
    }

    if (i == selected_category_) {
      SkPaint active;
      active.setColor(theme_->GetColor(ColorRole::kTabActiveBorderBottom));
      active.setStrokeWidth(2);
      canvas->sk_canvas()->drawLine(0, row.y() + row.height() - 1,
                                    row.width(), row.y() + row.height() - 1,
                                    active);
    }

    std::u16string label = base::UTF8ToUTF16(cat);
    canvas->DrawStringRect(label,
                           gfx::Rect(16, y + 8, bounds.width() - 32, 16),
                           font,
                           gfx::Canvas::TEXT_ALIGN_LEFT,
                           i == selected_category_ ? text_color : muted);
    y += 32;
  }
}

void SettingsView::DrawSettingsPanel(gfx::Canvas* canvas,
                                     const gfx::Rect& bounds) {
  gfx::FontList title_font("Inter, 14px");
  gfx::FontList label_font("Inter, 12px");
  gfx::FontList desc_font("Inter, 11px");
  SkColor text_color = theme_->GetColor(ColorRole::kTextPrimary);
  SkColor muted = theme_->GetColor(ColorRole::kTextQuaternary);

  if (categories_.empty())
    return;

  // Category title
  int y = kPadding;
  std::u16string title = base::UTF8ToUTF16(categories_[selected_category_]);
  canvas->DrawStringRect(title,
                         gfx::Rect(kPadding, y, bounds.width() - kPadding * 2,
                                   20),
                         title_font, gfx::Canvas::TEXT_ALIGN_LEFT, text_color);
  y += 28;

  // Separator
  SkPaint sep;
  sep.setColor(theme_->GetColor(ColorRole::kEdge));
  sep.setStrokeWidth(1);
  canvas->sk_canvas()->drawLine(kPadding, y, bounds.width() - kPadding, y, sep);
  y += 16;

  // Settings rows
  for (size_t i = 0; i < current_settings_.size(); ++i) {
    const SettingDef* def = current_settings_[i];
    gfx::Rect row(kPadding, y, bounds.width() - kPadding * 2, kRowHeight);

    if (static_cast<int>(i) == hovered_setting_) {
      SkPaint hover;
      hover.setColor(theme_->GetColor(ColorRole::kButtonHoverBackground));
      canvas->DrawRect(row, hover);
    }

    DrawSettingRow(canvas, def, row, static_cast<int>(i));
    y += kRowHeight;
  }
}

void SettingsView::DrawSettingRow(gfx::Canvas* canvas,
                                  const SettingDef* def,
                                  const gfx::Rect& row_bounds,
                                  int row_index) {
  gfx::FontList label_font("Inter, 12px");
  gfx::FontList desc_font("Inter, 11px");
  SkColor text_color = theme_->GetColor(ColorRole::kTextPrimary);
  SkColor muted = theme_->GetColor(ColorRole::kTextQuaternary);

  // Label
  std::u16string label = base::UTF8ToUTF16(def->string_key);
  size_t dot = def->string_key.find('.');
  if (dot != std::string::npos)
    label = base::UTF8ToUTF16(def->string_key.substr(dot + 1));
  canvas->DrawStringRect(label,
                         gfx::Rect(row_bounds.x() + 8, row_bounds.y() + 4,
                                   row_bounds.width() - 160, 16),
                         label_font, gfx::Canvas::TEXT_ALIGN_LEFT, text_color);

  // Description
  std::u16string desc = base::UTF8ToUTF16(def->description);
  canvas->DrawStringRect(desc,
                         gfx::Rect(row_bounds.x() + 8, row_bounds.y() + 22,
                                   row_bounds.width() - 160, 14),
                         desc_font, gfx::Canvas::TEXT_ALIGN_LEFT, muted);

  // Control (right side)
  int control_x = row_bounds.right() - 140;
  int control_y = row_bounds.y() + (row_bounds.height() - kToggleHeight) / 2;

  if (def->type == SettingType::kBool) {
    bool value = provider_ ? provider_->GetBool(def->string_key,
        std::get<bool>(def->default_value)) : std::get<bool>(def->default_value);
    gfx::Rect toggle_rect(control_x + 104, control_y, kToggleWidth, kToggleHeight);
    DrawToggle(canvas, toggle_rect, value,
               theme_->GetColor(ColorRole::kAccentCrimson),
               theme_->GetColor(ColorRole::kEdge),
               theme_->GetColor(ColorRole::kTextPrimary));
  } else if (def->type == SettingType::kInt && def->range) {
    int value = provider_ ? provider_->GetInt(def->string_key,
        std::get<int>(def->default_value)) : std::get<int>(def->default_value);
    std::string val_str = std::to_string(value);
    canvas->DrawStringRect(base::UTF8ToUTF16(val_str),
                           gfx::Rect(control_x, control_y, 60, 16),
                           label_font, gfx::Canvas::TEXT_ALIGN_RIGHT, text_color);
  } else if (def->type == SettingType::kEnum) {
    std::string value = provider_ ? provider_->GetString(def->string_key,
        std::get<std::string>(def->default_value)) : std::get<std::string>(def->default_value);
    canvas->DrawStringRect(base::UTF8ToUTF16(value),
                           gfx::Rect(control_x, control_y, 120, 16),
                           label_font, gfx::Canvas::TEXT_ALIGN_RIGHT, text_color);
  }
}

bool SettingsView::OnMousePressed(const ui::MouseEvent& event) {
  int x = event.x();
  int y = event.y();

  // Left pane: category selection
  if (x < kCategoryWidth) {
    int cat_index = (y - kPadding) / 32;
    if (cat_index >= 0 && cat_index < static_cast<int>(categories_.size())) {
      selected_category_ = cat_index;
      current_settings_ = SettingsSchema::GetInstance().GetByCategory(
          categories_[selected_category_]);
      SchedulePaint();
      return true;
    }
    return true;
  }

  // Right pane: toggle clicks
  int settings_y = kPadding + 28 + 16;
  int row_index = (y - settings_y) / kRowHeight;
  if (row_index >= 0 && row_index < static_cast<int>(current_settings_.size())) {
    const SettingDef* def = current_settings_[row_index];
    int control_x = width() - kPadding - 140 + 104;
    int control_y = settings_y + row_index * kRowHeight +
                    (kRowHeight - kToggleHeight) / 2;

    if (def->type == SettingType::kBool &&
        x >= control_x && x < control_x + kToggleWidth &&
        y >= control_y && y < control_y + kToggleHeight) {
      if (provider_) {
        bool current = provider_->GetBool(def->string_key,
            std::get<bool>(def->default_value));
        provider_->SetValue(def->string_key, SettingValue{!current});
        SchedulePaint();
      }
      return true;
    }
  }

  return true;
}

void SettingsView::OnMouseMoved(const ui::MouseEvent& event) {
  int x = event.x();
  int y = event.y();

  int old_hovered_cat = hovered_category_;
  int old_hovered_set = hovered_setting_;

  hovered_category_ = -1;
  hovered_setting_ = -1;

  if (x < kCategoryWidth) {
    hovered_category_ = (y - kPadding) / 32;
    if (hovered_category_ < 0 ||
        hovered_category_ >= static_cast<int>(categories_.size())) {
      hovered_category_ = -1;
    }
  } else {
    int settings_y = kPadding + 28 + 16;
    hovered_setting_ = (y - settings_y) / kRowHeight;
    if (hovered_setting_ < 0 ||
        hovered_setting_ >= static_cast<int>(current_settings_.size())) {
      hovered_setting_ = -1;
    }
  }

  if (old_hovered_cat != hovered_category_ ||
      old_hovered_set != hovered_setting_) {
    SchedulePaint();
  }
}

void SettingsView::OnMouseExited(const ui::MouseEvent& event) {
  if (hovered_category_ != -1 || hovered_setting_ != -1) {
    hovered_category_ = -1;
    hovered_setting_ = -1;
    SchedulePaint();
  }
}

}  // namespace veor
