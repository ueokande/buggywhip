package main

import (
	"bytes"
	"testing"
)

func TestDo(t *testing.T) {
	stdout := new(bytes.Buffer)
	stderr := new(bytes.Buffer)

	c, err := newContext("/bin/cat", "dummy", stdout, stderr)
	if err != nil {
		t.Fatal("Unexpected error", err)
	}

	lines := [][]string{
		{},
		{"hello", "world"},
		{"good", "morning"},
	}

	for _, line := range lines {
		err := c.do(line)
		if err != nil {
			t.Fatal(err)
		}
	}
	c.close()
	if lines := string(stdout.Bytes()); lines != "hello world\ngood morning\n" {
		t.Fatalf("Unexpected lines: %s", lines)
	}
}
