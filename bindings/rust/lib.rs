pub mod backend;
pub mod external_system;
pub mod feature_flag;
pub mod json;
pub mod lease;
pub mod log;
pub mod metric;
pub mod queue;
pub mod schema_compatibility;
pub mod workflow;

pub mod memory {
    pub mod backend;
    pub mod transaction;
}

pub mod runtime {
    pub mod codec;
    pub mod feature_flags;
    pub mod leases;
    pub mod logs;
    pub mod metrics;
    pub mod queues;
    pub mod workflows;
}

pub mod memory_backend {
    pub use crate::memory::backend::*;
}
pub(crate) mod runtime_codec {
    pub(crate) use crate::runtime::codec::*;
}
pub mod runtime_feature_flags {
    pub use crate::runtime::feature_flags::*;
}
pub mod runtime_leases {
    pub use crate::runtime::leases::*;
}
pub mod runtime_logs {
    pub use crate::runtime::logs::*;
}
pub mod runtime_metrics {
    pub use crate::runtime::metrics::*;
}
pub mod runtime_queues {
    pub use crate::runtime::queues::*;
}
pub mod memory_transaction {
    pub use crate::memory::transaction::*;
}
pub mod runtime_workflows {
    pub use crate::runtime::workflows::*;
}

#[cfg(test)]
mod feature_flag_tests;

#[cfg(test)]
mod external_system_tests;

#[cfg(test)]
mod json_tests;

#[cfg(test)]
mod log_tests;

#[cfg(test)]
mod metric_tests;

#[cfg(test)]
mod schema_compatibility_tests;

#[cfg(test)]
mod memory_backend_registration_tests;
