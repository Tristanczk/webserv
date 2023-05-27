package main

import (
	"fmt"
	"net"
	"os"
	"time"
)

func main() {
	msg := fmt.Sprintf("TESTING")
	
	c, _ := net.Dial("tcp", "127.0.0.1:8888")
	for i := 0; i < 3; i++ {
		c.Write([]byte(fmt.Sprintf("%s %d", msg, os.Getpid())))
		time.Sleep(time.Millisecond * time.Duration(100))
	}
	c.Close()
}