package main

import (
	"testing"
)

func TestDo(t *testing.T) {
	var buffer string
	w := func(line string) {
		buffer += line
	}

	c := newDoContext(w)

	lines := [][]string{
		{},
		{"hello", "world"},
		{"good", "morning"},
	}

	for _, line := range lines {
		err := c.run(line)
		if err != nil {
			t.Fatal(err)
		}
	}
	if buffer != "hello world\ngood morning\n" {
		t.Fatalf("Unexpected line: %s", buffer)
	}
}
