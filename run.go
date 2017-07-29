package main

import (
	"bufio"
	"errors"
	"io"
	"os"
)

type runContext struct {
	source string
	stdout io.Writer
	stderr io.Writer
	w      func(string)
}

func newRunContext(source string, stdout io.Writer, stderr io.Writer, writer func(string)) (*runContext, error) {
	return &runContext{
		source: source,
		stdout: stdout,
		stderr: stderr,
		w:      writer,
	}, nil
}

func (c *runContext) run(args []string) error {
	if len(c.source) == 0 {
		return errors.New("file not loaded")
	}

	f, err := os.Open(c.source)
	if err != nil {
		return err
	}

	s := bufio.NewScanner(f)
	for s.Scan() {
		c.w(s.Text() + "\n")
	}
	return nil
}
