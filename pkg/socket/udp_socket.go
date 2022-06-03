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
	"syscall"
	"time"
	"unsafe"
)

func listenUdpSockopt(ctx context.Context, id string) {
	log.Printf("starting gorutine: %s", id)

	buf := C.malloc(C.sizeof_char * C.size_t(C.BUF_SIZE))
	defer C.free(unsafe.Pointer(buf))

	host := C.CString("0.0.0.0")
	defer C.free(unsafe.Pointer(host))
	serv := C.CString("12345")
	defer C.free(unsafe.Pointer(serv))

	var saddrLen C.uint = 0
	sockfd := C.udp_server(host, serv, &saddrLen)

	for {
		select {
		case <-ctx.Done():
			return
		default:
			size := C.udp_read(sockfd, (*C.char)(buf))
			if size < 0 {
				log.Fatalln("server: Error reading data: ", size)
			}
			str := string(C.GoBytes(buf, size))
			log.Printf("[goroutine: %s] read message: %s", id, str)

			// TODO: without a sleep here, goroutine 1 will starve out 2
			// _with_ a sleep here, goroutine 2 will starve out 1
			// I should find if this is actually a problem and if it is, I should
			// search for a better solution.
			if id == "1" {
				time.Sleep(100 * time.Millisecond)
			}
		}
	}
}

// This uses a C function to create the socket, I have later learned that go offers the options to
// configure UDP sockets via ListenConfig:
//var lc = net.ListenConfig{
//        Control: func(network, address string, c syscall.RawConn) error {
//                var opErr error
//                if err := c.Control(func(fd uintptr) {
//                        opErr = unix.SetsockoptInt(int(fd), unix.SOL_SOCKET, unix.SO_REUSEPORT, 1)
//                }); err != nil {
//                        return err
//                }
//                return opErr
//        },
//}
// ...
//sock, err := lc.ListenPacket(ctx, "udp", *address)
func udpSocket(ip, port string) (C.int, *C.struct_sockaddr, C.uint) {
	host := C.CString(ip)
	defer C.free(unsafe.Pointer(host))
	serv := C.CString(port)
	defer C.free(unsafe.Pointer(serv))

	// TODO, need to double check here if everything is legal
	// or if there could be a memory leak.
	// https://pkg.go.dev/cmd/cgo#hdr-Passing_pointers
	var (
		saddrLen C.uint             = 0
		saddr    *C.struct_sockaddr = nil
	)
	return C.udp_socket(host, serv, &saddr, &saddrLen), saddr, saddrLen
}

// This function calls only bind(sockfd). The sockfd needs to be created externaly.
// It cannot be used for multiple goroutines because it will use bind() on the same sockfd.
// In case I reuse multiple goroutines, all except the first bind() will fail.
//mihai@deu0207: [socket] $ go test -v -run=TestListenUdpSockoptReusePort
//=== RUN   TestListenUdpSockoptReusePort
//2022/05/28 17:10:44 starting gorutine: 1
//2022/05/28 17:10:44 starting gorutine: 2
//2022/05/28 17:10:44 [goroutine 2] error binding to the socket: invalid argument
func listenUdpSockoptReusePort(
	ctx context.Context,
	sockfd C.int,
	saddr *C.struct_sockaddr,
	saddrLen C.uint,
	id string,
) {
	log.Printf("starting gorutine: %s", id)
	if retVal, err := C.bind(sockfd, saddr, saddrLen); retVal != 0 {
		log.Printf("[goroutine %s] error binding to the socket: %s", id, err.(syscall.Errno).Error())
		return
	}

	buf := C.malloc(C.sizeof_char * C.size_t(C.BUF_SIZE))
	defer C.free(unsafe.Pointer(buf))
	for {
		select {
		case <-ctx.Done():
			return
		default:
			size := C.udp_read(sockfd, (*C.char)(buf))
			if size < 0 {
				log.Fatalln("server: Error reading data: ", size)
			}
			str := string(C.GoBytes(buf, size))
			log.Printf("[goroutine: %s] read message: %s", id, str)

			// TODO: without a sleep here, goroutine 1 will starve out 2
			if id == "1" {
				time.Sleep(100 * time.Millisecond)
			}
		}
	}
}

func sendUdp() {
	p := make([]byte, C.BUF_SIZE)
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

func advanceBuf(buf **C.char, i int) **C.char {
	up := unsafe.Pointer(buf)
	upAtIdx := uintptr(up) + uintptr(i)
	return (**C.char)(unsafe.Pointer(upAtIdx))
}

func mSendUdp() {
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

	packetNum := 100_000
	var sent C.int = 0
	var buf **C.char
	msg := C.CString("test message")
	defer C.free(unsafe.Pointer(msg))

	// https://go101.org/article/unsafe.html
	// pattern 3
	size := C.int(unsafe.Sizeof(buf)) * C.BATCH_SIZE
	buf = (**C.char)(C.malloc(C.size_t(size)))
	defer C.free(unsafe.Pointer(buf))
	for i := 0; i < int(C.BATCH_SIZE); i++ {
		bufAtIdx := advanceBuf(buf, i)
		allocatedMemory := C.malloc(C.sizeof_char * C.size_t(C.BUF_SIZE)) // this is an unsafe.Pointer
		*bufAtIdx = (*C.char)(allocatedMemory)
		C.strcpy(*bufAtIdx, msg)
		defer C.free(unsafe.Pointer(bufAtIdx))
	}

	sockfd, _, _ := udpSocket("127.0.0.1", "12345")
	if sockfd < 0 {
		log.Fatalf("error opening socket")
	}

	for {
		time.Sleep(10 * time.Nanosecond)
		sent += C.udp_msend(sockfd, buf)

		if int(sent) >= packetNum {
			log.Printf("client: Client finished\n")
			return
		}
	}
}

func mReceiveUdp(id string) {
	sockfd, _, _ := udpSocket("127.0.0.1", "12345")
	if sockfd < 0 {
		log.Fatalf("error opening socket\n")
	}

	var buf **C.char

	size := C.int(unsafe.Sizeof(buf)) * C.BATCH_SIZE
	buf = (**C.char)(C.malloc(C.size_t(size)))
	defer C.free(unsafe.Pointer(buf))
	for i := 0; i < int(C.BATCH_SIZE); i++ {
		bufAtIdx := advanceBuf(buf, i)
		allocatedMemory := C.malloc(C.sizeof_char * C.size_t(C.BUF_SIZE)) // this is an unsafe.Pointer
		bufAtIdx = (**C.char)(allocatedMemory)
		defer C.free(unsafe.Pointer(bufAtIdx))
	}

	for {
		n := C.udp_mread(sockfd, buf)
		for i := 0; i < int(n); i++ {
			bufAtIdx := advanceBuf(buf, i)
			str := string(C.GoBytes(unsafe.Pointer(*bufAtIdx), 100)) // TODO 100 is the size of read buffer in the iovec, need to somehow propagate it.
			log.Printf("[goroutine: %s] read message: %s", id, str)
		}
	}
}
