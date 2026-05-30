#include "generator_go_artifact_makefile.hpp"

#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{

TemplateRenderer::Values go_makefile_values(
    BindingGenerationTier tier,
    const IrSystem& system
)
{
    const auto include_api =
        (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api) &&
        (!system.apis.empty() || !system.api_servers.empty());
    const auto usage = runtime_domain_usage(system);
    const auto include_worker =
        (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker) &&
        (!system.workers.empty() || usage.uses_workflows);
    const auto include_api_composition = include_api && !system.api_servers.empty();
    const auto include_worker_composition = include_worker && !system.workers.empty();

    std::ostringstream target_additions;
    std::ostringstream phony_targets;
    std::ostringstream help_target_additions;
    std::ostringstream api_package_additions;
    std::ostringstream worker_package_additions;
    std::ostringstream api_build_dependency;
    std::ostringstream worker_build_dependency;
    std::ostringstream api_build_command;
    std::ostringstream worker_build_command;
    std::ostringstream api_rules;
    std::ostringstream worker_rules;
    if (include_api_composition)
    {
        api_package_additions << " ./api/cmd/api";
        api_build_dependency << " $(BIN_DIR)";
        api_build_command << "\t$(GO) build -o $(BIN_DIR)/statespec-api ./api/cmd/api\n";
    }
    if (include_worker_composition)
    {
        worker_package_additions << " ./worker/cmd/worker";
        worker_build_dependency << " $(BIN_DIR)";
        worker_build_command
            << "\t$(GO) build -o $(BIN_DIR)/statespec-worker ./worker/cmd/worker\n";
    }
    if (include_api)
    {
        if (!include_api_composition)
        {
            api_build_command << "\t$(GO) build $(API_PACKAGES)\n";
        }
        target_additions << "\nCHECK_TARGETS += check-api";
        target_additions << "\nBUILD_TARGETS += build-api";
        target_additions << "\nPACKAGE_TARGETS += package-api";
        phony_targets << " check-api build-api package-api";
        help_target_additions << "\t@printf '%s\\n' '  check-api     build-api     package-api'\n";
        api_rules << "check-api:\n";
        api_rules << "\t$(GO) test $(API_PACKAGES)\n\n";
        api_rules << "build-api:" << api_build_dependency.str() << "\n";
        api_rules << "\t$(GO) build ./api/backend\n";
        api_rules << api_build_command.str();
        api_rules << "\n";
        api_rules << "package-api: build-api $(DIST_DIR)\n";
        api_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-api-go.tgz common api go.mod "
                     "Makefile\n\n";
    }
    if (include_worker)
    {
        if (!include_worker_composition)
        {
            worker_build_command << "\t$(GO) build $(WORKER_PACKAGES)\n";
        }
        target_additions << "\nCHECK_TARGETS += check-worker";
        target_additions << "\nBUILD_TARGETS += build-worker";
        target_additions << "\nPACKAGE_TARGETS += package-worker";
        phony_targets << " check-worker build-worker package-worker";
        help_target_additions
            << "\t@printf '%s\\n' '  check-worker  build-worker  package-worker'\n";
        worker_rules << "check-worker:\n";
        worker_rules << "\t$(GO) test $(WORKER_PACKAGES)\n\n";
        worker_rules << "build-worker:" << worker_build_dependency.str() << "\n";
        worker_rules << "\t$(GO) build ./worker/backend\n";
        worker_rules << worker_build_command.str();
        worker_rules << "\n";
        worker_rules << "package-worker: build-worker $(DIST_DIR)\n";
        worker_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-worker-go.tgz common worker "
                        "go.mod Makefile\n\n";
    }
    return TemplateRenderer::Values{
        {"target_additions", target_additions.str()},
        {"phony_targets", phony_targets.str()},
        {"help_target_additions", help_target_additions.str()},
        {"api_package_additions", api_package_additions.str()},
        {"worker_package_additions", worker_package_additions.str()},
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
    };
}

} // namespace statespec
