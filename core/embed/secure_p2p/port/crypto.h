#pragma once

// Include-name shim for the sources under secure_p2p/handshake and
// secure_p2p/transfer, which pull the crypto primitives as <crypto.h>.
// Everything they need lives in the port header.

#include "embed/secure_channels/secure_channel_port.h"
