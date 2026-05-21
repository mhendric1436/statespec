#pragma once

#include "statespec/generator_bindings.hpp"

#include <string>

namespace statespec
{

std::string generate_rust_descriptor_prelude(const IrSystem& system);
std::string generate_rust_feature_flag_descriptors(const IrSystem& system);
std::string generate_rust_declaration_descriptors(const IrSystem& system);
std::string generate_rust_external_system_descriptors(const IrSystem& system);
std::string generate_rust_api_descriptors(const IrSystem& system);
std::string generate_rust_worker_descriptors(const IrSystem& system);
std::string generate_rust_policy_descriptors(const IrSystem& system);
std::string generate_rust_shape_descriptors(const IrSystem& system);
std::string generate_rust_observability_descriptors(const IrSystem& system);
std::string generate_rust_entity_descriptors(const IrSystem& system);
std::string generate_rust_runtime_descriptors(const IrSystem& system);
std::string generate_rust_observability_registration(const IrSystem& system);
std::string generate_rust_runtime_registration(const IrSystem& system);

} // namespace statespec
