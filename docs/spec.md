# Payment Frame Protocol Spec

## 1. Frame Structure

```
+----------------+---------------------+
| length (2 B)   | body (length bytes) |
+----------------+---------------------+
```

- `length` is the size of the **body only**, not the whole frame.
- Total frame size = 2 (length field) + `length`.
- Maximum body size = **1024** bytes.
- Minimum valid body = **3** bytes (message type 2 + bitmap 1).

### Length validation

| Condition | Meaning | Action |
|---|---|---|
| `length < 3` (including 0) | Too short to hold message type + bitmap, invalid | Reject, close connection |
| `length > 1024` | Oversize, invalid (likely malicious or corrupt) | Reject, close connection. Do **not** allocate a buffer for it |

## 2. Body Structure

```
+------------------+--------------+------------------------------+
| message type (2) | bitmap (1)   | fields (variable, per bitmap)|
+------------------+--------------+------------------------------+
```

### Message types (2 bytes each, hex-coded)

| Type | Meaning |
|---|---|
| `0x0100` | Auth request |
| `0x0110` | Auth response |
| `0x0400` | Reversal request |
| `0x0410` | Reversal response |

An unknown message type is an error.

### Bitmap (1 byte)

Indicates which of the 8 fields are present. Each field has exactly one bit; no two fields share a bit position.

**Mapping (must be fixed explicitly):** MSB (bit 7) = field 0, ..., LSB (bit 0) = field 7.

```
bit:    7  6  5  4  3  2  1  0
field:  0  1  2  3  4  5  6  7
```

Example: `01010010` -> bits 6, 4, 1 set -> fields **1, 3, 6** present (amount, timestamp, original STAN).

Example: `11000000` -> fields 0 and 1 present, so card number and amount MUST both be in the body.

## 3. Fields

Fields are decoded in **ascending field order** (field 0 first), only for set bits.

| # | Field | Size | Encoding | Notes |
|---|---|---|---|---|
| 0 | Card number (PAN) | 1-byte length subfield + 12 to 19 digits | ASCII digits | Digits only. Length subfield must be in 12..19 |
| 1 | Amount | 8 bytes | Binary integer + decimal scale | See below. INR only for now |
| 2 | STAN (trace number) | 4 bytes | Binary integer | |
| 3 | Timestamp | 8 bytes | Binary integer | Define unit (e.g. epoch ms) |
| 4 | Idempotency key | 1-byte length subfield + max 64 bytes | ASCII / UTF-8 | Store the key and its processing result |
| 5 | Response code | 2 bytes | 2 ASCII digits | See response codes |
| 6 | Original STAN | 4 bytes | Binary integer | Used in reversals |
| 7 | Terminal ID | 1-byte length subfield + max 16 bytes | ASCII | |

### Amount (field 1)

The amount carries a stored integer and a decimal scale, which says where the decimal point goes.

| Scale | Stored | Actual amount |
|---|---|---|
| 0 | 12550 | 12550 |
| 1 | 12550 | 1255.0 |
| 2 | 12550 | 125.50 |

- Currency is fixed to INR, so no currency field is needed.
- **Open decision:** the exact layout inside the 8 bytes (e.g. 1 byte scale + 8 bytes unsigned integer).

## 4. Response Codes

| Code | Meaning |
|---|---|
| 00 | Approved |
| 05 | Do not honour (issuer declines) |
| 12 | Invalid transaction |
| 13 | Invalid amount |
| 30 | Format error (frame cannot be interpreted) |
| 51 | Insufficient funds |
| 54 | Expired card |
| 57 | Transaction not permitted |
| 91 | Issuer or switch unavailable |
| 96 | System malfunction |

## 5. Validation Rules

- **Bitmap vs. fields must agree.** Every set bit requires its field to be present and well-formed.
- **Leftover bytes after the last field = error.** After decoding all indicated fields, the parser must have consumed exactly the entire body.
- **Not enough bytes for a field = error.**
- Field-level checks: PAN is digits only with length 12..19, key and terminal ID lengths within their maximums, response code is two ASCII digits.
- Suggested message-specific checks: a reversal request must include Original STAN (field 6); a response must include Response code (field 5).

## 6. Invalid Frame Handling

If a frame is invalid, **close the connection**. It cannot be resynchronized.

**Why resync is impossible:** TCP is a continuous byte stream with no message boundaries. The only thing marking where one frame ends and the next begins is the length field. If that length is corrupt or malicious, the receiver cannot know where the next frame starts, and any attempt to continue would parse arbitrary bytes as a frame header.

(Note: receiving fewer bytes than `length` says is not itself an error. It just means the rest hasn't arrived yet, so keep reading until it does, or time out.)

## 7. Parsing Flow

1. Read the 2-byte frame length.
2. Validate the length (3 <= length <= 1024).
3. Read the complete body (exactly `length` bytes).
4. Read and validate the message type and bitmap.
5. For each set bit, in field order, decode the field and validate its length and value.
6. Check that no bytes remain unread.
7. Only then pass the decoded message to transaction-processing logic.

## 8. To Be Addressed Later

- **Tampering:** what if someone modifies the frame (e.g. swaps in a different PAN)? Needs integrity protection (e.g. a MAC or TLS).
- Idempotency handling: storage, expiry, and what to return on a duplicate key.
- Which fields are required for which requests ?
- In common/socket_utils.cpp...helper function needed