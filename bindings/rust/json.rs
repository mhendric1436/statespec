use std::collections::BTreeMap;
use std::fmt::Write;

#[derive(Clone, Debug, PartialEq)]
pub enum Json {
    Null,
    Bool(bool),
    Integer(i64),
    Decimal(f64),
    String(String),
    Array(Vec<Json>),
    Object(BTreeMap<String, Json>),
}

impl Json {
    pub fn parse(input: &str) -> Result<Self, JsonError> {
        Parser::new(input).parse()
    }

    pub fn canonical_string(&self) -> String {
        let mut out = String::new();
        self.append_canonical(&mut out);
        out
    }

    pub fn find(&self, key: &str) -> Option<&Json> {
        match self {
            Json::Object(values) => values.get(key),
            _ => None,
        }
    }

    fn append_canonical(&self, out: &mut String) {
        match self {
            Json::Null => out.push_str("null"),
            Json::Bool(value) => out.push_str(if *value { "true" } else { "false" }),
            Json::Integer(value) => {
                let _ = write!(out, "{}", value);
            }
            Json::Decimal(value) => {
                let mut encoded = value.to_string();
                if !encoded.contains('.') && !encoded.contains('e') && !encoded.contains('E') {
                    encoded.push_str(".0");
                }
                out.push_str(&encoded);
            }
            Json::String(value) => append_escaped_string(out, value),
            Json::Array(values) => {
                out.push('[');
                for (index, value) in values.iter().enumerate() {
                    if index > 0 {
                        out.push(',');
                    }
                    value.append_canonical(out);
                }
                out.push(']');
            }
            Json::Object(values) => {
                out.push('{');
                for (index, (key, value)) in values.iter().enumerate() {
                    if index > 0 {
                        out.push(',');
                    }
                    append_escaped_string(out, key);
                    out.push(':');
                    value.append_canonical(out);
                }
                out.push('}');
            }
        }
    }
}

#[derive(Debug, Clone)]
pub struct JsonError {
    pub message: String,
}

impl JsonError {
    fn new(message: impl Into<String>) -> Self {
        Self {
            message: message.into(),
        }
    }
}

fn append_escaped_string(out: &mut String, value: &str) {
    out.push('"');
    for c in value.chars() {
        match c {
            '"' => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            '\u{08}' => out.push_str("\\b"),
            '\u{0C}' => out.push_str("\\f"),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            c if c < '\u{20}' => {
                let _ = write!(out, "\\u{:04x}", c as u32);
            }
            c => out.push(c),
        }
    }
    out.push('"');
}

struct Parser<'a> {
    input: &'a str,
    position: usize,
}

impl<'a> Parser<'a> {
    fn new(input: &'a str) -> Self {
        Self { input, position: 0 }
    }

    fn parse(mut self) -> Result<Json, JsonError> {
        let value = self.parse_value()?;
        self.skip_whitespace();
        if self.position != self.input.len() {
            return Err(JsonError::new("unexpected trailing JSON content"));
        }
        Ok(value)
    }

    fn parse_value(&mut self) -> Result<Json, JsonError> {
        self.skip_whitespace();
        if self.starts_with("null") {
            self.position += 4;
            return Ok(Json::Null);
        }
        if self.starts_with("true") {
            self.position += 4;
            return Ok(Json::Bool(true));
        }
        if self.starts_with("false") {
            self.position += 5;
            return Ok(Json::Bool(false));
        }

        match self.peek_char() {
            Some('"') => Ok(Json::String(self.parse_string()?)),
            Some('[') => self.parse_array(),
            Some('{') => self.parse_object(),
            Some(_) => self.parse_number(),
            None => Err(JsonError::new("missing JSON value")),
        }
    }

    fn parse_array(&mut self) -> Result<Json, JsonError> {
        self.expect('[')?;
        let mut values = Vec::new();
        if self.consume(']') {
            return Ok(Json::Array(values));
        }
        loop {
            values.push(self.parse_value()?);
            if self.consume(']') {
                return Ok(Json::Array(values));
            }
            self.expect(',')?;
        }
    }

    fn parse_object(&mut self) -> Result<Json, JsonError> {
        self.expect('{')?;
        let mut values = BTreeMap::new();
        if self.consume('}') {
            return Ok(Json::Object(values));
        }
        loop {
            self.skip_whitespace();
            if self.peek_char() != Some('"') {
                return Err(JsonError::new("JSON object member name must be a string"));
            }
            let key = self.parse_string()?;
            self.expect(':')?;
            if values.contains_key(&key) {
                return Err(JsonError::new("duplicate JSON object member"));
            }
            values.insert(key, self.parse_value()?);
            if self.consume('}') {
                return Ok(Json::Object(values));
            }
            self.expect(',')?;
        }
    }

