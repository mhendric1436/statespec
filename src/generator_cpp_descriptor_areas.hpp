#pragma once

#include "statespec/generator_bindings.hpp"

#include <string>

namespace statespec
{

std::string generate_cpp_descriptor_prelude(
    const IrSystem& system,
    const std::string& external_system_runtime,
    const std::string& external_system_metadata_runtime
);
std::string generate_cpp_feature_flag_descriptors(const IrSystem& system);
std::string generate_cpp_declaration_descriptors(const IrSystem& system);
std::string generate_cpp_external_system_descriptors(
    const IrSystem& system,
    const std::string& external_system_call_adapters
);
std::string generate_cpp_api_descriptors(const IrSystem& system);
std::string generate_cpp_worker_descriptors(const IrSystem& system);
std::string generate_cpp_policy_descriptors(const IrSystem& system);
std::string generate_cpp_shape_descriptors(const IrSystem& system);
std::string generate_cpp_observability_descriptors(const IrSystem& system);
std::string generate_cpp_entity_descriptors(const IrSystem& system);
std::string generate_cpp_runtime_descriptors(const IrSystem& system);
std::string generate_cpp_observability_registration(const IrSystem& system);
std::string generate_cpp_runtime_registration(const IrSystem& system);

} // namespace statespec
