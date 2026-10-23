# OneKey THD89 SE Command Reference

> **Scope**: This document covers the runtime command interface of the THD89 Secure Element as exposed to the MCU in production firmware. Factory provisioning commands (which are disabled in product mode) are omitted.

---

## 1. Communication Interface

- **Physical protocol**: I2C
- **Transport capability**: Maximum TX/RX data length **1024 bytes**

---

## 2. MCU–SE Key Agreement Protocol

### 2.1 Overview

Encrypted communication between the MCU and the SE (Secure Element) uses **ECDH** to negotiate a session key.

- **Factory provisioning**: The SE generates and provisions its key pair. The MCU reads the SE public key and stores it in OTP (One-Time Programmable) memory.
- **Security constraint**: After manufacturing, the SE public key can no longer be read.

### 2.2 Key Agreement Procedure

#### Step 1: Preparation

1. The MCU reads the SE static public key (`SE_pubkey`) from OTP.
2. The MCU generates a 16-byte random value `r1`.
3. The MCU requests and obtains a 16-byte random value `r2` from the SE.

#### Step 2: Compute Session Key and Send Request

1. The MCU generates a temporary ECDSA key pair (curve `secp256k1`):
   - Temporary private key `prikey_tmp` (32 bytes)
   - Temporary public key `pubkey_tmp` (65 bytes, uncompressed format: `0x04` + 64-byte coordinates)
2. The MCU computes the ECDH session key:

   ```text
   session_tmp = ECDH(prikey_tmp, SE_pubkey)
   Ks = session_tmp.x[0:15]  // Take the first 16 bytes of the shared point X coordinate as the session key
   ```

3. The MCU encrypts the SE random value `r2` using the session key `Ks`:

   ```text
   r2_enc = AES128-ECB(Ks, r2)
   ```

4. The MCU constructs the key agreement APDU and sends it:
   - **APDU header**: `CLA=0x00, INS=0xFA, P1=0x00, P2=0x00, Lc=0x60`
   - **Data field**: `[r1 (16 bytes)] + [r2_enc (16 bytes)] + [pubkey_tmp (64 bytes, without 0x04 prefix)]`

#### Step 3: SE Verifies and Returns Signature

After receiving the data, the SE performs:

1. Compute ECDH using the SE private key and the MCU temporary public key `pubkey_tmp`, deriving `Ks'`.
2. Decrypt `r2_enc` using `Ks'` and verify the plaintext equals the internally stored `r2`.
3. If verification succeeds, compute the digest of `r1`: `digest = SHA256(r1)`.
4. Sign `digest` using the SE private key (ECDSA).
5. Return the 64-byte signature to the MCU.

#### Step 4: MCU Validates Key Agreement

After receiving the signature, the MCU:

1. Computes `digest = SHA256(r1)`.
2. Verifies the signature using the SE public key.
3. If verification succeeds, the key agreement is successful and session key `Ks` is established.

### 2.3 Secure Messaging for Subsequent Traffic

After a successful negotiation, subsequent communication is protected using `Ks`:

1. **IV generation**: Before each transaction, the MCU obtains a 16-byte random IV from the SE (transported encrypted under the session key).
2. **Data encryption**: Encrypt the payload using `AES-128-CBC` (ISO/IEC 7816-4 padding: append `0x80` then `0x00` bytes until the block boundary).
3. **MAC**: Compute `AES-CBC-MAC` over `[APDU header + encrypted data]` (4 bytes).
4. **Final format**: `[APDU header][AES-CBC encrypted data][MAC (4 bytes)]`

### 2.4 Sequence Diagram