    fn parse_number(&mut self) -> Result<Json, JsonError> {
        let start = self.position;
        if self.peek_char() == Some('-') {
            self.position += 1;
        }
        match self.peek_char() {
            Some('0') => {
                self.position += 1;
                if matches!(self.peek_char(), Some('0'..='9')) {
                    return Err(JsonError::new("invalid JSON number leading zero"));
                }
            }
            Some('1'..='9') => {
                while matches!(self.peek_char(), Some('0'..='9')) {
                    self.position += 1;
                }
            }
            _ => return Err(JsonError::new("invalid JSON number")),
        }

        let mut decimal = false;
        if self.peek_char() == Some('.') {
            decimal = true;
            self.position += 1;
            if !matches!(self.peek_char(), Some('0'..='9')) {
                return Err(JsonError::new("invalid JSON number fraction"));
            }
            while matches!(self.peek_char(), Some('0'..='9')) {
                self.position += 1;
            }
        }

        if matches!(self.peek_char(), Some('e' | 'E')) {
            decimal = true;
            self.position += 1;
            if matches!(self.peek_char(), Some('+' | '-')) {
                self.position += 1;
            }
            if !matches!(self.peek_char(), Some('0'..='9')) {
                return Err(JsonError::new("invalid JSON number exponent"));
            }
            while matches!(self.peek_char(), Some('0'..='9')) {
                self.position += 1;
            }
        }

        let text = &self.input[start..self.position];
        if decimal {
            let value: f64 = text
                .parse()
                .map_err(|_| JsonError::new("invalid JSON number"))?;
            if !value.is_finite() {
                return Err(JsonError::new("invalid JSON number"));
            }
            return Ok(Json::Decimal(value));
        }

        let value: i64 = text
            .parse()
            .map_err(|_| JsonError::new("invalid JSON integer"))?;
        Ok(Json::Integer(value))
    }

    fn parse_string(&mut self) -> Result<String, JsonError> {
        self.expect('"')?;
        let mut out = String::new();
        while let Some(c) = self.next_char() {
            if c == '"' {
                return Ok(out);
            }
            if c < '\u{20}' {
                return Err(JsonError::new("unescaped control character in JSON string"));
            }
            if c != '\\' {
                out.push(c);
                continue;
            }
            let escaped = self
                .next_char()
                .ok_or_else(|| JsonError::new("unterminated JSON escape"))?;
            match escaped {
                '"' | '\\' | '/' => out.push(escaped),
                'b' => out.push('\u{08}'),
                'f' => out.push('\u{0C}'),
                'n' => out.push('\n'),
                'r' => out.push('\r'),
                't' => out.push('\t'),
                'u' => {
                    let code_point = self.parse_hex_quad()?;
                    let value = char::from_u32(code_point)
                        .ok_or_else(|| JsonError::new("invalid JSON unicode escape"))?;
                    out.push(value);
                }
                _ => return Err(JsonError::new("invalid JSON escape")),
            }
        }
        Err(JsonError::new("unterminated JSON string"))
    }

    fn parse_hex_quad(&mut self) -> Result<u32, JsonError> {
        let mut value = 0;
        for _ in 0..4 {
            let c = self
                .next_char()
                .ok_or_else(|| JsonError::new("short JSON unicode escape"))?;
            value = (value << 4)
                | match c {
                    '0'..='9' => c as u32 - '0' as u32,
                    'a'..='f' => c as u32 - 'a' as u32 + 10,
                    'A'..='F' => c as u32 - 'A' as u32 + 10,
                    _ => return Err(JsonError::new("invalid JSON unicode escape")),
                };
        }
        Ok(value)
    }

    fn consume(&mut self, expected: char) -> bool {
        self.skip_whitespace();
        if self.peek_char() == Some(expected) {
            self.position += expected.len_utf8();
            return true;
        }
        false
    }

    fn expect(&mut self, expected: char) -> Result<(), JsonError> {
        if self.consume(expected) {
            Ok(())
        } else {
            Err(JsonError::new("unexpected JSON token"))
        }
    }

    fn starts_with(&self, value: &str) -> bool {
        self.input[self.position..].starts_with(value)
    }

    fn peek_char(&self) -> Option<char> {
        self.input[self.position..].chars().next()
    }

    fn next_char(&mut self) -> Option<char> {
        let c = self.peek_char()?;
        self.position += c.len_utf8();
        Some(c)
    }

    fn skip_whitespace(&mut self) {
        while matches!(self.peek_char(), Some(' ' | '\n' | '\r' | '\t')) {
            self.position += 1;
        }
    }
}
