// Copyright FIXME
package main

/*
#cgo CFLAGS: -I ../../../c
#cgo LDFLAGS: -L ../../../../../../cppbuild/Release/lib -laeron_static -ldl
#include <aeronc.h>

// Forward declarations
void PrintAvailableImage_cgo(void *clientd, aeron_subscription_t *subscription, aeron_image_t *image);
void PrintUnavailableImage_cgo(void *clientd, aeron_subscription_t *subscription, aeron_image_t *image);
*/
import "C"

import (
	"aeronc"
	"fmt"
	"os"
	"unsafe"
)

func main() {
	PingChannel := aeronc.DefaultPingChannel
	PongChannel := aeronc.DefaultPongChannel
	PingStreamId := aeronc.DefaultPingStreamId
	PongStreamId := aeronc.DefaultPongStreamId
	var Context *C.aeron_context_t
	var Client *C.aeron_t
	var AsyncPongSub *C.aeron_async_add_subscription_t
	var Subscription *C.aeron_subscription_t

	// FIXME: args for all the above defaults
	// FIXME: signal handling

	// Log
	fmt.Printf("Publishing Ping at channel %s on Stream ID %d\n", PingChannel, PingStreamId)
	fmt.Printf("Subscribing Pong at channel %s on Stream ID %d\n", PongChannel, PongStreamId)

	// Create context
	if C.aeron_context_init(&Context) < 0 {
		fmt.Fprintf(os.Stderr, "aeron_context_init: %s\n", C.GoString(C.aeron_errmsg()))
		// FIXME: goto cleanup
	}

	// FIXME: allow setting of aeron_dir
	//if C.aeron_context_set_dir(context, aeron_dir) < 0 {
	//	fmt.Fprintf(os.Stderr, "aeron_context_set_dir: %s\n", C.aeron_errmsg())
	//}

	// Init and Start
	if C.aeron_init(&Client, Context) < 0 {
		fmt.Fprintf(os.Stderr, "aeron_init: %s\n", C.GoString(C.aeron_errmsg()))
	}
	if C.aeron_start(Client) < 0 {
		fmt.Fprintf(os.Stderr, "aeron_start: %s\n", C.GoString(C.aeron_errmsg()))
	}

	if C.aeron_async_add_subscription(&AsyncPongSub, Client, C.CString(PongChannel), C.int32_t(PongStreamId), (C.aeron_on_available_image_t)(unsafe.Pointer(C.PrintAvailableImage_cgo)), nil, (C.aeron_on_unavailable_image_t)(unsafe.Pointer(C.PrintUnavailableImage_cgo)), nil) < 0 {
		fmt.Fprintf(os.Stderr, "aeron_async_add_subscription: %s\n", C.GoString(C.aeron_errmsg()))
	}

	for nil == Subscription {
		if C.aeron_async_add_subscription_poll(&Subscription, AsyncPongSub) < 0 {
			fmt.Fprintf(os.Stderr, "aeron_async_add_subscription_poll: %s\n", C.GoString(C.aeron_errmsg()))
		}
	}

	// FIXME: cleanup - build defer list
	return
}

//export PrintAvailableImage
func PrintAvailableImage(clientd unsafe.Pointer, subscription *C.aeron_subscription_t, image *C.aeron_image_t) {
	var subscription_constants C.aeron_subscription_constants_t
	var image_constants C.aeron_image_constants_t

	if C.aeron_subscription_constants(subscription, &subscription_constants) < 0 ||
		C.aeron_image_constants(image, &image_constants) < 0 {
		fmt.Fprintf(os.Stderr, "could not get subscription/image constants: %s\n", C.GoString(C.aeron_errmsg()))
	} else {
		fmt.Fprintf(os.Stdout, "Available image on %s streamId=%d sessionId=% from %s\n",
			subscription_constants.channel,
			subscription_constants.stream_id,
			image_constants.session_id,
			image_constants.source_identity)
	}
	return
}

//export PrintUnavailableImage
func PrintUnavailableImage(clientd unsafe.Pointer, subscription *C.aeron_subscription_t, image *C.aeron_image_t) {
	var subscription_constants C.aeron_subscription_constants_t
	var image_constants C.aeron_image_constants_t

	if C.aeron_subscription_constants(subscription, &subscription_constants) < 0 ||
		C.aeron_image_constants(image, &image_constants) < 0 {
		fmt.Fprintf(os.Stderr, "could not get subscription/image constants: %s\n", C.GoString(C.aeron_errmsg()))
	} else {
		fmt.Fprintf(os.Stdout, "Unavailable image on %s streamId=%d sessionId=% from %s\n",
			subscription_constants.channel,
			subscription_constants.stream_id,
			image_constants.session_id,
			image_constants.source_identity)
	}
}
