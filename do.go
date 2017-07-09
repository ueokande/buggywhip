package main

import (
	"errors"
	"strings"
)

func cmdDo(args []string) error {
	if context.ch == nil {
		errors.New("Shell not started")
	}
	if len(args) == 0 {
		return nil
	}
	context.ch <- strings.Join(args, " ") + "\n"
	return nil
}