```mermaid
sequenceDiagram
    participant MCU
    participant SE as SE Chip
  
    Note over MCU: Step 1: Preparation
    MCU->>MCU: Read SE public key from OTP
    MCU->>MCU: Generate random r1
    MCU->>SE: Request random (INS=0x84)
    SE->>SE: Generate random r2
    SE->>MCU: Return r2 (16 bytes)
  
    Note over MCU: Step 2: Compute session key
    MCU->>MCU: Generate temporary keypair<br/>(pri_tmp, pub_tmp)
    MCU->>MCU: Ks = ECDH(pri_tmp, SE_pub).x[0:15]
    MCU->>MCU: r2_enc = AES_ECB(Ks, r2)
    MCU->>SE: Send negotiation data<br/>[r1(16B)] + [r2_enc(16B)] + [pub_tmp(64B)]<br/>(INS=0xFA, Lc=0x60)
  
    Note over SE: Step 3: Verify and sign
    SE->>SE: Ks' = ECDH(SE_pri, pub_tmp)
    SE->>SE: Decrypt r2_enc and verify r2
    SE->>SE: digest = SHA256(r1)
    SE->>SE: sig = ECDSA_Sign(SE_pri, digest)
    SE->>MCU: Return signature (64 bytes)
  
    Note over MCU: Step 4: Validate key agreement
    MCU->>MCU: digest = SHA256(r1)
    MCU->>MCU: Verify ECDSA signature
    Note over MCU: Negotiation successful, Ks active ✓
```

---

## 3. Command Set (APDU)

> **Note**:
>
> 1. Standard commands use `CLA=0x00` or `0x80`.
> 2. Secure messaging uses `CLA=0x84`, and the packet ends with a 4-byte MAC.

### 3.1 System & Basic Commands

| Description | CLA | INS | P1 | P2 | Data / Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **Get random** | `00` | `84` | `00` | `00` | `Lc=2`: length (2 bytes, big-endian) → returns random bytes of the specified length |
| **Get random (encrypted)** | `84` | `84` | `00` | `00` | Get random under secure channel, AES_CBC |
| **Reset device** | `00` | `F0` | `00` | `00` | Reset the SE and reboot to the main application |
| **Power-on key synchronization** | `00` | `FA` | `00` | `00` | `Lc=0x60`: `[r1(16B)] + [r2_enc(16B)] + [pubkey_tmp(64B)]` → returns signature over r1 (64B) |
| **Get current SE state** | `80` | `CA` | `00` | `00` | Returns: `0x00`=Boot mode, `0x55`=App mode |

### 3.2 Device Information & Version

| Description | CLA | INS | P1 | P2 | Data / Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **Read serial number** | `00` | `F5` | `00` | `00` | → returns serial number |
| **Read device public key** | `00` | `F5` | `00` | `01` | → returns public key (64 bytes) |
| **Read device certificate** | `00` | `F5` | `00` | `02` | → returns certificate |
| **Sign with device private key** | `00` | `F5` | `00` | `03` | `Lc=0x20`: message (32B) → returns signature (64B) |
| **Get version information** | `00` | `F7` | `00` | `00` | → returns version string |
| **Get build ID** | `00` | `F7` | `00` | `01` | → returns build ID (7 bytes) |
| **Get firmware hash** | `00` | `F7` | `00` | `02` | → returns SHA256 (32 bytes) |
| **Get Boot version** | `00` | `F7` | `00` | `03` | → returns Boot version string |
| **Get Boot build ID** | `00` | `F7` | `00` | `04` | → returns Boot build ID |
| **Get Boot hash** | `00` | `F7` | `00` | `05` | → returns Boot SHA256 (32 bytes) |
| **Query initialization status** | `00` | `F8` | `00` | `00` | → `0x55`=initialized, `0x00`=not initialized |
| **Get product mode** | `00` | `F8` | `04` | `00` | → `0x55`=product mode, `0x00`=factory mode |

### 3.3 FIDO/U2F Operations

