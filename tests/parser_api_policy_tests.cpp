#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"

namespace
{

void parser_parses_api_and_policy()
{
    const auto spec = statespec::test::parse_text(R"sspec(
        system OrderSystem {
          api StartOrderProcessing {
            method POST
            path "/v1/orders/{orderId}/start"
            input StartOrderRequest
            output StartOrderResponse
            error ProblemDetails
            starts workflow OrderProcessing
            enqueues EmailQueue.SendConfirmation
          }
          api_server OrderApi {
            serves StartOrderProcessing
            serves Admin.ListOrders
            concurrency 16
          }
          policy WorkflowAccess {
            tenant scoped_by tenant_id
            allow StartOrderProcessing when caller.role == operator;
            deny StartOrderProcessing when caller.suspended == true;
            quota starts_per_minute: 60;
            audit StartOrderProcessing;
          }
        }
    )sspec");

    statespec::test::require(spec.system->apis.size() == 1, "parser should parse one API");
    const auto& api = spec.system->apis[0];
    statespec::test::require(api.name == "StartOrderProcessing", "parser should parse API name");
    statespec::test::require(api.method == "POST", "parser should parse API method");
    statespec::test::require(
        api.path == "/v1/orders/{orderId}/start", "parser should parse API path"
    );
    statespec::test::require(api.input == "StartOrderRequest", "parser should parse API input");
    statespec::test::require(api.output == "StartOrderResponse", "parser should parse API output");
    statespec::test::require(api.error == "ProblemDetails", "parser should parse API error");
    statespec::test::require(
        api.starts_workflow == "OrderProcessing", "parser should parse started workflow"
    );
    statespec::test::require(
        api.enqueues == "EmailQueue.SendConfirmation", "parser should parse enqueued message"
    );

    statespec::test::require(
        spec.system->api_servers.size() == 1, "parser should parse one API server"
    );
    const auto& api_server = spec.system->api_servers[0];
    statespec::test::require(api_server.name == "OrderApi", "parser should parse API server name");
    statespec::test::require(api_server.serves.size() == 2, "parser should parse served APIs");
    statespec::test::require(
        api_server.serves[0] == "StartOrderProcessing", "parser should parse first served API"
    );
    statespec::test::require(
        api_server.serves[1] == "Admin.ListOrders", "parser should parse qualified served API"
    );
    statespec::test::require(
        api_server.concurrency == 16, "parser should parse API server concurrency"
    );

    statespec::test::require(spec.system->policies.size() == 1, "parser should parse one policy");
    const auto& policy = spec.system->policies[0];
    statespec::test::require(policy.name == "WorkflowAccess", "parser should parse policy name");
    statespec::test::require(
        policy.tenant_scoped_by == "tenant_id", "parser should parse tenant scope"
    );
    statespec::test::require(policy.allows.size() == 1, "parser should parse allow rule");
    statespec::test::require(policy.denies.size() == 1, "parser should parse deny rule");
    statespec::test::require(policy.quotas.size() == 1, "parser should parse quota");
    statespec::test::require(policy.audits.size() == 1, "parser should parse audit");
}
} // namespace

TEST_CASE("parser parses APIs and policies")
{
    parser_parses_api_and_policy();
}
