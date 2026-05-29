#include "generator_cpp_artifact_makefile.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_support.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{

namespace
{

TemplateRenderer::Values cpp_common_runtime_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream includes;
    auto add = [&](std::string_view include) { includes << include_line(include); };
    add("common/schema_compatibility.hpp");
    if (usage.uses_any_runtime_domain)
    {
        add("common/runtime/codec.hpp");
    }
    if (usage.uses_feature_flags)
    {
        add("common/runtime/feature_flag_store.hpp");
    }
    if (usage.uses_queues)
    {
        add("common/runtime/queue_store.hpp");
    }
    if (usage.uses_leases)
    {
        add("common/runtime/lease_store.hpp");
    }
    if (usage.uses_workflows)
    {
        add("common/runtime/workflow_store.hpp");
    }
    if (usage.uses_logs)
    {
        add("common/runtime/log_sink.hpp");
    }
    if (usage.uses_metrics)
    {
        add("common/runtime/metric_sink.hpp");
    }
    if (usage.uses_entity_gc)
    {
        add("common/runtime/entity_gc_types.hpp");
        add("common/runtime/entity_gc_repository.hpp");
        add("common/runtime/entity_gc_workers.hpp");
        add("common/runtime/entity_gc_registration.hpp");
    }
    return TemplateRenderer::Values{{"common_runtime_includes", includes.str()}};
}

} // namespace

TemplateRenderer::Values cpp_makefile_values(
    BindingGenerationTier tier,
    const IrSystem& system
)
{
    const auto include_api =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api;
    const auto usage = runtime_domain_usage(system);
    const auto include_worker =
        (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker) &&
        (!system.workers.empty() || usage.uses_workflows);
    const auto include_api_composition = include_api && !system.api_servers.empty();
    const auto include_worker_composition = include_worker && !system.workers.empty();
    const auto include_worker_execution =
        include_worker && (include_worker_composition || usage.uses_workflows);

    std::ostringstream target_additions;
    std::ostringstream phony_targets;
    std::ostringstream help_target_additions;
    std::ostringstream api_rules;
    std::ostringstream worker_rules;
    if (include_api)
    {
        target_additions << "\nCHECK_TARGETS += check-api";
        target_additions << "\nBUILD_TARGETS += build-api";
        target_additions << "\nPACKAGE_TARGETS += package-api";
        phony_targets << " check-api build-api package-api";
        help_target_additions << "\t@printf '%s\\n' '  check-api     build-api     package-api'\n";
        api_rules << "check-api: $(BUILD_DIR)/.dir\n";
        api_rules << "\tprintf '#include \"api/api_descriptors.hpp\"\\n"
                     "#include \"api/api_codecs.hpp\"\\n"
                     "#include \"api/api_handler_registry.hpp\"\\n"
                     "#include \"api/api_handlers.hpp\"\\n"
                     "#include \"api/external_system_operator_metadata_api.hpp\"\\n";
        if (include_api_composition)
        {
            api_rules << "#include \"api/api_application.hpp\"\\n"
                         "#include \"api/api_dispatcher.hpp\"\\n"
                         "#include \"api/api_process.hpp\"\\n"
                         "#include \"api/api_routes.hpp\"\\n"
                         "#include \"api/api_server.hpp\"\\n";
            api_rules << "#include \"api/api_transport.hpp\"\\n";
        }
        api_rules << "int main() { return 0; }\\n' | "
                     "$(CXX) $(CXXFLAGS) -x c++ - -o $(BUILD_DIR)/check-api\n\n";
        api_rules << "build-api: check-api\n";
        if (include_api_composition)
        {
            api_rules << "\t$(CXX) $(CXXFLAGS) api/main.cpp -o $(BUILD_DIR)/api-main\n\n";
        }
        api_rules << "package-api: build-api $(DIST_DIR)\n";
        api_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-api-cpp.tgz common api "
                     "Makefile\n\n";
    }
    if (include_worker)
    {
        target_additions << "\nCHECK_TARGETS += check-worker";
        target_additions << "\nBUILD_TARGETS += build-worker";
        target_additions << "\nPACKAGE_TARGETS += package-worker";
        phony_targets << " check-worker build-worker package-worker";
        help_target_additions
            << "\t@printf '%s\\n' '  check-worker  build-worker  package-worker'\n";
        worker_rules << "check-worker: $(BUILD_DIR)/.dir\n";
        worker_rules << "\tprintf '#include \"worker/worker_contexts.hpp\"\\n"
                        "#include \"worker/worker_descriptors.hpp\"\\n"
                        "#include \"worker/worker_registry.hpp\"\\n";
        if (include_worker_composition)
        {
            worker_rules << "#include \"worker/worker_application.hpp\"\\n"
                            "#include \"worker/worker_process.hpp\"\\n"
                            "#include \"worker/worker_runtime.hpp\"\\n";
        }
        if (usage.uses_leases)
        {
            worker_rules << "#include \"worker/worker_leases.hpp\"\\n";
        }
        if (usage.uses_queues)
        {
            worker_rules << "#include \"worker/worker_queues.hpp\"\\n";
        }
        if (usage.uses_workflows)
        {
            worker_rules << "#include \"worker/worker_workflows.hpp\"\\n";
        }
        if (include_worker_execution)
        {
            worker_rules << "#include \"worker/workflow_runner.hpp\"\\n"
                            "#include \"worker/workflow_step_handlers.hpp\"\\n";
        }
        worker_rules << "int main() { return 0; }\\n' | "
                        "$(CXX) $(CXXFLAGS) -x c++ - -o $(BUILD_DIR)/check-worker\n\n";
        worker_rules << "build-worker: check-worker\n";
        if (include_worker_composition)
        {
            worker_rules << "\t$(CXX) $(CXXFLAGS) worker/main.cpp -o $(BUILD_DIR)/worker-main\n\n";
        }
        worker_rules << "package-worker: build-worker $(DIST_DIR)\n";
        worker_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-worker-cpp.tgz common worker "
                        "Makefile\n\n";
    }
    return TemplateRenderer::Values{
        {"target_additions", target_additions.str()},
        {"phony_targets", phony_targets.str()},
        {"help_target_additions", help_target_additions.str()},
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
        {"common_runtime_includes", cpp_common_runtime_values(system)["common_runtime_includes"]},
    };
}

} // namespace statespec
