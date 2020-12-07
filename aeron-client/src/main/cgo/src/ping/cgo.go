// Copyright (C) FIXME
package main

/*

#cgo CFLAGS: -I ../../../c
#cgo LDFLAGS: -L ../../../../../../cppbuild/Release/lib -laeron_static -ldl
#include <aeronc.h>

// Gateway functions

void PrintAvailableImage_cgo(void *clientd, aeron_subscription_t *subscription, aeron_image_t *image)
{
   void PrintAvailableImage(void *, aeron_subscription_t *, aeron_image_t *);
   PrintAvailableImage(clientd, subscription, image);
}

void PrintUnavailableImage_cgo(void *clientd, aeron_subscription_t *subscription, aeron_image_t *image)
{
   void PrintUnavailableImage(void *, aeron_subscription_t *, aeron_image_t *);
   PrintUnavailableImage(clientd, subscription, image);
}
*/
import "C"
