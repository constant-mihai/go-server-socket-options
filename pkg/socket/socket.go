package socket

import (
	"bufio"
	"context"
	"io"
	"log"
	"net"
	"syscall"
	"time"

	"golang.org/x/sys/unix"
)

func handleRequest(ctx context.Context, conn net.Conn) {
	buf := make([]byte, 10000)
	n, err := conn.Read(buf)
	if err != nil {
		if err == io.EOF {
			log.Printf("server: received EOF")
			return
		}
		log.Printf("server: error receiving byes: %s", err.Error())
		return
	}
	log.Printf("server: received %d bytes: %s", n, string(buf))
}

func listen(ctx context.Context) {
	ln, err := net.Listen("tcp4", "127.0.0.1:12345")
	if err != nil {
		log.Fatalln("server: Error listening:", err.Error())
	}
	defer ln.Close()
	for {
		select {
		case <-ctx.Done():
			return
		default:
			conn, err := ln.Accept()
			if err != nil {
				log.Fatalln("server: Error accepting: ", err.Error())
			}
			go handleRequest(ctx, conn)
		}
	}
}

// The feature to set socket options is missing for UDP.
//
// sysListener contains a Listen's parameters and configuration.
// type sysListener struct {
//     ListenConfig
//     network, address string
// }
// func ListenUDP(network string, laddr *UDPAddr) (*UDPConn, error) {
//     ...
//     sl := &sysListener{network: network, address: laddr.String()}
//     ...
//     return c, nil
// }
// One can use however:
// sock, err := lc.ListenPacket(ctx, "udp", *address)

func listenSockopt(ctx context.Context) {
	lc := net.ListenConfig{
		Control: func(network, address string, conn syscall.RawConn) error {
			var operr error
			if err := conn.Control(func(fd uintptr) {
				operr = syscall.SetsockoptInt(int(fd), unix.SOL_SOCKET, unix.SO_REUSEPORT, 1)
			}); err != nil {
				return err
			}
			return operr
		},
	}

	ln, err := lc.Listen(context.Background(), "tcp4", "0.0.0.0:12345")
	defer ln.Close()
	if err != nil {
		log.Fatalln("server: Error listening:", err.Error())
	}

	for {
		select {
		case <-ctx.Done():
			return
		default:
			conn, err := ln.Accept()
			if err != nil {
				log.Fatalln("server: Error accepting: ", err.Error())
			}
			go handleRequest(ctx, conn)
		}
	}

}

func send() {
	p := make([]byte, 1000)
	dst, err := net.ResolveTCPAddr("tcp", "127.0.0.1:12345")
	if err != nil {
		log.Fatalln("client: Error resolving tcp addr: ", err.Error())
	}

	var (
		connected bool
		conn      *net.TCPConn
	)
	for i := 0; i < 100; i += 5 {
		conn, err = net.DialTCP("tcp", nil, dst)
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
		_, err = conn.Write([]byte("test string"))
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
