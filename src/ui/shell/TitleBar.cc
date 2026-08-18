// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#include "ui/shell/TitleBar.h"
#include "workspace/Workspace.h"

#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/omnibox/OmniboxView.h"
#include "ui/theme/IThemeProvider.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/layout/box_layout.h"

namespace veor {

BEGIN_METADATA(TitleBar)
END_METADATA

TitleBar::TitleBar(IThemeProvider* theme) : theme_(theme) {
  DCHECK(theme_);
  SetPreferredSize(gfx::Size(0, 36));
  SetBackground(
      views::CreateSolidBackground(theme_->GetColor(ColorRole::kDepth)));

  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(0, 12), 8));

  // Brand — "VEOR"
  brand_label_ = AddChildView(std::make_unique<views::Label>(u"VEOR"));
  brand_label_->SetFontList(gfx::FontList("Inter, 12px"));
  brand_label_->SetEnabledColor(theme_->GetColor(ColorRole::kTextSecondary));
  brand_label_->SetPreferredSize(gfx::Size(48, 36));

  // Workspace name — subtle, next to brand
  workspace_label_ = AddChildView(std::make_unique<views::Label>());
  workspace_label_->SetFontList(gfx::FontList("Inter, 11px"));
  workspace_label_->SetEnabledColor(theme_->GetColor(ColorRole::kTextQuaternary));
  workspace_label_->SetPreferredSize(gfx::Size(100, 36));

  // Omnibox — unified address field, centered, flexes
  omnibox_ = AddChildView(std::make_unique<OmniboxView>(theme_));
  omnibox_->SetPreferredSize(gfx::Size(0, 28));
  omnibox_->SetBorder(views::CreateEmptyBorder(gfx::Insets(4, 0)));
  layout->SetFlexForView(omnibox_, 1);

  // Nav container — back, forward, palette, close
  auto* nav_container = AddChildView(std::make_unique<views::View>());
  nav_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(0), 4));
  nav_container->SetPreferredSize(gfx::Size(100, 36));

  back_button_ = nav_container->AddChildView(std::make_unique<views::View>());
  back_button_->SetPreferredSize(gfx::Size(20, 20));
  back_button_->SetEnabled(false);

  forward_button_ = nav_container->AddChildView(std::make_unique<views::View>());
  forward_button_->SetPreferredSize(gfx::Size(20, 20));
  forward_button_->SetEnabled(false);

  palette_button_ = nav_container->AddChildView(std::make_unique<views::View>());
  palette_button_->SetPreferredSize(gfx::Size(20, 20));

  close_button_ = nav_container->AddChildView(std::make_unique<views::View>());
  close_button_->SetPreferredSize(gfx::Size(20, 20));
}

TitleBar::~TitleBar() = default;

void TitleBar::OnPaint(gfx::Canvas* canvas) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Bottom edge — seam between title bar and tab strip
  SkPaint edge_paint;
  edge_paint.setColor(theme_->GetColor(ColorRole::kEdge));
  edge_paint.setStrokeWidth(1);
  canvas->sk_canvas()->drawLine(0, height() - 0.5f, width(), height() - 0.5f,
                                edge_paint);

  // Hover backgrounds for nav buttons
  auto DrawHoverBg = [&](views::View* btn) {
    gfx::Rect b(btn->GetMirroredBounds());
    b -= GetMirroredPosition();
    SkPaint bg;
    bg.setColor(theme_->GetColor(ColorRole::kButtonHoverBackground));
    canvas->DrawRect(b, bg);
  };

  if (hovered_button_ == HoveredButton::kBack && can_go_back_)
    DrawHoverBg(back_button_);
  if (hovered_button_ == HoveredButton::kForward && can_go_forward_)
    DrawHoverBg(forward_button_);
  if (hovered_button_ == HoveredButton::kPalette)
    DrawHoverBg(palette_button_);
  if (hovered_button_ == HoveredButton::kClose)
    DrawHoverBg(close_button_);

  // Nav button glyphs — drawn by TitleBar over the child views
  auto DrawNav = [&](views::View* btn, const std::u16string& glyph, bool on) {
    gfx::Rect b(btn->GetMirroredBounds());
    b -= GetMirroredPosition();
    SkColor c = on ? theme_->GetColor(ColorRole::kTextTertiary)
                   : theme_->GetColor(ColorRole::kTextQuaternary);
    canvas->DrawStringRect(glyph, b, gfx::FontList("Inter, 14px"),
                           gfx::Canvas::TEXT_ALIGN_CENTER, c);
  };

  DrawNav(back_button_, u"\u2039", can_go_back_);
  DrawNav(forward_button_, u"\u203A", can_go_forward_);

  // Palette trigger — thin horizontal line
  gfx::Rect pal_bounds(palette_button_->GetMirroredBounds());
  pal_bounds -= GetMirroredPosition();
  SkPaint pal_paint;
  pal_paint.setColor(theme_->GetColor(ColorRole::kTextQuaternary));
  pal_paint.setStrokeWidth(1);
  int mid_y = pal_bounds.y() + pal_bounds.height() / 2;
  canvas->sk_canvas()->drawLine(pal_bounds.x() + 4, mid_y,
                                pal_bounds.right() - 4, mid_y, pal_paint);

  // Close — "\u00D7"
  gfx::Rect close_bounds(close_button_->GetMirroredBounds());
  close_bounds -= GetMirroredPosition();
  canvas->DrawStringRect(
      u"\u00D7", close_bounds, gfx::FontList("Inter, 16px"),
      gfx::Canvas::TEXT_ALIGN_CENTER,
      theme_->GetColor(ColorRole::kTextTertiary));
}

