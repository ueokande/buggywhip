package main

import (
	"bytes"
	"strings"
	"testing"
)

func TestLine(t *testing.T) {
	var b bytes.Buffer

	ctx, err := newListContext("./testdata/reboot", &b)
	if err != nil {
		t.Fatal(err)
	}

	cases := []struct {
		args   []string
		found  bool
		prefix string
	}{
		{[]string{}, true, " 1 #! /bin/sh\n"},
		{[]string{}, true, "11 \n"},
		{[]string{"12"}, true, "12 PATH=/sbin:/usr/sbin:/bin:/usr/bin\n"},
		{[]string{"do_stop"}, true, "16 do_stop () {\n"},
		{[]string{"alice-in-wonderland"}, false, ""},
	}

	for _, c := range cases {
		b.Truncate(0)
		err = ctx.run(c.args)
		if c.found {
			if err != nil {
				t.Fatal("Unexpected error", err)
			}
			if body := b.String(); !strings.HasPrefix(body, c.prefix) {
				t.Fatal("Unexpected body: ", body)
			}
		} else {
			if err == nil {
				t.Fatal("Unexpected error", err)
			}
		}
	}
}
