#!/usr/bin/env python3
# verify_pt1_frame.py — 对齐 MachinePeer 与 SerialPortComm PT1 组解包

from __future__ import annotations


def hex_to_float_ascii(hex_latin1: bytes) -> str:
    mapping = {
        b"2D": "-",
        b"2E": ".",
        b"30": "0",
        b"31": "1",
        b"32": "2",
        b"33": "3",
        b"34": "4",
        b"35": "5",
        b"36": "6",
        b"37": "7",
        b"38": "8",
        b"39": "9",
    }
    out: list[str] = []
    for i in range(0, len(hex_latin1) - 1, 2):
        pair = hex_latin1[i : i + 2]
        if pair in mapping:
            out.append(mapping[pair])
    return "".join(out)


def encode_kexin_measurement_body(force: float) -> bytes:
    numeric = f"{abs(force):.4f}"
    if force < 0.0:
        pad = 8 - len(numeric)
        digits = ("0" * pad + numeric) if pad > 0 else numeric[-8:]
        body = "-" + digits
    else:
        pad = 9 - len(numeric)
        body = ("0" * pad + numeric) if pad > 0 else numeric[-9:]
    return body.encode("latin1")


def pack_pt1_measurement(force: float) -> bytes:
    payload = encode_kexin_measurement_body(force)
    assert len(payload) == 9
    hex_latin1 = payload.hex().upper().encode("ascii")
    assert len(hex_latin1) == 18
    decoded = hex_to_float_ascii(hex_latin1)
    assert abs(float(decoded) - force) <= 1e-3
    return bytes([0x02, 0x4C]) + payload


def encode_fixed_double_to_hex(force: float) -> bytes:
    # 对齐 SerialPortComm::FixedDoubleToHex(iRowNum=0)
    frame = bytearray(14)
    s_data = f"{force:.5f}"
    if "." not in s_data:
        for i in range(2, 13):
            frame[i] = 0x30
        frame[7] = 0x2E
    else:
        dot = s_data.index(".")
        start = dot - 5 if dot > 4 else 0
        s_integer = s_data[start:dot]
        if force < 0:
            s_integer = s_integer[1:]
        s_decimal = s_data[dot + 1 :] if dot + 1 < len(s_data) else ""
        s_integer = s_integer.zfill(5)
        s_decimal = (s_decimal + "00000")[:5]
        for i in range(5):
            ch = s_integer[i]
            frame[2 + i] = ord("-") if force < 0 and i == 0 else ord(ch)
        frame[7] = ord(".")
        for i in range(5):
            frame[8 + i] = ord(s_decimal[i])
    frame[13] = ord("#")
    return bytes(frame)


def decode_fixed_double_to_hex(raw: bytes) -> tuple[bool, float | None, str]:
    if len(raw) < 14:
        return False, None, f"len={len(raw)}"
    frame = raw[:14]
    if frame[13] != ord("#"):
        return False, None, "missing #"
    if frame[7] != ord("."):
        return False, None, "missing dot"
    field = frame[2:13]
    try:
        return True, float(field.decode("latin1")), field.decode("latin1")
    except ValueError as exc:
        return False, None, str(exc)


def main() -> None:
    print("=== PT1 测数组包（HexToFloat 往返） ===")
    for force in [0.0, 1.0, 12.34, -1.0]:
        frame = pack_pt1_measurement(force)
        print(f"force={force:>8} len={len(frame)} hex={frame.hex().upper()}")

    print("\n=== PT1 回包编解码（FixedDoubleToHex 14B） ===")
    for force in [0.0, 1.0, 12.34, -1.0]:
        raw = encode_fixed_double_to_hex(force)
        ok, val, field = decode_fixed_double_to_hex(raw)
        print(f"force={force:>8} ok={ok} parsed={val} field={field!r} hex={raw.hex().upper()}")

    print("\n=== 粘包：两帧 14B ===")
    buf = encode_fixed_double_to_hex(1.0) + encode_fixed_double_to_hex(2.0)
    frames = [buf[i : i + 14] for i in range(0, len(buf), 14)]
    for idx, frame in enumerate(frames):
        ok, val, _ = decode_fixed_double_to_hex(frame)
        print(f"frame{idx} ok={ok} force={val}")


if __name__ == "__main__":
    main()