gfx::Size TitleBar::CalculatePreferredSize() const {
  return gfx::Size(views::View::kUsePreferredSize, 36);
}

bool TitleBar::OnMousePressed(const ui::MouseEvent& event) {
  gfx::Point point = event.location();

  if (back_button_->GetMirroredBounds().Contains(point) && can_go_back_ && on_back_) {
    on_back_.Run();
    return true;
  }
  if (forward_button_->GetMirroredBounds().Contains(point) && can_go_forward_ && on_forward_) {
    on_forward_.Run();
    return true;
  }
  if (palette_button_->GetMirroredBounds().Contains(point) && on_command_palette_) {
    on_command_palette_.Run();
    return true;
  }
  if (close_button_->GetMirroredBounds().Contains(point) && on_close_) {
    on_close_.Run();
    return true;
  }

  return views::View::OnMousePressed(event);
}

void TitleBar::OnMouseMoved(const ui::MouseEvent& event) {
  gfx::Point point = event.location();
  HoveredButton new_hover = HoveredButton::kNone;

  if (back_button_->GetMirroredBounds().Contains(point))
    new_hover = HoveredButton::kBack;
  else if (forward_button_->GetMirroredBounds().Contains(point))
    new_hover = HoveredButton::kForward;
  else if (palette_button_->GetMirroredBounds().Contains(point))
    new_hover = HoveredButton::kPalette;
  else if (close_button_->GetMirroredBounds().Contains(point))
    new_hover = HoveredButton::kClose;

  if (new_hover != hovered_button_) {
    hovered_button_ = new_hover;
    SchedulePaint();
  }
}

void TitleBar::OnMouseExited(const ui::MouseEvent& event) {
  if (hovered_button_ != HoveredButton::kNone) {
    hovered_button_ = HoveredButton::kNone;
    SchedulePaint();
  }
}

void TitleBar::SetCanGoBack(bool can) {
  can_go_back_ = can;
  back_button_->SetEnabled(can);
  SchedulePaint();
}

void TitleBar::SetCanGoForward(bool can) {
  can_go_forward_ = can;
  forward_button_->SetEnabled(can);
  SchedulePaint();
}

void TitleBar::SetWorkspaceName(const std::string& name) {
  workspace_label_->SetText(base::UTF8ToUTF16(name));
}

void TitleBar::SetWorkspaceList(const std::vector<std::string>& names,
                                size_t active_index) {
  workspace_names_ = names;
  active_workspace_index_ = active_index;
}

void TitleBar::SetOnBackPressed(base::RepeatingClosure cb) {
  on_back_ = std::move(cb);
}

void TitleBar::SetOnForwardPressed(base::RepeatingClosure cb) {
  on_forward_ = std::move(cb);
}

void TitleBar::SetOnWorkspaceSelected(base::RepeatingCallback<void(size_t)> cb) {
  on_workspace_selected_ = std::move(cb);
}

void TitleBar::SetOnCommandPaletteRequested(base::RepeatingClosure cb) {
  on_command_palette_ = std::move(cb);
}

void TitleBar::SetOnCloseRequested(base::RepeatingClosure cb) {
  on_close_ = std::move(cb);
}

}  // namespace veor
