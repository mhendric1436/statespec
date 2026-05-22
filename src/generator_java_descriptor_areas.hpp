#pragma once

#include "statespec/generator_bindings.hpp"

#include <string>

namespace statespec
{

std::string generate_java_descriptor_prelude(
    const IrSystem& system,
    const std::string& external_system_runtime,
    const std::string& external_system_metadata_runtime,
    const std::string& entity_repository_runtime
);
std::string generate_java_feature_flag_descriptors(const IrSystem& system);
std::string generate_java_declaration_descriptors(const IrSystem& system);
std::string generate_java_external_system_descriptors(
    const IrSystem& system,
    const std::string& external_system_call_adapters
);
std::string generate_java_api_descriptors(const IrSystem& system);
std::string generate_java_worker_descriptors(const IrSystem& system);
std::string generate_java_policy_descriptors(const IrSystem& system);
std::string generate_java_shape_descriptors(const IrSystem& system);
std::string generate_java_observability_descriptors(const IrSystem& system);
std::string generate_java_entity_descriptors(const IrSystem& system);
std::string generate_java_runtime_descriptors(const IrSystem& system);
std::string generate_java_observability_registration(const IrSystem& system);
std::string generate_java_runtime_registration(const IrSystem& system);

} // namespace statespec
