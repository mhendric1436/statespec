#pragma once

#include "../schema_compatibility.hpp"
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
        const auto existing = state_->collections.find(descriptor.name);
        if (existing != state_->collections.end())
        {
            validate_collection_descriptor_upgrade(existing->second, descriptor);
        }
        state_->collections.insert_or_assign(descriptor.name, descriptor);
        state_->indexes.insert_or_assign(descriptor.name, detail::empty_index_states(descriptor));
    }

    void ensure_collections(const std::vector<CollectionDescriptor>& descriptors) override
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        auto staged = state_->collections;
        for (const auto& descriptor : descriptors)
        {
            const auto existing = staged.find(descriptor.name);
            if (existing != staged.end())
            {
                validate_collection_descriptor_upgrade(existing->second, descriptor);
            }
            staged.insert_or_assign(descriptor.name, descriptor);
        }
        auto staged_indexes = state_->indexes;
        for (const auto& descriptor : descriptors)
        {
            staged_indexes.insert_or_assign(
                descriptor.name, detail::empty_index_states(descriptor)
            );
        }
        state_->collections = std::move(staged);
        state_->indexes = std::move(staged_indexes);
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
