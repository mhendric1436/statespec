#include "generator_backend.hpp"

namespace statespec::generator_backend
{

namespace
{

std::string generate_dl_manifest(const SystemDecl& system, const GenerateDecl& declaration)
{
    std::ostringstream out;
    append_header(out, system, declaration.target);
    out << "leases:\n";
    for (const auto& lease : system.leases)
    {
        out << "  - name: " << lease.name << "\n";
        out << "    resource: " << optional_or_empty(lease.resource) << "\n";
        out << "    ttl: " << optional_or_empty(lease.ttl) << "\n";
        out << "    renew_every: " << optional_or_empty(lease.renew_every) << "\n";
        out << "    holder: " << optional_or_empty(lease.holder) << "\n";
        out << "    fencing_token: " << optional_bool_or_empty(lease.fencing_token) << "\n";
        out << "    max_ttl: " << optional_or_empty(lease.max_ttl) << "\n";
    }
    out << "workers:\n";
    for (const auto& worker : system.workers)
    {
        out << "  - name: " << worker.name << "\n";
        out << "    lease: " << optional_or_empty(worker.lease) << "\n";
        out << "    singleton: " << optional_bool_or_empty(worker.singleton) << "\n";
        out << "    concurrency: " << worker.concurrency.value_or(0) << "\n";
    }
    return out.str();
}

std::string generate_dl_leases_header()
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include <cstddef>\n";
    out << "#include <string_view>\n\n";
    out << "namespace statespec_generated::dl\n";
    out << "{\n\n";
    out << "struct LeaseMetadata\n";
    out << "{\n";
    out << "    std::string_view name;\n";
    out << "    std::string_view resource;\n";
    out << "    std::string_view ttl;\n";
    out << "    std::string_view renew_every;\n";
    out << "    std::string_view holder;\n";
    out << "    bool fencing_token;\n";
    out << "    std::string_view max_ttl;\n";
    out << "};\n\n";
    out << "struct WorkerLeaseBinding\n";
    out << "{\n";
    out << "    std::string_view worker_name;\n";
    out << "    std::string_view lease_name;\n";
    out << "    bool singleton;\n";
    out << "    int concurrency;\n";
    out << "};\n\n";
    out << "const LeaseMetadata* leases();\n";
    out << "std::size_t lease_count();\n";
    out << "const WorkerLeaseBinding* worker_lease_bindings();\n";
    out << "std::size_t worker_lease_binding_count();\n";
    out << "const LeaseMetadata* find_lease(std::string_view name);\n\n";
    out << "} // namespace statespec_generated::dl\n";
    return out.str();
}

std::string generate_dl_metadata_source(const SystemDecl& system)
{
    std::ostringstream out;
    out << "#include \"dl_leases.hpp\"\n\n";
    out << "#include <array>\n\n";
    out << "namespace statespec_generated::dl\n";
    out << "{\n";
    out << "namespace\n";
    out << "{\n\n";
    out << "constexpr std::array<LeaseMetadata, " << system.leases.size() << "> all_leases{{\n";
    for (const auto& lease : system.leases)
    {
        out << "    LeaseMetadata{\"" << lease.name << "\", \"" << optional_or_empty(lease.resource)
            << "\", \"" << optional_or_empty(lease.ttl) << "\", \""
            << optional_or_empty(lease.renew_every) << "\", \"" << optional_or_empty(lease.holder)
            << "\", " << bool_text(lease.fencing_token.value_or(false)) << ", \""
            << optional_or_empty(lease.max_ttl) << "\"},\n";
    }
    out << "}};\n\n";
    out << "constexpr std::array<WorkerLeaseBinding, " << system.workers.size()
        << "> all_worker_lease_bindings{{\n";
    for (const auto& worker : system.workers)
    {
        out << "    WorkerLeaseBinding{\"" << worker.name << "\", \""
            << optional_or_empty(worker.lease) << "\", "
            << bool_text(worker.singleton.value_or(false)) << ", " << worker.concurrency.value_or(0)
            << "},\n";
    }
    out << "}};\n\n";
    out << "} // namespace\n\n";
    out << "const LeaseMetadata* leases()\n";
    out << "{\n";
    out << "    return all_leases.data();\n";
    out << "}\n\n";
    out << "std::size_t lease_count()\n";
    out << "{\n";
    out << "    return all_leases.size();\n";
    out << "}\n\n";
    out << "const WorkerLeaseBinding* worker_lease_bindings()\n";
    out << "{\n";
    out << "    return all_worker_lease_bindings.data();\n";
    out << "}\n\n";
    out << "std::size_t worker_lease_binding_count()\n";
    out << "{\n";
    out << "    return all_worker_lease_bindings.size();\n";
    out << "}\n\n";
    out << "const LeaseMetadata* find_lease(std::string_view name)\n";
    out << "{\n";
    out << "    for (std::size_t i = 0; i < all_leases.size(); ++i)\n";
    out << "    {\n";
    out << "        if (all_leases[i].name == name)\n";
    out << "        {\n";
    out << "            return &all_leases[i];\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return nullptr;\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::dl\n";
    return out.str();
}

} // namespace

void generate_dl(
    const SystemDecl& system,
    const GenerateDecl& declaration,
    const GenerationOptions& options,
    GenerationResult& result
)
{
    const auto root = output_root(declaration, options);
    result.files.push_back(
        GeneratedFile{join_path(root, "dl-manifest.yaml"), generate_dl_manifest(system, declaration)}
    );
    result.files.push_back(
        GeneratedFile{join_path(root, "dl_leases.hpp"), generate_dl_leases_header()}
    );
    result.files.push_back(
        GeneratedFile{join_path(root, "dl_metadata.cpp"), generate_dl_metadata_source(system)}
    );
}

} // namespace statespec::generator_backend