| Description | CLA | INS | P1 | P2 | Data / Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **Generate FIDO root node** | `00` | `F9` | `00` | `00` | Generate FIDO seed |
| **U2F Register** | `00` | `F9` | `00` | `01` | `Lc=0x40`: `[AppID(32B)] + [Challenge(32B)]` → returns `[KeyHandle(64B)] + [PubKey(65B)] + [Sign(64B)]` |
| **U2F Generate handle & node** | `00` | `F9` | `00` | `02` | `Lc=0x20`: AppID (32B) → returns `[KeyHandle(64B)] + [Node]` |
| **U2F Verify handle** | `00` | `F9` | `00` | `03` | `Lc=0x40`: `[AppID(32B)] + [KeyHandle(64B)]` |
| **U2F Authenticate** | `00` | `F9` | `00` | `04` | `Lc=0x60`: `[AppID(32B)] + [KeyHandle(64B)] + [Challenge(32B)]` → returns `[Counter(4B)] + [Sign(64B)]` |
| **Get U2F counter** | `00` | `F9` | `00` | `05` | → returns counter (4 bytes) |
| **Increment U2F counter** | `00` | `F9` | `00` | `06` | → returns new counter (4 bytes) |
| **Set U2F counter** | `00` | `F9` | `00` | `07` | `Lc=4`: counter value (4 bytes) |
| **FIDO derive node** | `00` | `F9` | `00` | `08` | Curve name (nist256p1) + BIP32 path → returns node |
| **FIDO node signature** | `00` | `F9` | `00` | `09` | `Lc=0x20`: Digest (32B) → returns signature (64B) |
| **FIDO authenticator signature** | `00` | `F9` | `00` | `0A` | `Lc=0x20`: Digest (32B) → returns signature (64B) |

### 3.4 Mnemonics & Data Management

| Description | CLA | INS | P1 | P2 | Data / Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **Import mnemonic** | `84` | `E2` | `00` | `00` | `Lc=len`: BIP39 mnemonic string |
| **Validate mnemonic** | `84` | `E2` | `00` | `01` | `Lc=len`: mnemonic string → returns `0x55`=valid, `0x00`=invalid |
| **Set Backup flag** | `84` | `E2` | `00` | `03` | `Lc=1`: `0x01`=backup required, `0x00`=no backup required |
| **Get Backup flag** | `84` | `E2` | `00` | `04` | → returns `0x01`=backup required, `0x00`=no backup required |
| **Import SLIP39 mnemonic** | `84` | `E2` | `00` | `05` | `Lc=len`: SLIP39 mnemonic info structure |
| **Read public data** | `84` | `E3` | `00` | `00` | `Lc=4`: `[Offset(2B)] + [Len(2B)]` → returns data |
| **Read private data** | `84` | `E3` | `00` | `01` | `Lc=4`: `[Offset(2B)] + [Len(2B)]` → returns data (unlock required) |
| **Read FIDO2 data** | `84` | `E3` | `00` | `03` | `Lc=4`: `[Offset(2B)] + [Len(2B)]` → returns data (unlock required) |
| **Write public data** | `84` | `E4` | `00` | `00` | `Lc=4+len`: `[Offset(2B)] + [Len(2B)] + [Data]` |
| **Write private data** | `84` | `E4` | `00` | `01` | `Lc=4+len`: `[Offset(2B)] + [Len(2B)] + [Data]` (unlock required) |
| **Write FIDO2 data** | `84` | `E4` | `00` | `03` | `Lc=4+len`: `[Offset(2B)] + [Len(2B)] + [Data]` (unlock required) |
| **Clear FIDO2 data** | `84` | `E4` | `00` | `04` | Clear all FIDO2 data |
| **Wipe device** | `84` | `E1` | `00` | `00` | `Lc=0x10`: confirmation data (16B, session key required) |

### 3.5 PIN & Security

