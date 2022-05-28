package socket

import (
	"context"
	"testing"
	"time"
)

func TestListenUdpSockopt(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	go listenUdpSockopt(ctx, "1")
	time.Sleep(100 * time.Millisecond) //not sure if this is needed. does the UDP socket has a state machine?
	go listenUdpSockopt(ctx, "2")

	sendUdp()
	cancel()
}

func TestListenUdpSockoptReusePort(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	sockfd, saddr, saddrLen := udpSocket()
	if sockfd < 0 {
		t.Errorf("Error opening socket")
	}
	go listenUdpSockoptReusePort(ctx, sockfd, saddr, saddrLen, "1")
	go listenUdpSockoptReusePort(ctx, sockfd, saddr, saddrLen, "2")

	sendUdp()
	cancel()
}
