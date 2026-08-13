// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#include "navigation/NavigationControllerImpl.h"

#include "base/logging.h"
#include "base/sequence_checker.h"

namespace veor {

NavigationControllerImpl::NavigationControllerImpl() = default;
NavigationControllerImpl::~NavigationControllerImpl() = default;

void NavigationControllerImpl::PushEntry(const NavigationEntry& entry) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Truncate forward history if we are not at the end
  if (current_index_ + 1 < entries_.size()) {
    entries_.erase(entries_.begin() + current_index_ + 1, entries_.end());
  }

  entries_.push_back(entry);
  current_index_ = entries_.size() - 1;

  if (on_url_changed_)
    on_url_changed_.Run(entry.url);
}

bool NavigationControllerImpl::CanGoBack() const {
  return current_index_ > 0;
}

bool NavigationControllerImpl::CanGoForward() const {
  return current_index_ + 1 < entries_.size();
}

GURL NavigationControllerImpl::GoBack() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!CanGoBack())
    return GURL();

  current_index_--;
  GURL url = entries_[current_index_].url;
  if (on_url_changed_)
    on_url_changed_.Run(url);
  return url;
}

GURL NavigationControllerImpl::GoForward() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!CanGoForward())
    return GURL();

  current_index_++;
  GURL url = entries_[current_index_].url;
  if (on_url_changed_)
    on_url_changed_.Run(url);
  return url;
}

GURL NavigationControllerImpl::GetCurrentUrl() const {
  if (entries_.empty())
    return GURL();
  return entries_[current_index_].url;
}

void NavigationControllerImpl::SetOnUrlChanged(
    base::RepeatingCallback<void(const GURL&)> cb) {
  on_url_changed_ = std::move(cb);
}

}  // namespace veor
