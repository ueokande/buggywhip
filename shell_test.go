package main

import (
	"bufio"
	"bytes"
	"testing"
)

func TestNewShell(t *testing.T) {
	var stdout bytes.Buffer
	var stderr bytes.Buffer

	cat, err := newShell(`/bin/sh`, bufio.NewWriter(&stdout), bufio.NewWriter(&stderr))
	if err != nil {
		t.Fatalf("Unexpected error: %v", err)
	}

	cat.Write([]byte("echo >&1 Hello World!\n"))
	cat.Write([]byte("echo >&2 Good morning!\n"))
	cat.Close()

	if o := string(stdout.Bytes()); o != "Hello World!\n" {
		t.Fatalf("Unexpected stdout: %s", o)
	}
	if o := string(stderr.Bytes()); o != "Good morning!\n" {
		t.Fatalf("Unexpected stdout: %s", o)
	}
}
