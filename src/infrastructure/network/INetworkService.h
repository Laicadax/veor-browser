// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/base/VeorResult.h"
#include "url/gurl.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// HttpMethod
// ─────────────────────────────────────────────────────────────────────────────

enum class HttpMethod {
  kGet,
  kPost,
  kPut,
  kDelete,
  kHead,
  kPatch
};

// ─────────────────────────────────────────────────────────────────────────────
// HttpHeader
// ─────────────────────────────────────────────────────────────────────────────

struct HttpHeader {
  std::string name;
  std::string value;
};

// ─────────────────────────────────────────────────────────────────────────────
// HttpRequest
// ─────────────────────────────────────────────────────────────────────────────

struct HttpRequest {
  GURL url;
  HttpMethod method = HttpMethod::kGet;
  std::vector<HttpHeader> headers;
  std::vector<uint8_t> body;
  base::TimeDelta timeout = base::Seconds(30);
  std::string tag;  // For cancellation grouping
};

// ─────────────────────────────────────────────────────────────────────────────
// HttpResponse
// ─────────────────────────────────────────────────────────────────────────────

struct HttpResponse {
  int status_code = 0;
  std::vector<HttpHeader> headers;
  std::vector<uint8_t> body;
  GURL final_url;
  base::TimeDelta total_time;
};

// ─────────────────────────────────────────────────────────────────────────────
// NetworkError
// ─────────────────────────────────────────────────────────────────────────────

struct NetworkError {
  int net_error_code = 0;  // Chromium net::Error codes
  std::string message;
  bool is_certificate_error = false;

  std::string ToString() const {
    return "NET[" + std::to_string(net_error_code) + "]: " + message +
           (is_certificate_error ? " [CERT]" : "");
  }
};

// ─────────────────────────────────────────────────────────────────────────────
// INetworkService
// ─────────────────────────────────────────────────────────────────────────────
// HTTP client using Chromium's network service.
//
// Thread safety: [Any Thread] for SendRequest and CancelRequests.

class INetworkService {
 public:
  virtual ~INetworkService() = default;

  // Performs an HTTP request asynchronously.
  // Callback is invoked on the caller's task runner.
  // [Any Thread]
  virtual void SendRequest(
      const HttpRequest& request,
      base::OnceCallback<void(Result<HttpResponse, NetworkError>)> callback) = 0;

  // Cancels all pending requests matching the given tag.
  // [Any Thread]
  virtual void CancelRequests(const std::string& tag) = 0;

  // Sets a global request interceptor (for tracker blocking, etc.).
  // Returns true to allow the request, false to block.
  // [Any Thread]
  virtual void SetRequestInterceptor(
      base::RepeatingCallback<bool(const HttpRequest&)> interceptor) = 0;
};

}  // namespace veor
