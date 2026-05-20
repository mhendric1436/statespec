#pragma once

#include "transaction.hpp"

namespace statespec::backend::memory
{

class InMemoryBackend : public IBackend
{
  public:
    BackendCapabilities capabilities() const override
    {
        BackendCapabilities capabilities;
        capabilities.prefix_query = true;
        return capabilities;
    }

    void ensure_collection(const CollectionDescriptor& descriptor) override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        state_->collections.insert_or_assign(descriptor.name, descriptor);
    }

    void ensure_collections(const std::vector<CollectionDescriptor>& descriptors) override
    {
        for (const auto& descriptor : descriptors)
        {
            ensure_collection(descriptor);
        }
    }

    std::unique_ptr<ITransaction> begin() override
    {
        return std::make_unique<InMemoryTransaction>(state_);
    }

    std::optional<VersionedRecord>
    get(ITransaction& tx,
        const CollectionName& collection,
        const Key& key) override
    {
        return as_memory_tx(tx).get(collection, key);
    }

    std::vector<VersionedRecord> query(
        ITransaction& tx,
        const CollectionName& collection,
        const Query& query
    ) override
    {
        return as_memory_tx(tx).query(collection, query);
    }

    void
    put(ITransaction& tx,
        const CollectionName& collection,
        const Key& key,
        Json document) override
    {
        as_memory_tx(tx).put(collection, key, std::move(document));
    }

    void erase(
        ITransaction& tx,
        const CollectionName& collection,
        const Key& key
    ) override
    {
        as_memory_tx(tx).erase(collection, key);
    }

    void commit(ITransaction& tx) override
    {
        as_memory_tx(tx).commit();
    }

  private:
    std::shared_ptr<detail::InMemoryState> state_ = std::make_shared<detail::InMemoryState>();
};

} // namespace statespec::backend::memory