| Description | CLA | INS | P1 | P2 | Data / Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **Has PIN** | `84` | `E5` | `00` | `00` | → returns `0x55`=set, `0x00`=not set |
| **Set PIN** | `84` | `E5` | `00` | `01` | `Lc=Len+1`: `[Len(1B)] + [PIN]` |
| **Change PIN** | `84` | `E5` | `00` | `02` | `Lc=OldLen+NewLen+2`: `[OldLen(1B)] + [OldPIN] + [NewLen(1B)] + [NewPIN]` |
| **Verify PIN** | `84` | `E5` | `00` | `03` | `Lc=Len+1` or `Len+2`: `[Len(1B)] + [PIN] + [Type(1B, optional)]` → returns status |
| **Get PIN lock state** | `84` | `E5` | `00` | `04` | → returns `0x55`=unlocked, `0x00`=locked |
| **Get PIN remaining attempts** | `84` | `E5` | `00` | `05` | → returns remaining retry count (1 byte) |
| **Lock PIN** | `84` | `E5` | `00` | `06` | Lock the device immediately |
| **Has wipe code** | `84` | `E5` | `00` | `07` | → returns `0x55`=set, `0x00`=not set |
| **Change wipe code** | `84` | `E5` | `00` | `08` | `Lc=PinLen+WipeLen+2`: `[PinLen(1B)] + [PIN] + [WipeLen(1B)] + [WipeCode]` |
| **Set Passphrase PIN** | `84` | `E5` | `00` | `09` | `Lc=PinLen+PassPinLen+PassLen+3`: `[PinLen] + [PIN] + [PassPinLen] + [PassPIN] + [PassLen] + [Passphrase]` → returns result |
| **Delete Passphrase PIN** | `84` | `E5` | `00` | `0A` | `Lc=PassPinLen+1`: `[PassPinLen(1B)] + [PassPIN]` → returns result |
| **Check Passphrase address** | `84` | `E5` | `00` | `0B` | `Lc=AddrLen+1`: `[AddrLen(1B)] + [Address]` → returns `0x55`=exists, `0x00`=does not exist |
| **Get Passphrase capacity** | `84` | `E5` | `00` | `0C` | → returns remaining capacity (1 byte) |
| **Get Passphrase overwrite flag** | `84` | `E5` | `00` | `0D` | → returns `0x01`=overwrite allowed, `0x00`=not allowed |
| **Change Passphrase PIN** | `84` | `E5` | `00` | `0E` | `Lc=OldLen+NewLen+2`: `[OldLen(1B)] + [OldPIN] + [NewLen(1B)] + [NewPIN]` |

### 3.6 Session & Seed

| Description | CLA | INS | P1 | P2 | Data / Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **Start session** | `84` | `E6` | `00` | `00` | Create a new session → returns SessionID (32 bytes) |
| **Open session** | `84` | `E6` | `00` | `01` | `Lc=0x20`: SessionID (32B) → returns SessionID (32B) |
| **Close current session** | `84` | `E6` | `00` | `02` | Close the currently opened session |
| **Clear session** | `84` | `E6` | `00` | `03` | Close all sessions and lock |
| **Get session cache state** | `84` | `E6` | `00` | `04` | → returns state: `0x80`=seed present, `0x40`=Cardano seed present, `0x00`=none |
| **Generate session seed** | `84` | `E6` | `00` | `05` | `Lc=len`: Passphrase string → generate master seed |
| **Generate Cardano seed** | `84` | `E6` | `00` | `06` | `Lc=len`: Passphrase string → generate Cardano seed |
| **Get session state** | `84` | `E6` | `00` | `07` | → returns `0x55`=session open, `0x00`=no session |
| **Get session type** | `84` | `E6` | `00` | `09` | → returns session type (1 byte) |
| **Get current session ID** | `84` | `E6` | `00` | `0A` | → returns current SessionID (32 bytes) |
| **Seed generation progress** | `80` | `E6` | `00` | `08` | Get seed generation progress (compact) |

### 3.7 Key Derivation

