package main

import (
	"errors"
	"os"
)

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
