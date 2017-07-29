package main

import (
	"strings"
)

type doContext struct {
	w func(string)
}

func newDoContext(writer func(string)) (*doContext, error) {
	return &doContext{
		w: writer,
	}, nil
}

func (c *doContext) run(args []string) error {
	if len(args) == 0 {
		return nil
	}
	c.w(strings.Join(args, " ") + "\n")
	return nil
}
