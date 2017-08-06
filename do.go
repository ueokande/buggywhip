package main

import (
	"strings"
)

func (c *context) do(args []string) error {
	if len(args) == 0 {
		return nil
	}
	c.shell.Write([]byte(strings.Join(args, " ") + "\n"))
	return nil
}