| Description | CLA | INS | P1 | P2 | Data / Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **Derive BIP32 node** | `84` | `E7` | `00` | `00` | `Lc=1+CurveLen+PathLen`: `[CurveLen(1B)] + [Curve] + [Path(4B×N)]` → returns node (unlock required) |
| **Derive XMR node** | `84` | `E7` | `00` | `01` | `Lc=1+CurveLen+PathLen`: `[CurveLen(1B)] + [Curve] + [Path(4B×N)]` → returns node (unlock required) |
| **Derive XMR private key** | `84` | `E7` | `00` | `02` | `Lc=0x24`: `[Pubkey(32B)] + [Index(4B)]` → returns private key (32B, unlock required) |
| **Get XMR transaction key** | `84` | `E7` | `00` | `03` | `Lc=0x40`: `[Rand(32B)] + [Hash(32B)]` → returns key (32B, unlock required) |

### 3.8 Signing

| Description | CLA | INS | P1 | P2 | Data / Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **HDNode sign digest** | `84` | `E8` | `00` | `00` | `Lc=0x20`: Digest (32B) → returns signature (65B, unlock required) |
| **ECDSA sign digest** | `84` | `E8` | `00` | `01` | `Lc=0x22`: `[Curve(1B)] + [IsCanonical(1B)] + [Digest(32B)]` → returns signature (65B, unlock required) |
| **Ed25519 sign** | `84` | `E8` | `00` | `02` | `Lc=len`: message → returns signature (64B, unlock required) |
| **Ed25519_ext sign** | `84` | `E8` | `00` | `03` | `Lc=len`: extended message → returns signature (64B, unlock required) |
| **Ed25519-Keccak sign** | `84` | `E8` | `00` | `04` | `Lc=len`: message data (Keccak-hashed) → returns signature (64B, unlock required) |
| **BIP340 tweak private key** | `84` | `E8` | `00` | `06` | `Lc=0x20` or `0x00`: RootHash (32B, optional) → tweak private key (unlock required) |
| **BIP340 sign digest** | `84` | `E8` | `00` | `07` | `Lc=0x20`: Digest (32B) → returns Schnorr signature (64B, unlock required) |
| **Ed25519 sign digest** | `84` | `E8` | `00` | `08` | `Lc=1`: `[Index(1B)]` → returns signature (64B, unlock required) |
| **BCH Schnorr sign** | `84` | `E8` | `00` | `09` | `Lc=0x20`: Digest (32B) → returns BCH Schnorr signature (64B, unlock required) |
| **Ed25519 hash r** | `84` | `ED` | `XX` | `XX` | `P1=type, P2=step`: start `0x40`, middle `0x00`, end `0x80` → returns R hash value (unlock required) |
| **Ed25519 hash RAM** | `84` | `EE` | `XX` | `XX` | `P1=type, P2=step`: hashing operations for RAM (unlock required) |

### 3.9 ECDH Key Exchange

| Description | CLA | INS | P1 | P2 | Data / Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **ECDSA ECDH** | `84` | `E9` | `00` | `00` | `Lc=0x40`: peer public key (64B) → returns shared secret (unlock required) |
| **CURVE25519 ECDH** | `84` | `E9` | `00` | `01` | `Lc=0x20`: peer public key (32B) → returns shared secret (unlock required) |

### 3.10 AES

| Description | CLA | INS | P1 | P2 | Data / Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **AES256 Encrypt** | `84` | `EA` | `00` | `00` | `Lc=DataLen+ValueLen+4(+16)`: `[DataLen(2B)] + [Data] + [ValueLen(2B)] + [Value] + [IV(16B, optional)]` → returns ciphertext (unlock required) |
| **AES256 Decrypt** | `84` | `EA` | `00` | `01` | `Lc=DataLen+ValueLen+4(+16)`: `[DataLen(2B)] + [Data] + [ValueLen(2B)] + [Value] + [IV(16B, optional)]` → returns plaintext (unlock required) |

### 3.11 SLIP21 Key Derivation

