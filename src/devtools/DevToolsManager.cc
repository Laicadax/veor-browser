// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "devtools/DevToolsManager.h"
#include "url/gurl.h"

#include "base/strings/stringprintf.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"

#include "core/logging/VeorLogger.h"

namespace veor {

namespace {

// DevTools frontend URL. Chromium bundles the frontend at this URL.
// The ws parameter connects to the DevToolsAgentHost via WebSocket.
std::string GetFrontendURL(const std::string& agent_id) {
  return base::StringPrintf(
      "devtools://devtools/bundled/inspector.html?"
      "experiments=true&"
      "ws=localhost:9222/devtools/page/%s",
      agent_id.c_str());
}

}  // namespace

DevToolsManager::DevToolsManager(content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  DCHECK(browser_context_);
}

DevToolsManager::~DevToolsManager() = default;

void DevToolsManager::ToggleFor(content::WebContents* web_contents) {
  if (!web_contents)
    return;

  auto it = sessions_.find(web_contents);
  if (it != sessions_.end()) {
    CloseDevTools(web_contents);
  } else {
    OpenDevTools(web_contents);
  }
}

void DevToolsManager::CloseFor(content::WebContents* web_contents) {
  if (!web_contents)
    return;
  CloseDevTools(web_contents);
}

bool DevToolsManager::IsOpenFor(content::WebContents* web_contents) const {
  return web_contents && sessions_.count(web_contents) > 0;
}

void DevToolsManager::OpenDevTools(content::WebContents* web_contents) {
  Session session;
  session.agent_host = content::DevToolsAgentHost::GetOrCreateFor(web_contents);

  // Create DevTools frontend WebContents
  content::WebContents::CreateParams params(browser_context_);
  session.devtools_contents = content::WebContents::Create(params);
  session.devtools_contents->SetDelegate(this);

  // Load DevTools frontend
  GURL frontend_url(GetFrontendURL(session.agent_host->GetId()));
  session.devtools_contents->GetController().LoadURL(
      frontend_url, content::Referrer(), ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
      std::string());

  // Attach to agent
  session.agent_host->AttachClient(this);

  Observe(web_contents);
  sessions_[web_contents] = std::move(session);

  VEOR_LOGI(LogCategory::kCore,
            "DevTools opened for: " + web_contents->GetVisibleURL().spec());
}

void DevToolsManager::CloseDevTools(content::WebContents* web_contents) {
  auto it = sessions_.find(web_contents);
  if (it == sessions_.end())
    return;

  it->second.agent_host->DetachClient(this);
  sessions_.erase(it);

  VEOR_LOGI(LogCategory::kCore,
            "DevTools closed for: " + web_contents->GetVisibleURL().spec());
}

GURL DevToolsManager::GetDevToolsFrontendURL(
    content::DevToolsAgentHost* agent_host) {
  return GURL(GetFrontendURL(agent_host->GetId()));
}

// content::DevToolsAgentHostClient
void DevToolsManager::DispatchProtocolMessage(
    content::DevToolsAgentHost* agent_host,
    base::span<const uint8_t> message) {
  // Protocol messages from the target page are forwarded to the
  // DevTools frontend. In a full implementation this routes through
  // the WebSocket server (DevToolsHttpHandler). For MVP the
  // frontend loads but the WebSocket endpoint is not active.
}

void DevToolsManager::OnAgentHostDestroyed(
    content::DevToolsAgentHost* agent_host) {
  // Clean up any session referencing this agent
  for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
    if (it->second.agent_host.get() == agent_host) {
      sessions_.erase(it);
      return;
    }
  }
}

// content::WebContentsDelegate
void DevToolsManager::CloseContents(content::WebContents* source) {
  // Find which session owns this devtools contents and close it
  for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
    if (it->second.devtools_contents.get() == source) {
      CloseDevTools(it->first);
      return;
    }
  }
}

void DevToolsManager::ActivateContents(content::WebContents* contents) {
  // No-op for MVP
}

}  // namespace veor
