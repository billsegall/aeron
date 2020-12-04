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

func SizeofHeaderFrame() uintptr {
	var chvf C.aeron_header_values_frame_t
	return unsafe.Sizeof(chvf)
}

func SizeofHeaderValues() uintptr {
	var chv C.aeron_header_values_t
	return unsafe.Sizeof(chv)
}

func SizeofCounterValueDescriptor() uintptr {
	var ccvd C.aeron_counter_value_descriptor_t
	return unsafe.Sizeof(ccvd)
}

func SizeofCounterMetadataDescriptor() uintptr {
	var ccmd C.aeron_counter_metadata_descriptor_t
	return unsafe.Sizeof(ccmd)
}
