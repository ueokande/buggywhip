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

func fileLineNoWidth(source string) (int, error) {
	nos, err := countFileLines(source)
	if err != nil {
		return 0, err
	}
	w := len(strconv.FormatInt(int64(nos), 10))
	return w, nil
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

func (c *context) listContinuously(count int, out io.Writer) error {
	return c.listByNum(c.listLast+0, count, out)
}

func (c *context) listByNum(from int, count int, out io.Writer) error {
	f, err := os.Open(c.source)
	if err != nil {
		return err
	}
	defer f.Close()

	format := "%" + strconv.FormatInt(int64(c.listWidth), 10) + "d %s\n"
	s := bufio.NewScanner(f)
	no := 0
	for s.Scan() && no < from+count {
		if no >= from {
			fmt.Fprintf(out, format, no+1, s.Text())
		}
		no++
	}
	c.listLast = no
	if !s.Scan() {
		c.listLast = 0
	}
	return nil
}

func (c *context) listByKeyword(keyword string, count int, out io.Writer) error {
	f, err := os.Open(c.source)
	if err != nil {
		return err
	}
	defer f.Close()

	delf := func(c rune) bool {
		return !unicode.IsLetter(c) && !unicode.IsNumber(c) && c != '_'
	}

	format := "%" + strconv.FormatInt(int64(c.listWidth), 10) + "d %s\n"
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
			fmt.Fprintf(out, format, no, s.Text())
		}
		no++
		if from >= 0 && no >= from+count {
			break
		}
	}
	if from == -1 {
		return errors.New("keyword not found: " + keyword)
	}
	c.listLast = no
	if !s.Scan() {
		c.listLast = 0
	}
	return nil
}
