#pragma once

#include "history/IHistoryStore.h"
#include "base/time/time.h"
#include "url/gurl.h"

namespace veor {

class HistoryStoreStub : public IHistoryStore {
 public:
  Result<void, std::string> AddVisit(const GURL& url,
                                     const std::string& title,
                                     base::Time visit_time) override {
    return Result<void, std::string>::Ok();
  }

  Result<HistoryQueryResult, std::string> Query(
      const HistoryQuery& query) override {
    return Result<HistoryQueryResult, std::string>::Ok(HistoryQueryResult{});
  }

  Result<std::vector<HistoryEntry>, std::string> GetRecent(
      size_t count) override {
    return Result<std::vector<HistoryEntry>, std::string>::Ok(
        std::vector<HistoryEntry>{});
  }

  Result<void, std::string> DeleteEntry(HistoryEntryId id) override {
    return Result<void, std::string>::Ok();
  }

  Result<void, std::string> DeleteRange(base::Time start,
                                         base::Time end) override {
    return Result<void, std::string>::Ok();
  }

  Result<void, std::string> DeleteAll() override {
    return Result<void, std::string>::Ok();
  }

  Result<size_t, std::string> ExpireOldEntries(base::Time cutoff) override {
    return Result<size_t, std::string>::Ok(0);
  }
};

}  // namespace veor
