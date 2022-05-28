package socket

// #cgo CFLAGS: -g -Wall
// #include <stdlib.h>
// #include "udp.h"
import "C"

import (
	"bufio"
	"context"
	"log"
	"net"
	"time"
	"unsafe"
)

const BUFF_SIZE = 1024

func listenUdpSockopt(ctx context.Context) {
	buf := C.malloc(C.sizeof_char * BUFF_SIZE)
	defer C.free(unsafe.Pointer(buf))

	host := C.CString("127.0.0.1")
	defer C.free(unsafe.Pointer(host))
	serv := C.CString("12345")
	defer C.free(unsafe.Pointer(serv))

	// example of using a c struct
	// server := C.server_t{
	//     host: host,
	//     serv: serv,
	// }

	var socklenp C.uint = 0
	sockfd := C.udp_server(host, serv, &socklenp)

	for {
		select {
		case <-ctx.Done():
			return
		default:
			size := C.udp_mread(sockfd, (*C.char)(buf))
			if size < 0 {
				log.Fatalln("server: Error reading data: ", size)
			}
			str := string(C.GoBytes(buf, size))
			log.Printf("read message: %s", str)
		}
	}

}

func sendUdp() {
	p := make([]byte, BUFF_SIZE)
	var (
		connected bool
		conn      net.Conn
		err       error
	)
	for i := 0; i < 100; i += 5 {
		conn, err = net.Dial("udp", "127.0.0.1:12345")
		connected = true
		if err != nil {
			log.Println("client: Dial failed:", err.Error())
			connected = false
		}
	}
	if !connected {
		log.Println("client: Dial failed")
		return
	}
	defer conn.Close()

	log.Println("client: Dial successful")

	timeThen := time.Now()
	for {
		time.Sleep(10 * time.Millisecond)
		_, err = conn.Write([]byte("some message here"))
		if err != nil {
			log.Fatalln("client: Write to server failed:", err.Error())
		}

		_, err = bufio.NewReader(conn).Read(p)
		if err == nil {
			log.Printf("%s\n", p)
		} else {
			log.Printf("Some error %v\n", err)
		}

		if time.Now().After(timeThen.Add(10 * time.Second)) {
			log.Printf("client: Client finished")
			return
		}
	}
}
