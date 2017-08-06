package main

import "io"

type context struct {
	source string

	shell *shell

	listWidth int
	listLast  int
}

func newContext(shell string, name string, stdout io.Writer, stderr io.Writer) (*context, error) {
	var err error
	var c context

	c.source = name

	c.listWidth, err = fileLineNoWidth(name)
	if err != nil {
		return nil, err
	}

	c.shell, err = newShell(shell, stdout, stderr)
	if err != nil {
		return nil, err
	}
	return &c, nil
}

func (c *context) close() {
	c.shell.Close()
}
