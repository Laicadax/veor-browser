// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/VeorRendererClient.h"

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "core/logging/VeorLogger.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace veor {

namespace {

constexpr char kReaderModeCSS[] = R"css(
/* VEOR Reader Mode */
.veor-reader-active body {
  background: #0a0a0a !important;
  color: rgba(255,255,255,0.92) !important;
  font-family: "Inter", -apple-system, sans-serif !important;
  font-size: 18px !important;
  line-height: 1.7 !important;
  max-width: 680px !important;
  margin: 0 auto !important;
  padding: 48px 24px !important;
}
.veor-reader-active img,
.veor-reader-active video,
.veor-reader-active iframe,
.veor-reader-active nav,
.veor-reader-active header,
.veor-reader-active footer,
.veor-reader-active aside,
.veor-reader-active .ad,
.veor-reader-active .ads,
.veor-reader-active .sidebar,
.veor-reader-active .comments,
.veor-reader-active .social,
.veor-reader-active .share {
  display: none !important;
}
.veor-reader-active article,
.veor-reader-active main,
.veor-reader-active .content,
.veor-reader-active .post,
.veor-reader-active .entry {
  display: block !important;
  width: 100% !important;
}
)css";

constexpr char kReaderModeJS[] = R"js(
(function() {
  window.__veor_reader = {
    active: false,
    originalHTML: null,
    enable: function() {
      if (this.active) return;
      this.originalHTML = document.documentElement.innerHTML;
      document.documentElement.classList.add('veor-reader-active');
      var candidates = Array.from(document.querySelectorAll('article, main, [role="main"], .content, .post, .entry, #content'));
      var best = null, bestScore = 0;
      candidates.forEach(function(el) {
        var text = el.innerText || '';
        var score = text.length;
        if (score > bestScore) { bestScore = score; best = el; }
      });
      if (best) {
        document.body.innerHTML = '';
        document.body.appendChild(best);
      }
      this.active = true;
    },
    disable: function() {
      if (!this.active || !this.originalHTML) return;
      document.documentElement.innerHTML = this.originalHTML;
      this.active = false;
      this.originalHTML = null;
    },
    toggle: function() {
      this.active ? this.disable() : this.enable();
    }
  };
})();
)js";

}  // namespace

VeorRendererClient::VeorRendererClient() = default;
VeorRendererClient::~VeorRendererClient() = default;

void VeorRendererClient::RenderFrameCreated(content::RenderFrame* render_frame) {
  VEOR_LOGD(LogCategory::kRenderer, "RenderFrame created");

  blink::WebLocalFrame* web_frame = render_frame->GetWebFrame();
  if (!web_frame)
    return;

  // Inject reader mode stylesheet via DOM
  blink::WebScriptSource css_script(blink::WebString::FromUTF8(
      std::string("(function(){var s=document.createElement('style');"
                  "s.textContent=") + std::string(kReaderModeCSS) +
      ";document.head.appendChild(s);})();"));
  web_frame->ExecuteScript(css_script);

  // Inject reader mode JavaScript bridge
  blink::WebScriptSource js_script(blink::WebString::FromUTF8(kReaderModeJS));
  web_frame->ExecuteScript(js_script);

  VEOR_LOGD(LogCategory::kRenderer, "Reader mode scripts injected");
}

void VeorRendererClient::RenderViewCreated(content::RenderView* render_view) {
  VEOR_LOGD(LogCategory::kRenderer, "RenderView created");
}

}  // namespace veor
