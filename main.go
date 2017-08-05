package main

import (
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"strconv"
	"strings"

	"github.com/chzyer/readline"
)

var ctx *context

func listFiles(path string) func(string) []string {
	return func(line string) []string {
		files, _ := ioutil.ReadDir(path)
		names := make([]string, 0, len(files))
		for _, f := range files {
			names = append(names, f.Name())
		}
		return names
	}
}

var completer = readline.NewPrefixCompleter(
	readline.PcItem("help"),
	readline.PcItem("exit"),

	readline.PcItem("load", readline.PcItemDynamic(listFiles("./"))),
	readline.PcItem("list"),
	readline.PcItem("run"),
	readline.PcItem("step"),
	readline.PcItem("next"),
	readline.PcItem("breakpoint"),
	readline.PcItem("do"),
)

var commands = map[string]func([]string) error{
	"help":       cmdHelp,
	"load":       cmdLoad,
	"list":       cmdList,
	"do":         cmdDo,
	"run":        cmdRun,
	"step":       cmdNotImplementedFn("step"),
	"next":       cmdNotImplementedFn("next"),
	"breakpoint": cmdNotImplementedFn("breakpoint"),
}

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
func cmdLoad(args []string) error {
	if len(args) == 0 {
		return errors.New("no files specified")
	}
	if ctx != nil {
		ctx.close()
	}

	var err error

	source := args[0]
	ctx, err = newContext("/bin/sh", source, os.Stdout, os.Stderr)
	if err != nil {
		return err
	}
	return nil
}

func cmdList(args []string) error {
	if ctx == nil {
		return errors.New("file not loaded")
	}
	if len(args) == 0 {
		return ctx.listContinuously(10, os.Stderr)
	}
	n, err := strconv.Atoi(args[0])
	if err == nil {
		return ctx.listByNum(n, 10, os.Stderr)
	} else {
		return ctx.listByKeyword(args[0], 10, os.Stderr)
	}
}

func cmdDo(args []string) error {
	return ctx.do(args)
}

func cmdRun(args []string) error {
	return ctx.run()
}

func cmdNotImplementedFn(name string) func([]string) error {
	return func([]string) error {
		return errors.New("command not implemented: " + name)
	}
}

func readlineLoop(rl *readline.Instance) {
	for {
		line, err := rl.Readline()
		if err == readline.ErrInterrupt {
			continue
		} else if err == io.EOF {
			break
		}

		fields := strings.Fields(line)
		if len(fields) == 0 {
			continue
		}
		if fields[0] == "exit" {
			break
		}

		cmd, ok := commands[fields[0]]
		if ok {
			err = cmd(fields[1:])
		} else {
			err = errors.New("unknown command: " + fields[0] + "")
		}
		if err != nil {
			fmt.Fprintln(rl.Stderr(), err.Error())
		}
	}

}

func run() int {
	var err error
	if len(os.Args) > 1 {
		ctx, err = newContext("/bin/sh", os.Args[1], os.Stderr, os.Stdout)
	}

	rl, err := readline.NewEx(&readline.Config{
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
	defer rl.Close()

	readlineLoop(rl)

	return 0
}

func main() {
	os.Exit(run())
}
