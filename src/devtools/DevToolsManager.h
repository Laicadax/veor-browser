// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include "url/gurl.h"
#include "base/memory/raw_ptr.h"
#include <unordered_map>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// DevToolsManager
// ─────────────────────────────────────────────────────────────────────────────
// Manages DevTools sessions per WebContents.
// Opens DevTools frontend in a docked or undocked WebContents.
//
// Each session:
//   - Creates DevToolsAgentHost for the target WebContents
//   - Creates a second WebContents loading the DevTools frontend
//   - Frontend connects via WebSocket to the agent
// ─────────────────────────────────────────────────────────────────────────────

class DevToolsManager : public content::DevToolsAgentHostClient,
                        public content::WebContentsDelegate,
                        public content::WebContentsObserver {
 public:
  explicit DevToolsManager(content::BrowserContext* browser_context);
  ~DevToolsManager() override;

  // Toggle DevTools for the given WebContents
  void ToggleFor(content::WebContents* web_contents);

  // Close DevTools for the given WebContents
  void CloseFor(content::WebContents* web_contents);

  // Check if DevTools is open for WebContents
  bool IsOpenFor(content::WebContents* web_contents) const;

  // content::DevToolsAgentHostClient
  void DispatchProtocolMessage(
      content::DevToolsAgentHost* agent_host,
      base::span<const uint8_t> message) override;
  void OnAgentHostDestroyed(
      content::DevToolsAgentHost* agent_host) override;

  // content::WebContentsDelegate
  void CloseContents(content::WebContents* source) override;
  void ActivateContents(content::WebContents* contents) override;

 private:
  struct Session {
    scoped_refptr<content::DevToolsAgentHost> agent_host;
    std::unique_ptr<content::WebContents> devtools_contents;
    bool is_docked = false;
  };

  void OpenDevTools(content::WebContents* web_contents);
  void CloseDevTools(content::WebContents* web_contents);
  GURL GetDevToolsFrontendURL(content::DevToolsAgentHost* agent_host);

  raw_ptr<content::BrowserContext> browser_context_= nullptr;
  std::unordered_map<content::WebContents*, Session> sessions_;

  base::WeakPtrFactory<DevToolsManager> weak_factory_{this};
};

}  // namespace veor