| Description | CLA | INS | P1 | P2 | Data / Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **SLIP21 root node** | `84` | `EB` | `00` | `00` | → returns root node (64B, unlock required) |
| **SLIP21 FIDO root node** | `84` | `EB` | `00` | `01` | → returns FIDO root node (64B, unlock required) |

### 3.12 CoinJoin Authorization

| Description | CLA | INS | P1 | P2 | Data / Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **Set authorization data** | `84` | `EC` | `00` | `00` | `Lc=4+len`: `[Type(4B)] + [Data]` |
| **Get authorization type** | `84` | `EC` | `00` | `01` | → returns type (4 bytes) |
| **Get authorization data** | `84` | `EC` | `00` | `02` | → returns authorization data |
| **Clear authorization** | `84` | `EC` | `00` | `03` | Clear all authorization data |

### 3.13 Fingerprint Operations

| Description | CLA | INS | P1 | P2 | Data / Notes |
|:---|:---:|:---:|:---:|:---:|:---|
| **Get fingerprint state** | `84` | `EF` | `00` | `00` | → returns `0x55`=enabled, `0x00`=disabled |
| **Lock fingerprint** | `84` | `EF` | `00` | `01` | Lock the fingerprint module |
| **Unlock fingerprint** | `84` | `EF` | `00` | `02` | `Lc=1`: `0x01` → unlock the fingerprint module |

---

## Appendix A: Status Codes (SW1SW2)

| Code | Meaning |
|:---:|:---|
| `0x9000` | Success |
| `0x6A86` | Incorrect P1/P2 |
| `0x6700` | Incorrect Lc (data length) |
| `0x6C00` | Incorrect Le (expected length) |
| `0x6D00` | Incorrect INS |
| `0x6E00` | Incorrect CLA |
| `0x6A80` | Incorrect data |
| `0x6A87` | Incorrect P3 |
| `0x6982` | Security status not satisfied (e.g. PIN not verified) |
| `0x6985` | Conditions not satisfied |
| `0x6F00` | Execution error |
| `0x6F80` | Wipe code entered (device will be wiped) |
| `0x6901` | Data padding error |
| `0x6902` | Data MAC error |
| `0x6903` | Response length too long |

---

## Appendix B: Data Encoding Conventions

### BIP32 Path Encoding

Each path level is a **4-byte little-endian** `uint32`. Hardened derivation is indicated by setting the highest bit:

```text
hardened index = index | 0x80000000
```

Example: `m/44'/0'/0'/0/0`

```text
[2C000080] [00000080] [00000080] [00000000] [00000000]
  44'          0'         0'         0          0
```

### Curve Identifier

| Value | Curve |
|:---:|:---|
| `0x00` | NIST P-256 (nist256p1) |
| `0x01` | secp256k1 (Bitcoin) |

### Signature Format (65 bytes)

```text
[v (1B)] + [r (32B)] + [s (32B)]
```

Where `v` is the recovery ID. BIP340 Schnorr signatures return 64 bytes (`r + s`, no recovery ID).

---

## Appendix C: Omitted Commands

The following factory provisioning commands are **disabled in product mode** and are not included in this document:

| Description | INS | Reason for omission |
|:---|:---:|:---|
| Set device serial number | `F6` | Factory-only: writes device identity during manufacturing |
| Import device certificate | `F6` | Factory-only: provisions device certificate |
| Set session key | `F6` | Factory-only: provisions initial session key |
| Set device private key | `F6` | Factory-only: provisions device private key |
| Set product mode | `F8` | Factory-only: transitions device from factory to product mode (one-way) |
| Sign with device private key (FEITIAN) | `F5` | Vendor-specific variant, not used in product firmware |
| Get ECDH public key | `F5` | Factory-only: retrieves/locks ECDH public key during provisioning |
| Switch to Boot | `FC` | Firmware update entry point, not part of runtime command set |
| NEM AES256 Encrypt | `EA` | Deprecated chain-specific command |
| Lite Card ECDH | `E9` | Hardware-specific accessory command |

