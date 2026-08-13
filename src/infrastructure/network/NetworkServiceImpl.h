// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "infrastructure/network/INetworkService.h"

#include <atomic>
#include <mutex>
#include <unordered_map>

#include "base/functional/callback.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// NetworkServiceImpl
// ─────────────────────────────────────────────────────────────────────────────
// Chromium URLLoader-based HTTP client.

class NetworkServiceImpl : public INetworkService {
 public:
  explicit NetworkServiceImpl(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~NetworkServiceImpl() override;

  // INetworkService
  void SendRequest(
      const HttpRequest& request,
      base::OnceCallback<void(Result<HttpResponse, NetworkError>)> callback) override;
  void CancelRequests(const std::string& tag) override;
  void SetRequestInterceptor(
      base::RepeatingCallback<bool(const HttpRequest&)> interceptor) override;

  // Called by RequestContext when a request completes to remove itself.
  void RemoveContext(const std::string& tag, uint64_t id);

 private:
  class RequestContext;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  std::mutex requests_mutex_;
  std::unordered_map<std::string, std::vector<std::unique_ptr<RequestContext>>> requests_;

  std::mutex interceptor_mutex_;
  base::RepeatingCallback<bool(const HttpRequest&)> interceptor_;

  std::atomic<uint64_t> request_id_counter_{0};
};

}  // namespace veor
