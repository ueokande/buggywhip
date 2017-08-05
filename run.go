package main

import (
	"bufio"
	"os"
)

func (c context) run() error {
	f, err := os.Open(c.source)
	if err != nil {
		return err
	}

	s := bufio.NewScanner(f)
	for s.Scan() {
		c.shell.Write([]byte(s.Text() + "\n"))
	}
	return nil
}
