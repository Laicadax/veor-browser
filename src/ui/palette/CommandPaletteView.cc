// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#include "ui/palette/CommandPaletteView.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/theme/IThemeProvider.h"

namespace veor {

BEGIN_METADATA(CommandPaletteView)
END_METADATA

CommandPaletteView::CommandPaletteView(IThemeProvider* theme) : theme_(theme) {
  DCHECK(theme_);
  SetVisible(false);
}

CommandPaletteView::~CommandPaletteView() = default;

void CommandPaletteView::Show() {
  visible_ = true;
  SetVisible(true);
  SchedulePaint();
}

void CommandPaletteView::Hide() {
  visible_ = false;
  SetVisible(false);
  if (on_dismissed_)
    on_dismissed_.Run();
}

bool CommandPaletteView::IsVisible() const {
  return visible_;
}

void CommandPaletteView::SetQuery(const std::string& query) {
  query_ = query;
  SchedulePaint();
}

std::string CommandPaletteView::GetQuery() const {
  return query_;
}

void CommandPaletteView::SetResults(const std::vector<PaletteItem>& items) {
  items_ = items;
  selected_index_ = items_.empty() ? -1 : 0;
  SchedulePaint();
}

void CommandPaletteView::SetSelectedIndex(int index) {
  selected_index_ = index;
  SchedulePaint();
}

void CommandPaletteView::SetOnItemSelected(base::RepeatingCallback<void(int)> cb) {
  on_item_selected_ = std::move(cb);
}

void CommandPaletteView::SetOnQueryChanged(base::RepeatingCallback<void(const std::string&)> cb) {
  on_query_changed_ = std::move(cb);
}

void CommandPaletteView::SetOnDismissed(base::RepeatingClosure cb) {
  on_dismissed_ = std::move(cb);
}

void CommandPaletteView::OnPaint(gfx::Canvas* canvas) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!visible_) return;

  DrawBackdrop(canvas);
  DrawInput(canvas);
  DrawResults(canvas);
}

void CommandPaletteView::DrawBackdrop(gfx::Canvas* canvas) {
  // Full-screen darkness. No blur. Just void.
  canvas->FillRect(GetLocalBounds(),
                   SkColorSetA(theme_->GetColor(ColorRole::kOverlayBackdrop), 0xD9));
}

void CommandPaletteView::DrawInput(gfx::Canvas* canvas) {
  // Input field: centered horizontally, 80px from top.
  int input_w = 560;
  int input_h = 48;
  int x = (width() - input_w) / 2;
  int y = 80;

  gfx::Rect input_rect(x, y, input_w, input_h);

  // Bottom border only — no background, no side borders
  SkPaint border;
  border.setColor(theme_->GetColor(ColorRole::kEdge));
  border.setStrokeWidth(1);
  canvas->sk_canvas()->drawLine(x, y + input_h - 0.5f,
                                x + input_w, y + input_h - 0.5f, border);

  // Query text — monumental, light weight
  gfx::FontList font("Inter, 24px");
  std::u16string text = base::UTF8ToUTF16(query_);
  if (text.empty())
    text = u"";

  canvas->DrawStringRect(
      text, input_rect, font, gfx::Canvas::TEXT_ALIGN_LEFT,
      theme_->GetColor(ColorRole::kTextPrimary));

  // Caret hint: thin vertical line at end of text
  if (query_.empty()) {
    SkPaint caret;
    caret.setColor(theme_->GetColor(ColorRole::kTextQuaternary));
    caret.setStrokeWidth(1);
    canvas->sk_canvas()->drawLine(x, y + 12, x, y + input_h - 12, caret);
  }
}

void CommandPaletteView::DrawResults(gfx::Canvas* canvas) {
  if (items_.empty()) return;

  int start_y = 140;
  int item_h = 32;
  int x = (width() - 560) / 2;

  for (size_t i = 0; i < items_.size() && i < 8; ++i) {
    gfx::Rect item_rect(x, start_y + static_cast<int>(i) * item_h, 560, item_h);

    // Selection highlight — whisper, not a block
    if (static_cast<int>(i) == selected_index_) {
      canvas->FillRect(item_rect, theme_->GetColor(ColorRole::kWhisper));
    }

    // Separator line
    if (i > 0) {
      SkPaint sep;
      sep.setColor(theme_->GetColor(ColorRole::kEdge));
      sep.setStrokeWidth(1);
      canvas->sk_canvas()->drawLine(x, item_rect.y() - 0.5f,
                                    x + 560, item_rect.y() - 0.5f, sep);
    }

    // Title
    gfx::FontList title_font("Inter, 13px");
    gfx::Rect title_rect = item_rect;
    title_rect.Inset(12, 0);
    canvas->DrawStringRect(
        base::UTF8ToUTF16(items_[i].title), title_rect, title_font,
        gfx::Canvas::TEXT_ALIGN_LEFT,
        theme_->GetColor(ColorRole::kTextPrimary));

    // Type label — far right, tiny, muted
    gfx::FontList type_font("Inter, 10px");
    gfx::Rect type_rect = item_rect;
    type_rect.Inset(12, 0);
    canvas->DrawStringRect(
        base::UTF8ToUTF16(items_[i].type), type_rect, type_font,
        gfx::Canvas::TEXT_ALIGN_RIGHT,
        theme_->GetColor(ColorRole::kTextQuaternary));
  }
}

gfx::Size CommandPaletteView::CalculatePreferredSize() const {
  // Overlay fills parent
  return gfx::Size(views::View::kUsePreferredSize,
                   views::View::kUsePreferredSize);
}

bool CommandPaletteView::OnMousePressed(const ui::MouseEvent& event) {
  int input_w = 560;
  int input_h = 48;
  int input_x = (width() - input_w) / 2;
  int input_y = 80;
  gfx::Rect input_rect(input_x, input_y, input_w, input_h);

  int results_top = 140;
  int item_h = 32;
  int results_bottom = results_top + static_cast<int>(items_.size()) * item_h;
  gfx::Rect results_rect(input_x, results_top, input_w,
                         results_bottom - results_top);

  if (input_rect.Contains(event.location()) ||
      results_rect.Contains(event.location())) {
    return true;
  }

  Hide();
  return true;
}

bool CommandPaletteView::OnKeyPressed(const ui::KeyEvent& event) {
  if (event.key_code() == ui::VKEY_ESCAPE) {
    Hide();
    return true;
  }
  if (event.key_code() == ui::VKEY_DOWN &&
      selected_index_ + 1 < static_cast<int>(items_.size())) {
    selected_index_++;
    SchedulePaint();
    return true;
  }
  if (event.key_code() == ui::VKEY_UP && selected_index_ > 0) {
    selected_index_--;
    SchedulePaint();
    return true;
  }
  if (event.key_code() == ui::VKEY_RETURN && selected_index_ >= 0) {
    if (on_item_selected_)
      on_item_selected_.Run(selected_index_);
    Hide();
    return true;
  }
  if (event.key_code() == ui::VKEY_BACK && !query_.empty()) {
    query_.pop_back();
    if (on_query_changed_)
      on_query_changed_.Run(query_);
    SchedulePaint();
    return true;
  }
  uint16_t ch = event.GetCharacter();
  if (ch >= 32 && ch < 127) {
    query_ += static_cast<char>(ch);
    if (on_query_changed_)
      on_query_changed_.Run(query_);
    SchedulePaint();
    return true;
  }
  return views::View::OnKeyPressed(event);
}

}  // namespace veor
