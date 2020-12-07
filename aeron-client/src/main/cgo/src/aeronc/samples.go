// Copyright FIXME
package aeronc

/*
#include <stdbool.h>
#include <stdint.h>
const char *CDefaultChannel = "aeron:udp?endpoint=localhost:20121";
const char *CDefaultPingChannel = "Aeron:udp?endpoint=localhost:20123";
const char *DefaultPongChannel = "aeron:udp?endpoint=localhost:20124";
const int32_t CDefaultStreamId = 1001;
const int32_t CDefaultPingStreamId = 1002;
const int32_t CDefaultPongStreamId = 1003;
const int32_t CDefaultNumberOfWarmUpMessages = 100000;
const int32_t CDefaultNumberOfMessages = 10000000;
const int32_t CDefaultMessageLength = 32;
const int32_t CDefaultLingerTimeoutMs = 0;
const int32_t CDefaultFragmentCountLimit = 10;
const bool CDefaultRandomMessageLength = false;
const bool CDefaultPublicationRateProgress = false;
*/
import "C"

// FIXME: scoping: samples?

const DefaultChannel = "aeron:udp?endpoint=localhost:20121"
const DefaultPingChannel = "Aeron:udp?endpoint=localhost:20123"
const DefaultPongChannel = "aeron:udp?endpoint=localhost:20124"
const DefaultStreamId = 1001
const DefaultPingStreamId = 1002
const DefaultPongStreamId = 1003
const DefaultNumberOfWarmUpMessages = 100000
const DefaultNumberOfMessages = 10000000
const DefaultMessageLength = 32
const DefaultLingerTimeoutMs = 0
const DefaultFragmentCountLimit = 10
const DefaultRandomMessageLength = false
const DefaultPublicationRateProgress = false
