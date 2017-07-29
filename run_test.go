package main

import (
	"io/ioutil"
	"os"
	"testing"
)

func TestRun(t *testing.T) {
	var buffer string
	w := func(line string) {
		buffer += line
	}

	f, err := ioutil.TempFile("", "bgw")
	if err != nil {
		t.Fatal(err)
	}
	f.WriteString("hello world\n")
	f.WriteString("good morning\n")
	f.Close()
	defer os.Remove(f.Name())

	c := newRunContext(f.Name(), w)
	err = c.run([]string{})

	if err != nil {
		t.Fatal(err)
	}
	if buffer != "hello world\ngood morning\n" {
		t.Fatalf("Unexpected line: %s", buffer)
	}
}
