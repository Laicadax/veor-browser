#pragma once

#include "bookmarks/IBookmarkStore.h"
#include "core/base/VeorId.h"

namespace veor {

class BookmarkStoreStub : public IBookmarkStore {
 public:
  Result<BookmarkNodeId, std::string> AddBookmark(
      const GURL& url,
      const std::string& title,
      BookmarkNodeId parent_folder) override {
    return Result<BookmarkNodeId, std::string>::Ok(
        IdGenerator::NextId<BookmarkNodeTag>());
  }

  Result<BookmarkNodeId, std::string> AddFolder(
      const std::string& title,
      BookmarkNodeId parent_folder) override {
    return Result<BookmarkNodeId, std::string>::Ok(
        IdGenerator::NextId<BookmarkNodeTag>());
  }

  Result<void, std::string> UpdateNode(BookmarkNodeId id,
                                        const std::string& new_title,
                                        const GURL& new_url) override {
    return Result<void, std::string>::Ok();
  }

  Result<void, std::string> DeleteNode(BookmarkNodeId id) override {
    return Result<void, std::string>::Ok();
  }

  Result<void, std::string> MoveNode(BookmarkNodeId id,
                                     BookmarkNodeId new_parent,
                                     int new_index) override {
    return Result<void, std::string>::Ok();
  }

  Result<BookmarkNode, std::string> GetNode(BookmarkNodeId id) const override {
    return Result<BookmarkNode, std::string>::Err("Not found");
  }

  Result<std::vector<BookmarkNode>, std::string> GetChildren(
      BookmarkNodeId parent) const override {
    return Result<std::vector<BookmarkNode>, std::string>::Ok(
        std::vector<BookmarkNode>{});
  }

  Result<std::vector<BookmarkNode>, std::string> Search(
      const std::string& query) const override {
    return Result<std::vector<BookmarkNode>, std::string>::Ok(
        std::vector<BookmarkNode>{});
  }

  BookmarkNodeId GetRootId() const override {
    return BookmarkNodeId();
  }

  Result<std::vector<BookmarkNode>, std::string> GetTree() const override {
    return Result<std::vector<BookmarkNode>, std::string>::Ok(
        std::vector<BookmarkNode>{});
  }
};

}  // namespace veor
