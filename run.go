package main

import (
	"bufio"
	"errors"
	"os"
)

func cmdRun(args []string) error {
	if len(context.source) == 0 {
		return errors.New("file not loaded")
	}

	f, err := os.Open(context.source)
	if err != nil {
		return err
	}

	s := bufio.NewScanner(f)
	for s.Scan() {
		context.ch <- s.Text() + "\n"
	}
	return nil
}
