package main

import (
	"fmt"
	"os"
	"path/filepath"
	"syscall"
)

func mkfifo() (string, error) {
	dir := os.TempDir()
	base := fmt.Sprintf("buggywhip-%d", os.Getpid())
	name := filepath.Join(dir, base)
	err := syscall.Mknod(name, syscall.S_IFIFO|0666, 0)
	if err != nil {
		return "", err
	}
	return name, nil
}
