#pragma once

#include <string>
#include "url/gurl.h"
#include <vector>

#include "base/time/time.h"
#include "core/base/VeorId.h"

namespace veor {

struct TabSession {
  TabId tab_id;
  GURL url;
  std::string title;
  bool pinned = false;
  bool active = false;
};

class WorkspaceSession {
 public:
  WorkspaceSession() = default;
  ~WorkspaceSession() = default;

  void SetWorkspaceId(WorkspaceId id) { workspace_id_ = id; }
  WorkspaceId GetWorkspaceId() const { return workspace_id_; }

  void SetName(const std::string& name) { name_ = name; }
  const std::string& GetName() const { return name_; }

  void SetLastActiveTime(base::Time time) { last_active_time_ = time; }
  base::Time GetLastActiveTime() const { return last_active_time_; }

  void AddTab(const TabSession& tab) { tabs_.push_back(tab); }
  const std::vector<TabSession>& GetTabs() const { return tabs_; }
  void ClearTabs() { tabs_.clear(); }

 private:
  WorkspaceId workspace_id_;
  std::string name_;
  base::Time last_active_time_;
  std::vector<TabSession> tabs_;
};

}  // namespace veor
