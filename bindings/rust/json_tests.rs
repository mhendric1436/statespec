#[cfg(test)]
mod tests {
    use crate::json::Json;
    use std::collections::BTreeMap;

    #[test]
    fn parses_objects_and_arrays() {
        let parsed = Json::parse(r#"{"active":true,"count":7,"tags":["a",null,2.5]}"#)
            .expect("parse should succeed");

        let object = match parsed {
            Json::Object(values) => values,
            other => panic!("expected object got {:?}", other),
        };

        match object.get("active") {
            Some(Json::Bool(true)) => {}
            other => panic!("unexpected active value: {:?}", other),
        }

        match object.get("count") {
            Some(Json::Integer(7)) => {}
            other => panic!("unexpected count value: {:?}", other),
        }

        match object.get("tags") {
            Some(Json::Array(values)) => {
                assert_eq!(values.len(), 3);
                match &values[2] {
                    Json::Decimal(value) => assert_eq!(*value, 2.5),
                    other => panic!("unexpected decimal value: {:?}", other),
                }
            }
            other => panic!("unexpected tags value: {:?}", other),
        }
    }

    #[test]
    fn canonical_string_is_stable() {
        let mut object = BTreeMap::new();
        object.insert("z".to_string(), Json::Integer(1));
        object.insert(
            "a".to_string(),
            Json::Array(vec![Json::Bool(true), Json::String("x".to_string())]),
        );

        let value = Json::Object(object);
        let canonical = value.canonical_string();

        assert_eq!(canonical, r#"{"a":[true,"x"],"z":1}"#);

        let reparsed = Json::parse(&canonical).expect("canonical parse should succeed");
        assert_eq!(reparsed, value);
    }

    #[test]
    fn rejects_malformed_input() {
        assert!(Json::parse("[1,]").is_err());
        assert!(Json::parse(r#"{"a":1,"a":2}"#).is_err());
        assert!(Json::parse("01").is_err());
        assert!(Json::parse("-01").is_err());
        assert!(Json::parse("\"raw\nnewline\"").is_err());
    }

    #[test]
    fn backend_surfaces_use_typed_json() {
        let query = crate::backend::Query::JsonEquals {
            path: "$.active".to_string(),
            value: Json::Bool(true),
        };

        match query {
            crate::backend::Query::JsonEquals { value: Json::Bool(true), .. } => {}
            other => panic!("unexpected query value: {:?}", other),
        }

        let index = crate::backend::IndexValue::String("alice@example.com".to_string());
        match index.json_value() {
            Json::String(value) => assert_eq!(value, "alice@example.com"),
            other => panic!("unexpected index json value: {:?}", other),
        }
    }
}
