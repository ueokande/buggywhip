package main

import (
	"bufio"
	"errors"
	"os"
)

type runContext struct {
	source string
	w      func(string)
}

func newRunContext(source string, writer func(string)) *runContext {
	return &runContext{
		source: source,
		w:      writer,
	}
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
