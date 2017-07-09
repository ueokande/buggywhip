package main

import (
	"bufio"
	"errors"
	"fmt"
	"os"
	"strconv"
	"strings"
	"unicode"
)

var listContext struct {
	noWidth      int
	listLastLine int
}

func initListCommand(source string) error {
	nos, err := countFileLines(source)
	if err != nil {
		return err
	}
	listContext.noWidth = len(strconv.FormatInt(int64(nos), 10))
	return nil
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

func cmdList(args []string) error {
	if len(context.source) == 0 {
		return errors.New("file not loaded")
	}
	if len(args) == 0 {
		return cmdLineNumber(listContext.listLastLine, 10)
	}
	n, err := strconv.Atoi(args[0])
	if err == nil {
		return cmdLineNumber(n, 10)
	} else {
		return cmdLineKeyword(args[0], 10)
	}
}

func cmdLineNumber(from int, count int) error {
	f, err := os.Open(context.source)
	if err != nil {
		return err
	}
	defer f.Close()

	format := "%" + strconv.FormatInt(int64(listContext.noWidth), 10) + "d %s\n"
	s := bufio.NewScanner(f)
	no := 1
	for s.Scan() && no <= from+count {
		if no >= from {
			fmt.Fprintf(context.stderr, format, no, s.Text())
		}
		no++
	}
	listContext.listLastLine = no
	if !s.Scan() {
		listContext.listLastLine = 0
	}
	return nil
}

func cmdLineKeyword(keyword string, count int) error {
	f, err := os.Open(context.source)
	if err != nil {
		return err
	}
	defer f.Close()

	delf := func(c rune) bool {
		return !unicode.IsLetter(c) && !unicode.IsNumber(c) && c != '_'
	}

	format := "%" + strconv.FormatInt(int64(listContext.noWidth), 10) + "d %s\n"
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
			fmt.Fprintf(context.stderr, format, no, s.Text())
		}
		no++
		if from >= 0 && no > from+count {
			break
		}
	}
	if from == -1 {
		return errors.New("keyword not found: " + keyword)
	}
	listContext.listLastLine = no
	if !s.Scan() {
		listContext.listLastLine = 0
	}
	return nil
}
