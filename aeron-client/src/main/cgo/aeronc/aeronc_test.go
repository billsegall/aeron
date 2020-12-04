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
func TestSizeof(t *testing.T) {
	if SizeofHeader() != 32 {
		t.Log("Unexpected change to header size")
		t.Fail()
	}
}
