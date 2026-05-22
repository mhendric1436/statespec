use std::collections::BTreeMap;

use statespec_generated::api_handlers::ApiHandler;
use statespec_generated::api_server::{find_api_server, ApiServer};
use statespec_generated::backend::Backend;
use statespec_generated::descriptors::{ApiRequestContext, ApiResponse};
use statespec_generated::json::Json;
use statespec_generated::memory_backend::InMemoryBackend;

struct LinkingHandler {
    backend: InMemoryBackend,
}

impl ApiHandler for LinkingHandler {
    fn handle_start_provision(
        &self,
        context: &ApiRequestContext,
    ) -> statespec_generated::backend::BackendResult<ApiResponse> {
        self.record_request(context)
    }

    fn handle_report_provision_ready(
        &self,
        context: &ApiRequestContext,
    ) -> statespec_generated::backend::BackendResult<ApiResponse> {
        self.record_request(context)
    }
}

impl LinkingHandler {
    fn record_request(
        &self,
        context: &ApiRequestContext,
    ) -> statespec_generated::backend::BackendResult<ApiResponse> {
        let mut tx = self.backend.begin()?;
        self.backend.put(
            &mut tx,
            "api_requests",
            "request-1",
            Json::Object(BTreeMap::from([
                (
                    "server".to_string(),
                    Json::String(context.server_name.clone()),
                ),
                ("api".to_string(), Json::String(context.api_name.clone())),
            ])),
        )?;
        self.backend.commit(tx)?;

        Ok(ApiResponse {
            status_code: 202,
            body: Json::Object(BTreeMap::from([("linked".to_string(), Json::Bool(true))])),
        })
    }
}

#[test]
fn generated_api_server_links_with_memory_backend() {
    let descriptor = find_api_server("ProvisionApi").expect("ProvisionApi descriptor not found");
    let server = ApiServer::new(
        descriptor,
        LinkingHandler {
            backend: InMemoryBackend::new(),
        },
    );
    let response = server
        .handle(
            "ProvisionApi.StartProvision",
            &ApiRequestContext {
                server_name: "ProvisionApi".to_string(),
                api_name: "StartProvision".to_string(),
                method: Some("POST".to_string()),
                path: Some("/v1/provision".to_string()),
                body: Json::Object(BTreeMap::from([(
                    "tenant_id".to_string(),
                    Json::String("tenant-1".to_string()),
                )])),
            },
        )
        .expect("API dispatch failed")
        .expect("route not handled");
    assert_eq!(response.status_code, 202);
}
