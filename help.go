package main

import (
	"io"
)

type helpContext struct {
	out io.Writer
}

func newHelpContext(out io.Writer) (*helpContext, error) {
	return &helpContext{
		out: out,
	}, nil
}

func (c *helpContext) run(args []string) error {
	_, err := io.WriteString(c.out, `List of classes of commands:

help -- Print list of commands
exit -- Exit buggywhip
load -- Load source
list -- List source from specified line of keyword
run -- Start debugged script
step -- Step program line by line
next -- Step program until it reaches a breakpoint
breakpoint -- Manage breakpoints
`)
	return err
}
