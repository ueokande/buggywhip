package main

import (
	"bytes"
	"io/ioutil"
	"os"
	"testing"
)

func TestRun(t *testing.T) {
	f, err := ioutil.TempFile("", "bgw-testscript")
	if err != nil {
		t.Fatal(err)
	}
	f.WriteString("hello world\n")
	f.WriteString("good morning\n")
	f.Close()
	defer os.Remove(f.Name())

	stdout := new(bytes.Buffer)
	stderr := new(bytes.Buffer)
	c, err := newContext("/bin/cat", f.Name(), stdout, stderr)
	if err != nil {
		t.Fatal(err)
	}

	err = c.run()
	if err != nil {
		t.Fatal(err)
	}
	c.close()

	if lines := string(stdout.Bytes()); lines != "hello world\ngood morning\n" {
		t.Fatalf("Unexpected lines: %s", lines)
	}
}
