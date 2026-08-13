// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "infrastructure/network/NetworkServiceImpl.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "core/logging/VeorLogger.h"

namespace veor {

namespace {

constexpr net::NetworkTrafficAnnotationTag kVeorTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("veor_browser", R"(
      semantics {
        sender: "VEOR Browser"
        description: "General HTTP requests initiated by the browser."
        trigger: "User navigation, API calls, resource loading."
        data: "URL, headers, request body."
        destination: OTHER
      }
      policy {
        cookies_allowed: YES
        cookie_store: "user"
        setting: "No user control."
        policy_exception_justification: "Required for browser functionality."
      }
    )");

std::string HttpMethodToString(HttpMethod method) {
  switch (method) {
    case HttpMethod::kGet:    return "GET";
    case HttpMethod::kPost:   return "POST";
    case HttpMethod::kPut:    return "PUT";
    case HttpMethod::kDelete: return "DELETE";
    case HttpMethod::kHead:   return "HEAD";
    case HttpMethod::kPatch:  return "PATCH";
  }
  return "GET";
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// RequestContext
// ─────────────────────────────────────────────────────────────────────────────

class NetworkServiceImpl::RequestContext {
 public:
  RequestContext(uint64_t id,
                 std::string tag,
                 NetworkServiceImpl* service,
                 std::unique_ptr<network::SimpleURLLoader> loader,
                 base::OnceCallback<void(Result<HttpResponse, NetworkError>)> callback)
      : id_(id),
        tag_(std::move(tag)),
        service_(service),
        loader_(std::move(loader)),
        callback_(std::move(callback)) {}

  uint64_t id() const { return id_; }
  const std::string& tag() const { return tag_; }

  void Start(scoped_refptr<network::SharedURLLoaderFactory> factory) {
    loader_->DownloadToString(
        std::move(factory),
        base::BindOnce(&RequestContext::OnResponse, base::Unretained(this)),
        10 * 1024 * 1024);  // 10 MB max body size
  }

  void OnResponse(std::unique_ptr<std::string> response_body) {
    int net_error = loader_->NetError();
    if (net_error != net::OK) {
      std::move(callback_).Run(
          Result<HttpResponse, NetworkError>::Err(
              NetworkError{net_error, net::ErrorToString(net_error), false}));
      service_->RemoveContext(tag_, id_);
      return;
    }

    auto* info = loader_->ResponseInfo();
    HttpResponse response;
    response.status_code = info && info->headers ? info->headers->response_code() : 0;
    response.final_url = loader_->GetFinalURL();

    if (response_body) {
      response.body.assign(
          reinterpret_cast<const uint8_t*>(response_body->data()),
          reinterpret_cast<const uint8_t*>(response_body->data() + response_body->size()));
    }

    if (info && info->headers) {
      size_t iter = 0;
      std::string name, value;
      while (info->headers->EnumerateHeaderLines(&iter, &name, &value)) {
        response.headers.push_back({name, value});
      }
    }

    std::move(callback_).Run(
        Result<HttpResponse, NetworkError>::Ok(std::move(response)));
    service_->RemoveContext(tag_, id_);
  }

 private:
  uint64_t id_;
  std::string tag_;
  NetworkServiceImpl* service_;
  std::unique_ptr<network::SimpleURLLoader> loader_;
  base::OnceCallback<void(Result<HttpResponse, NetworkError>)> callback_;
};

// ─────────────────────────────────────────────────────────────────────────────
// NetworkServiceImpl
// ─────────────────────────────────────────────────────────────────────────────

NetworkServiceImpl::NetworkServiceImpl(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(std::move(url_loader_factory)) {}

NetworkServiceImpl::~NetworkServiceImpl() = default;

void NetworkServiceImpl::SendRequest(
    const HttpRequest& request,
    base::OnceCallback<void(Result<HttpResponse, NetworkError>)> callback) {
  {
    std::lock_guard<std::mutex> lock(interceptor_mutex_);
    if (interceptor_ && !interceptor_.Run(request)) {
      std::move(callback).Run(
          Result<HttpResponse, NetworkError>::Err(
              NetworkError{-1, "Request blocked by interceptor", false}));
      return;
    }
  }

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = request.url;
  resource_request->method = HttpMethodToString(request.method);

  for (const auto& header : request.headers) {
    resource_request->headers.SetHeader(header.name, header.value);
  }

  uint64_t id = request_id_counter_.fetch_add(1);

  auto loader = network::SimpleURLLoader::Create(
      std::move(resource_request), kVeorTrafficAnnotation);
  loader->SetTimeoutDuration(request.timeout);

  if (!request.body.empty()) {
    loader->AttachStringForUpload(
        std::string(request.body.begin(), request.body.end()),
        "application/octet-stream");
  }

  auto context = std::make_unique<RequestContext>(
      id, request.tag, this, std::move(loader), std::move(callback));
  RequestContext* raw_context = context.get();

  {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    requests_[request.tag].push_back(std::move(context));
  }

  raw_context->Start(url_loader_factory_);

  VEOR_LOGD(LogCategory::kNetwork,
            base::StringPrintf("HTTP %s %s",
                              HttpMethodToString(request.method).c_str(),
                              request.url.spec().c_str()));
}

void NetworkServiceImpl::CancelRequests(const std::string& tag) {
  std::lock_guard<std::mutex> lock(requests_mutex_);
  requests_.erase(tag);
}

void NetworkServiceImpl::RemoveContext(const std::string& tag, uint64_t id) {
  std::lock_guard<std::mutex> lock(requests_mutex_);
  auto it = requests_.find(tag);
  if (it == requests_.end()) return;

  auto& vec = it->second;
  vec.erase(
      std::remove_if(vec.begin(), vec.end(),
                     [id](const std::unique_ptr<RequestContext>& ctx) {
                       return ctx->id() == id;
                     }),
      vec.end());

  if (vec.empty()) {
    requests_.erase(it);
  }
}

void NetworkServiceImpl::SetRequestInterceptor(
    base::RepeatingCallback<bool(const HttpRequest&)> interceptor) {
  std::lock_guard<std::mutex> lock(interceptor_mutex_);
  interceptor_ = std::move(interceptor);
}

}  // namespace veor
