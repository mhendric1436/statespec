#include "generator_java_artifact_makefile.hpp"

#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{

TemplateRenderer::Values java_makefile_values(
    BindingGenerationTier tier,
    const IrSystem& system
)
{
    const auto usage = runtime_domain_usage(system);
    const auto include_api =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api;
    const auto include_worker =
        (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker) &&
        (!system.workers.empty() || usage.uses_workflows);

    std::ostringstream build_target_additions;
    std::ostringstream package_target_additions;
    std::ostringstream phony_targets;
    std::ostringstream help_target_additions;
    std::ostringstream api_rules;
    std::ostringstream worker_rules;
    if (include_api)
    {
        build_target_additions << " build-api";
        package_target_additions << " package-api";
        phony_targets << " build-api package-api";
        help_target_additions << "\t@printf '%s\\n' '  build-api     package-api'\n";
        api_rules << "build-api: $(BUILD_DIR)\n";
        api_rules << "\t$(JAVAC) -d $(BUILD_DIR) $(COMMON_SOURCES) $(API_SOURCES)\n\n";
        api_rules << "package-api: build-api $(DIST_DIR)\n";
        api_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-api-java.tgz common api "
                     "Makefile\n\n";
    }
    if (include_worker)
    {
        build_target_additions << " build-worker";
        package_target_additions << " package-worker";
        phony_targets << " build-worker package-worker";
        help_target_additions << "\t@printf '%s\\n' '  build-worker  package-worker'\n";
        worker_rules << "build-worker: $(BUILD_DIR)\n";
        worker_rules << "\t$(JAVAC) -d $(BUILD_DIR) $(COMMON_SOURCES) $(WORKER_SOURCES)\n\n";
        worker_rules << "package-worker: build-worker $(DIST_DIR)\n";
        worker_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-worker-java.tgz common worker "
                        "Makefile\n\n";
    }

    return TemplateRenderer::Values{
        {"build_target_additions", build_target_additions.str()},
        {"package_target_additions", package_target_additions.str()},
        {"phony_targets", phony_targets.str()},
        {"help_target_additions", help_target_additions.str()},
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
    };
}

} // namespace statespec
