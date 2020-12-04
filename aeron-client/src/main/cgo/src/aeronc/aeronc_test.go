// Copyright FIXME
package aeronc

import (
	"os"
	"testing"
)

func TestMain(m *testing.M) {
	Setup()
	os.Exit(m.Run())
}

// Given aeron-c uses packed structs make sure we know if anything changes
func TestPackedStructs(t *testing.T) {

	size := SizeofHeaderFrame()
	if size != 32 {
		t.Log("Unexpected change to header frame size", size)
		t.Fail()
	}

	size = SizeofHeaderValues()
	if size != 40 {
		t.Log("Unexpected change to header value size", size)
		t.Fail()
	}

	size = SizeofCounterValueDescriptor()
	if size != 128 {
		t.Log("Unexpected change to counter value descriptor size", size)
		t.Fail()
	}

	size = SizeofCounterMetadataDescriptor()
	if size != 512 {
		t.Log("Unexpected change to counter metadata descriptor size", size)
		t.Fail()
	}
}
