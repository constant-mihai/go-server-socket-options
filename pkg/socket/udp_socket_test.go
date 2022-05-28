package socket

import (
	"context"
	"testing"
)

func TestListenUdpSockopt(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	go listenUdpSockopt(ctx)

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
