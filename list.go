package main

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
	"unicode"
)

type lineContext struct {
	stdout io.Writer
	stderr io.Writer
	source string
	width  int
	last   int
}

func newListContext(source string, stdout io.Writer, stderr io.Writer) (*lineContext, error) {
	nos, err := countFileLines(source)
	if err != nil {
		return nil, err
	}
	w := len(strconv.FormatInt(int64(nos), 10))
	return &lineContext{
		stdout: stdout,
		stderr: stderr,
		source: source,
		width:  w,
	}, nil
}

func countFileLines(source string) (int, error) {
	f, err := os.Open(source)
	if err != nil {
		return 0, nil
	}
	defer f.Close()

	n := 0
	s := bufio.NewScanner(f)
	for s.Scan() {
		n++
	}
	return n, nil
}

func (c *lineContext) run(args []string) error {
	if len(c.source) == 0 {
		return errors.New("file not loaded")
	}
	if len(args) == 0 {
		return c.fromNumber(c.last, 10)
	}
	n, err := strconv.Atoi(args[0])
	if err == nil {
		return c.fromNumber(n, 10)
	} else {
		return c.fromKeyword(args[0], 10)
	}
}

func (c *lineContext) fromNumber(from int, count int) error {
	f, err := os.Open(c.source)
	if err != nil {
		return err
	}
	defer f.Close()

	format := "%" + strconv.FormatInt(int64(c.width), 10) + "d %s\n"
	s := bufio.NewScanner(f)
	no := 1
	for s.Scan() && no <= from+count {
		if no >= from {
			fmt.Fprintf(c.stderr, format, no, s.Text())
		}
		no++
	}
	c.last = no
	if !s.Scan() {
		c.last = 0
	}
	return nil
}

func (c *lineContext) fromKeyword(keyword string, count int) error {
	f, err := os.Open(c.source)
	if err != nil {
		return err
	}
	defer f.Close()

	delf := func(c rune) bool {
		return !unicode.IsLetter(c) && !unicode.IsNumber(c) && c != '_'
	}

	format := "%" + strconv.FormatInt(int64(c.width), 10) + "d %s\n"
	s := bufio.NewScanner(f)
	no := 1
	from := -1
	for s.Scan() {
		text := s.Text()
		if from < 0 {
			fields := strings.FieldsFunc(text, delf)
			for _, f := range fields {
				if f == keyword {
					from = no
					break
				}
			}

		}
		if from >= 0 {
			fmt.Fprintf(c.stderr, format, no, s.Text())
		}
		no++
		if from >= 0 && no > from+count {
			break
		}
	}
	if from == -1 {
		return errors.New("keyword not found: " + keyword)
	}
	c.last = no
	if !s.Scan() {
		c.last = 0
	}
	return nil
}
