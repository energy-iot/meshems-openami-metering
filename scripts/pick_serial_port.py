Import("env")
# PlatformIO pre-script: set UPLOAD_PORT to a real USB UART.
# macOS exposes Bluetooth gadgets and debug endpoints as /dev/cu.* — we only allow known patterns.

try:
    from serial.tools import list_ports
except ImportError:
    list_ports = None


def _is_allowed_mac_port(low: str) -> bool:
    """True if device name looks like CH340 / CP210x / Espressif CDC, not BT/speakers."""
    if "ttyacm" in low:
        return True
    if "cu.bluetooth" in low or "incoming-port" in low:
        return False
    if "debug-console" in low or "wlan-debug" in low:
        return False
    if "usbmodem" in low:
        return True
    if "wchusbserial" in low:
        return True
    if "usbserial" in low and "bluetooth" not in low:
        return True
    if "slab" in low:
        return True
    return False


def _pick():
    if list_ports is None:
        return None
    scored = []
    for p in list_ports.comports():
        dev = p.device
        low = dev.lower()
        if not (dev.startswith("/dev/cu.") or "/dev/ttyACM" in dev or "/dev/tty.usb" in low):
            continue
        if not _is_allowed_mac_port(low):
            continue
        rank = 99
        if "usbmodem" in low:
            rank = 0
        elif "wchusbserial" in low:
            rank = 1
        elif "usbserial" in low:
            rank = 2
        elif "slab" in low:
            rank = 3
        elif "ttyacm" in low:
            rank = 0
        scored.append((rank, dev))
    if not scored:
        return None
    scored.sort(key=lambda x: (x[0], x[1]))
    return scored[0][1]


chosen = _pick()
if chosen:
    print("pick_serial_port: UPLOAD_PORT=%s" % chosen)
    env.Replace(UPLOAD_PORT=chosen)
else:
    print(
        "pick_serial_port: no USB serial (usbmodem / wchusbserial / SLAB). "
        "Plug the ESP32 data USB cable — pio device list must show a new cu.* line."
    )
