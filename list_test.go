package main

import (
	"bytes"
	"strings"
	"testing"
)

func TestListContinuously(t *testing.T) {
	c, err := newContext("/bin/cat", "./testdata/reboot", nil, nil)
	if err != nil {
		t.Fatal(err)
	}

	b := new(bytes.Buffer)
	err = c.listByNum(0, 5, b)
	if err != nil {
		t.Fatal(err)
	}
	defer c.close()

	if body := b.String(); !strings.HasPrefix(body, " 1 #! /bin/sh\n") ||
		len(strings.Split(body, "\n")) != 6 {
		t.Fatal("Unexpected body: ", body)
	}

	b = new(bytes.Buffer)
	err = c.listContinuously(5, b)
	if err != nil {
		t.Fatal(err)
	}
	if body := b.String(); !strings.HasPrefix(body, " 6 # Default-Start:\n") ||
		len(strings.Split(body, "\n")) != 6 {
		t.Fatal("Unexpected body: ", body, len(strings.Split(body, "\n")))
	}
}

func TestListByNum(t *testing.T) {
	c, err := newContext("/bin/cat", "./testdata/reboot", nil, nil)
	if err != nil {
		t.Fatal(err)
	}

	var b bytes.Buffer
	err = c.listByNum(11, 5, &b)
	if err != nil {
		t.Fatal(err)
	}
	c.close()

	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	if body := b.String(); !strings.HasPrefix(body, "12 PATH=/sbin:/usr/sbin:/bin:/usr/bin\n") ||
		len(strings.Split(body, "\n")) != 6 {
		t.Fatal("Unexpected body: ", body)
	}
}

func TestListByKeyword(t *testing.T) {
	c, err := newContext("/bin/cat", "./testdata/reboot", nil, nil)
	if err != nil {
		t.Fatal(err)
	}

	var b bytes.Buffer
	err = c.listByKeyword("do_stop", 5, &b)
	if err != nil {
		t.Fatal(err)
	}
	c.close()

	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	if body := b.String(); !strings.HasPrefix(body, "16 do_stop () {\n") ||
		len(strings.Split(body, "\n")) != 6 {
		t.Fatal("Unexpected body: ", body)
	}
}
