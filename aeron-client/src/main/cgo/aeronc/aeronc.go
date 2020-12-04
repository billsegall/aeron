// Copyright FIXME
package aeronc

/*
#cgo CFLAGS: -I ../../c
#cgo LDFLAGS: -L ../../../../../cppbuild/Release/lib -laeron_client
#include <aeronc.h>
*/
import "C"

import (
	"unsafe"
)

func Setup() {
	return
}

func SizeofHeader() uintptr {
	var chvf C.aeron_header_values_frame_t
	return unsafe.Sizeof(chvf)
}
