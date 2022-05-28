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
