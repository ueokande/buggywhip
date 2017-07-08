package main

import (
	"fmt"
	"io"
	"os"
	"strings"

	"github.com/chzyer/readline"
)

var completer = readline.NewPrefixCompleter(
	readline.PcItem("help"),
	readline.PcItem("exit"),

	readline.PcItem("list"),
	readline.PcItem("run"),
	readline.PcItem("step"),
	readline.PcItem("next"),
	readline.PcItem("breakpoint"),
	readline.PcItem("do"),
)

func cmdHelp(w io.Writer) (int, error) {
	return io.WriteString(w, `List of classes of commands:

help -- Print list of commands
exit -- Exit buggywhip
list -- List source from specified line of keyword
run -- Start debugged script
step -- Step program line by line
next -- Step program until it reaches a breakpoint
breakpoint -- Manage breakpoints
`)
}

func run() int {
	l, err := readline.NewEx(&readline.Config{
		Prompt:          "(bgw) ",
		AutoComplete:    completer,
		InterruptPrompt: "^C",
		EOFPrompt:       "exit",

		HistorySearchFold: true,
	})
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return 1
	}
	defer l.Close()

readline_loop:
	for {
		line, err := l.Readline()
		if err == readline.ErrInterrupt {
			continue
		} else if err == io.EOF {
			break
		}

		fields := strings.Fields(line)
		if len(fields) == 0 {
			continue
		}

		switch fields[0] {
		case "help":
			cmdHelp(l.Stderr())
		case "list":
			io.WriteString(l.Stderr(), "list command not implemented yet\n")
		case "run":
			io.WriteString(l.Stderr(), "run command not implemented yet\n")
		case "step":
			io.WriteString(l.Stderr(), "step command not implemented yet\n")
		case "next":
			io.WriteString(l.Stderr(), "next command not implemented yet\n")
		case "breakpoint":
			io.WriteString(l.Stderr(), "breakpoint command not implemented yet\n")
		case "exit":
			break readline_loop
		default:
			io.WriteString(l.Stderr(), "Unknown command: "+fields[0]+"\n")
		}
	}

	return 0
}

func main() {
	os.Exit(run())
}
