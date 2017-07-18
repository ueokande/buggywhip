package main

import (
	"io"
	"os"
)

func cmdHelp(args []string) error {
	_, err := io.WriteString(os.Stderr, `List of classes of commands:

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
