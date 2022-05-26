package socket

import (
	"context"
	"testing"
)

func TestListen(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	go listen(ctx)

	send()
	cancel()
}

func TestListenSockopt(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	go listenSockopt(ctx)
	go listenSockopt(ctx)

	send()
	cancel()
}