These commands are either hard-disabled by the product mode flag or irrelevant to the runtime security model. Reviewers can verify that product mode is active via `Get product mode` (`INS=0xF8, P1=0x04`) — a return value of `0x55` confirms factory commands are locked out.

---

## Appendix D: Independent Evaluation Guide

This section describes how a reviewer can independently verify the SE's behavior using only this document and a physical device, without access to SE source code.

### Prerequisites

- A OneKey Pro device (production unit)
- I2C access to the SE via MCU debug interface, or use the open-source MCU firmware as the transport layer
- A reference implementation of: ECDH (secp256k1), AES-128-ECB/CBC, SHA-256, ECDSA, BIP32, BIP39, BIP340

### Test 1: Secure Channel Establishment

1. Read SE public key from MCU OTP (or device certificate)
2. Implement the key agreement protocol (Section 2.2)
3. **Verify**: SE returns a valid ECDSA signature over `SHA256(r1)`, verifiable with the SE public key
4. **Verify**: Subsequent commands with incorrect MAC are rejected (`0x6902`)
5. **Verify**: Replaying a previous session's APDU is rejected

### Test 2: Device Authenticity

1. Read device certificate via `INS=0xF5, P2=0x02`
2. Read device public key via `INS=0xF5, P2=0x01`
3. Send a random 32-byte challenge via `INS=0xF5, P2=0x03`
4. **Verify**: Returned signature is valid against the device public key
5. **Verify**: Certificate chains to a known OneKey root CA

### Test 3: PIN Security

1. Set a PIN via `INS=0xE5, P2=0x01`
2. Attempt signing without PIN verification → expect `0x6982`
3. Verify PIN → expect `0x9000`
4. Attempt 10 wrong PINs → observe remaining attempts decrement via `P2=0x05`
5. **Verify**: Device locks after max attempts
6. Set wipe code → enter wipe code as PIN → **verify** device wipes (`0x6F80`)

### Test 4: BIP39 → BIP32 → ECDSA (Bitcoin)

1. Wipe device, import a **known test mnemonic** (e.g. BIP39 test vector)
2. Generate session seed with empty passphrase
3. Derive BIP32 node at `m/44'/0'/0'/0/0` with `Curve=0x01` (secp256k1)
4. Sign a known 32-byte digest
5. **Verify**: Signature is valid against the derived public key
6. **Verify**: Derived public key matches the expected output from a reference BIP32 implementation (e.g. `trezor-crypto`, `bitcoinjs-lib`)

### Test 5: BIP340 / Taproot (Schnorr)

1. Derive a BIP32 node at a Taproot path (e.g. `m/86'/0'/0'/0/0`)
2. Optionally tweak with a root hash via `INS=0xE8, P2=0x06`
3. Sign a digest via `INS=0xE8, P2=0x07`
4. **Verify**: 64-byte Schnorr signature is valid per BIP340

### Test 6: Wipe Verification

1. After completing signing tests, execute Wipe (`INS=0xE1`)
2. Attempt to derive any node → expect failure (`0x6985`)
3. **Verify**: No residual key material is accessible

### Test 7: FIDO/U2F

1. Generate FIDO root node (`INS=0xF9, P2=0x00`)
2. Register with a test AppID and Challenge (`P2=0x01`)
3. **Verify**: Returned KeyHandle + PubKey + Signature are consistent
4. Authenticate using the same AppID + KeyHandle + new Challenge (`P2=0x04`)
5. **Verify**: Counter increments, signature is valid

### Test 8: Firmware Integrity

1. Read firmware hash via `INS=0xF7, P2=0x02`
2. Read boot hash via `INS=0xF7, P2=0x05`
3. **Verify**: Hashes match the published firmware build (reproducible build artifacts in the `firmware-pro` repository)
