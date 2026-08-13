// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "devtools/VeorDevToolsPanels.h"

#include "base/json/json_writer.h"
#include "base/values.h"
#include "core/logging/VeorLogger.h"
#include "core/memory/IMemoryTracker.h"
#include "sync/ISyncEngine.h"
#include "workspace/WorkspaceManager.h"

namespace veor {

VeorDevToolsPanels::VeorDevToolsPanels(MemoryTracker* memory,
                                       WorkspaceManager* workspaces,
                                       ISyncEngine* sync)
    : memory_(memory), workspaces_(workspaces), sync_(sync) {}

void VeorDevToolsPanels::OnDevToolsOpened(
    content::DevToolsAgentHost* agent_host) {
  if (!agent_host)
    return;

  VEOR_LOGI(LogCategory::kContent, "Injecting VEOR custom DevTools panels");

  InjectMemoryPanel(agent_host);
  InjectWorkspacePanel(agent_host);
  InjectSyncPanel(agent_host);
}

void VeorDevToolsPanels::UpdatePanels() {
  // In a full implementation, this would broadcast updates to all open
  // DevTools instances via DevToolsAgentHost::SendMessage.
  // For MVP, panels are static after injection.
}

void VeorDevToolsPanels::InjectMemoryPanel(content::DevToolsAgentHost* host) {
  std::string payload = BuildMemoryPayload();
  std::string script =
      "(() => {"
      "  if (window.__veor_memory_panel) return;"
      "  const data = " + payload + ";"
      "  console.log('[VEOR Memory]', data);"
      "  window.__veor_memory_panel = data;"
      "  if (typeof UI !== 'undefined' && UI.panels) {"
      "    UI.panels.createPanel('VEOR Memory', 'memory',"
      "      '<pre>' + JSON.stringify(data, null, 2) + '</pre>');"
      "  }"
      "})();";

  host->DispatchProtocolMessage(
      base::as_bytes(base::make_span(script)));
}

void VeorDevToolsPanels::InjectWorkspacePanel(content::DevToolsAgentHost* host) {
  std::string payload = BuildWorkspacePayload();
  std::string script =
      "(() => {"
      "  if (window.__veor_workspace_panel) return;"
      "  const data = " + payload + ";"
      "  console.log('[VEOR Workspaces]', data);"
      "  window.__veor_workspace_panel = data;"
      "  if (typeof UI !== 'undefined' && UI.panels) {"
      "    UI.panels.createPanel('VEOR Workspaces', 'workspace',"
      "      '<pre>' + JSON.stringify(data, null, 2) + '</pre>');"
      "  }"
      "})();";

  host->DispatchProtocolMessage(
      base::as_bytes(base::make_span(script)));
}

void VeorDevToolsPanels::InjectSyncPanel(content::DevToolsAgentHost* host) {
  std::string payload = BuildSyncPayload();
  std::string script =
      "(() => {"
      "  if (window.__veor_sync_panel) return;"
      "  const data = " + payload + ";"
      "  console.log('[VEOR Sync]', data);"
      "  window.__veor_sync_panel = data;"
      "  if (typeof UI !== 'undefined' && UI.panels) {"
      "    UI.panels.createPanel('VEOR Sync', 'sync',"
      "      '<pre>' + JSON.stringify(data, null, 2) + '</pre>');"
      "  }"
      "})();";

  host->DispatchProtocolMessage(
      base::as_bytes(base::make_span(script)));
}

std::string VeorDevToolsPanels::BuildMemoryPayload() {
  base::Value::Dict dict;
  if (memory_) {
    auto snapshot = memory_->GetSnapshot();
    dict.Set("total_allocated_bytes",
             static_cast<double>(snapshot.total_allocated_bytes));
    dict.Set("peak_allocated_bytes",
             static_cast<double>(snapshot.peak_allocated_bytes));
    dict.Set("tab_count", static_cast<int>(snapshot.tab_count));
    dict.Set("timestamp", base::Time::Now().ToTimeT());
  } else {
    dict.Set("status", "Memory tracker not available");
  }
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

std::string VeorDevToolsPanels::BuildWorkspacePayload() {
  base::Value::Dict dict;
  if (workspaces_) {
    dict.Set("workspace_count",
             static_cast<int>(workspaces_->GetWorkspaceCount()));
    dict.Set("active_workspace_id",
             static_cast<double>(workspaces_->GetActiveWorkspaceId().value()));
  } else {
    dict.Set("status", "Workspace manager not available");
  }
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

std::string VeorDevToolsPanels::BuildSyncPayload() {
  base::Value::Dict dict;
  if (sync_) {
    auto status = sync_->GetStatus();
    dict.Set("enabled", status.enabled);
    dict.Set("authenticated", status.authenticated);
    dict.Set("pending_changes", static_cast<int>(status.pending_changes));
    dict.Set("last_sync", status.last_sync_time);
    if (!status.error_message.empty())
      dict.Set("error", status.error_message);
  } else {
    dict.Set("status", "Sync engine not available");
  }
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

}  // namespace veor
