pub mod backend;
pub mod external_system;
pub mod feature_flag;
pub mod json;
pub mod lease;
pub mod log;
pub mod metric;
pub mod queue;
pub mod workflow;

pub mod memory {
    pub mod backend;
    pub mod feature_flags;
    pub mod leases;
    pub mod logs;
    pub mod metrics;
    pub mod queues;
    pub mod transaction;
    pub mod workflows;
}

pub mod memory_backend {
    pub use crate::memory::backend::*;
}
pub mod memory_feature_flags {
    pub use crate::memory::feature_flags::*;
}
pub mod memory_leases {
    pub use crate::memory::leases::*;
}
pub mod memory_logs {
    pub use crate::memory::logs::*;
}
pub mod memory_metrics {
    pub use crate::memory::metrics::*;
}
pub mod memory_queues {
    pub use crate::memory::queues::*;
}
pub mod memory_transaction {
    pub use crate::memory::transaction::*;
}
pub mod memory_workflows {
    pub use crate::memory::workflows::*;
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
