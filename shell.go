package main

import (
	"fmt"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"syscall"
)

type shell struct {
	fifo *os.File
	cmd  *exec.Cmd
}

func newShell(name string, stdout io.Writer, stderr io.Writer) (*shell, error) {
	var err error

	fifoname, err := mkfifo()
	if err != nil {
		return nil, err
	}

	s := new(shell)
	s.cmd, err = runShell(name, stdout, stderr, fifoname)
	if err != nil {
		s.Close()
		return nil, err
	}

	s.fifo, err = os.OpenFile(fifoname, os.O_WRONLY, 0200)
	if err != nil {
		s.Close()
		return nil, err
	}

	return s, err
}

func (s *shell) Write(p []byte) (n int, err error) {
	return s.fifo.Write(p)
}

func (s *shell) Close() error {
	if s.fifo != nil {
		s.fifo.Close()
		os.Remove(s.fifo.Name())
	}
	if s.cmd != nil {
		in, err := s.cmd.StdinPipe()
		if err == nil {
			in.Close()
		}
		return s.cmd.Wait()
	}
	return nil
}

func runShell(name string, stdout io.Writer, stderr io.Writer, arg ...string) (*exec.Cmd, error) {
	cmd := exec.Command(name, arg...)

	if stdout != nil {
		pipe, err := cmd.StdoutPipe()
		if err != nil {
			return nil, err
		}
		go func() { io.Copy(stdout, pipe) }()
	}

	if stderr != nil {
		pipe, err := cmd.StderrPipe()
		if err != nil {
			return nil, err
		}
		go func() { io.Copy(stderr, pipe) }()
	}

	err := cmd.Start()
	if err != nil {
		return nil, err
	}
	return cmd, err
}

func mkfifo() (string, error) {
	dir := os.TempDir()
	base := fmt.Sprintf("buggywhip-%d", os.Getpid())
	name := filepath.Join(dir, base)
	err := syscall.Mknod(name, syscall.S_IFIFO|0666, 0)
	if err != nil {
		fmt.Println("0")
		return "", err
	}
	return name, nil
}
