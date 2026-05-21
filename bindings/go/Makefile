.PHONY: test format clean help

test:
	go test ./...

format:
	gofmt -w backend/*.go backend/memory/*.go.tmpl backend/runtime/*.go.tmpl

clean:
	go clean -testcache

help:
	@echo "StateSpec Go binding targets:"
	@echo "  make test   Run Go binding tests"
	@echo "  make format Format Go binding source files"
	@echo "  make clean  Clean Go test cache"
