package main

import (
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"os/exec"
	"strings"

	"github.com/chzyer/readline"
)

var context struct {
	stderr io.Writer
	stdout io.Writer

	source string

	ch   chan string
	fifo string
	cmd  *exec.Cmd
}

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

func cmdHelp(w io.Writer) (int, error) {
	return io.WriteString(w, `List of classes of commands:

help -- Print list of commands
exit -- Exit buggywhip
load -- Load source
list -- List source from specified line of keyword
run -- Start debugged script
step -- Step program line by line
next -- Step program until it reaches a breakpoint
breakpoint -- Manage breakpoints
`)
}

func cmdLoad(args []string) error {
	if len(args) == 0 {
		return errors.New("no files specified")
	}
	source := args[0]
	_, err := os.Stat(source)
	if err != nil {
		return err
	}
	context.source = source

	return reloadSource()
}

func readlineLoop(rl *readline.Instance) {
readline_loop:
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

		switch cmd := fields[0]; cmd {
		case "help":
			cmdHelp(rl.Stderr())
		case "list":
			err = cmdList(fields[1:])
		case "do":
			err = cmdDo(fields[1:])
		case "run", "step", "next", "breakpoint":
			err = errors.New("command not implemented: " + fields[0])
		case "load":
			err = cmdLoad(fields[1:])
		case "exit":
			break readline_loop
		default:
			err = errors.New("unknown command: " + fields[0] + "")
		}
		if err != nil {
			fmt.Fprintln(rl.Stderr(), err.Error())
		}
	}

}

func reloadSource() error {
	initListCommand(context.source)

	closeShell()

	fifo, err := mkfifo()
	go func() {
		f, err := os.OpenFile(fifo, os.O_WRONLY, 0200)
		if err != nil {
			return
		}
		for {
			s, more := <-context.ch
			io.WriteString(f, s)
			if !more {
				return
			}

		}
		defer f.Close()

	}()
	if err != nil {
		return err
	}
	context.ch = make(chan string)
	context.fifo = fifo
	context.cmd = exec.Command("/bin/sh", fifo)
	if err != nil {
		return err
	}
	stdout, err := context.cmd.StdoutPipe()
	if err != nil {
		return err
	}
	stderr, err := context.cmd.StderrPipe()
	if err != nil {
		return err
	}
	go func() {
		io.Copy(context.stdout, stdout)
	}()
	go func() {
		io.Copy(context.stderr, stderr)
	}()
	return context.cmd.Start()
}

func closeShell() {
	if context.ch != nil {
		close(context.ch)
	}

	if context.cmd != nil {
		in, err := context.cmd.StdinPipe()
		if err == nil {
			in.Close()
		}
		context.cmd.Wait()
	}
	if context.fifo != "" {
		os.Remove(context.fifo)
	}
}

func run() int {
	if len(os.Args) > 1 {
		context.source = os.Args[1]
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

	context.stderr = rl.Stderr()
	context.stdout = rl.Stdout()
	err = reloadSource()
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return 1
	}

	readlineLoop(rl)

	return 0
}

func main() {
	os.Exit(run())
}
