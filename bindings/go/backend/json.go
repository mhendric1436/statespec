package backend

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"math"
	"sort"
	"strconv"
	"strings"
)

// JSON is the typed JSON value used by backend bindings. It replaces raw []byte
// JSON payloads so binding implementations can validate, inspect, and serialize
// documents deterministically.
type JSON struct {
	value any
}

func JSONNull() JSON           { return JSON{value: nil} }
func JSONBool(value bool) JSON { return JSON{value: value} }
func JSONInt(value int64) JSON { return JSON{value: value} }
func JSONFloat(value float64) (JSON, error) {
	if math.IsInf(value, 0) || math.IsNaN(value) {
		return JSON{}, errors.New("JSON number must be finite")
	}
	return JSON{value: value}, nil
}
func JSONString(value string) JSON           { return JSON{value: value} }
func JSONArray(values []JSON) JSON           { return JSON{value: values} }
func JSONObject(values map[string]JSON) JSON { return JSON{value: values} }

func ParseJSON(encoded string) (JSON, error) {
	decoder := json.NewDecoder(strings.NewReader(encoded))
	decoder.UseNumber()
	value, err := parseDecoderValue(decoder)
	if err != nil {
		return JSON{}, err
	}
	if _, err := decoder.Token(); err != io.EOF {
		if err != nil {
			return JSON{}, err
		}
		return JSON{}, errors.New("unexpected trailing JSON content")
	}
	return value, nil
}

func parseDecoderValue(decoder *json.Decoder) (JSON, error) {
	token, err := decoder.Token()
	if err != nil {
		return JSON{}, err
	}
	switch value := token.(type) {
	case nil:
		return JSONNull(), nil
	case bool:
		return JSONBool(value), nil
	case string:
		return JSONString(value), nil
	case json.Number:
		text := value.String()
		if strings.ContainsAny(text, ".eE") {
			f, err := strconv.ParseFloat(text, 64)
			if err != nil || math.IsInf(f, 0) || math.IsNaN(f) {
				return JSON{}, fmt.Errorf("invalid JSON number %q", text)
			}
			return JSON{value: f}, nil
		}
		i, err := strconv.ParseInt(text, 10, 64)
		if err != nil {
			return JSON{}, fmt.Errorf("invalid JSON integer %q", text)
		}
		return JSONInt(i), nil
	case json.Delim:
		switch value {
		case '[':
			var values []JSON
			for decoder.More() {
				item, err := parseDecoderValue(decoder)
				if err != nil {
					return JSON{}, err
				}
				values = append(values, item)
			}
			end, err := decoder.Token()
			if err != nil || end != json.Delim(']') {
				return JSON{}, errors.New("unterminated JSON array")
			}
			return JSONArray(values), nil
		case '{':
			values := map[string]JSON{}
			for decoder.More() {
				keyToken, err := decoder.Token()
				if err != nil {
					return JSON{}, err
				}
				key, ok := keyToken.(string)
				if !ok {
					return JSON{}, errors.New("JSON object member name must be a string")
				}
				if _, exists := values[key]; exists {
					return JSON{}, fmt.Errorf("duplicate JSON object member %q", key)
				}
				item, err := parseDecoderValue(decoder)
				if err != nil {
					return JSON{}, err
				}
				values[key] = item
			}
			end, err := decoder.Token()
			if err != nil || end != json.Delim('}') {
				return JSON{}, errors.New("unterminated JSON object")
			}
			return JSONObject(values), nil
		}
	}
	return JSON{}, errors.New("invalid JSON value")
}

func (j JSON) IsNull() bool         { return j.value == nil }
func (j JSON) AsBool() (bool, bool) { v, ok := j.value.(bool); return v, ok }
func (j JSON) AsInt() (int64, bool) { v, ok := j.value.(int64); return v, ok }
func (j JSON) AsFloat() (float64, bool) {
	switch v := j.value.(type) {
	case float64:
		return v, true
	case int64:
		return float64(v), true
	default:
		return 0, false
	}
}
func (j JSON) AsString() (string, bool)          { v, ok := j.value.(string); return v, ok }
func (j JSON) AsArray() ([]JSON, bool)           { v, ok := j.value.([]JSON); return v, ok }
func (j JSON) AsObject() (map[string]JSON, bool) { v, ok := j.value.(map[string]JSON); return v, ok }

func (j JSON) Find(key string) (JSON, bool) {
	object, ok := j.AsObject()
	if !ok {
		return JSON{}, false
	}
	value, ok := object[key]
	return value, ok
}

func (j JSON) CanonicalString() string {
	var out bytes.Buffer
	appendCanonical(&out, j)
	return out.String()
}

func (j JSON) MarshalJSON() ([]byte, error) { return []byte(j.CanonicalString()), nil }
func (j *JSON) UnmarshalJSON(data []byte) error {
	value, err := ParseJSON(string(data))
	if err != nil {
		return err
	}
	*j = value
	return nil
}

func appendCanonical(out *bytes.Buffer, value JSON) {
	switch v := value.value.(type) {
	case nil:
		out.WriteString("null")
	case bool:
		if v {
			out.WriteString("true")
		} else {
			out.WriteString("false")
		}
	case int64:
		out.WriteString(strconv.FormatInt(v, 10))
	case float64:
		encoded := strconv.FormatFloat(v, 'g', -1, 64)
		out.WriteString(encoded)
		if !strings.ContainsAny(encoded, ".eE") {
			out.WriteString(".0")
		}
	case string:
		encoded, _ := json.Marshal(v)
		out.Write(encoded)
	case []JSON:
		out.WriteByte('[')
		for i, item := range v {
			if i > 0 {
				out.WriteByte(',')
			}
			appendCanonical(out, item)
		}
		out.WriteByte(']')
	case map[string]JSON:
		keys := make([]string, 0, len(v))
		for key := range v {
			keys = append(keys, key)
		}
		sort.Strings(keys)
		out.WriteByte('{')
		for i, key := range keys {
			if i > 0 {
				out.WriteByte(',')
			}
			encoded, _ := json.Marshal(key)
			out.Write(encoded)
			out.WriteByte(':')
			appendCanonical(out, v[key])
		}
		out.WriteByte('}')
	}
}
