
def getFlags(entry):
    flags = {
        "Present":    (entry >> 31) & 1,
        "Supervisor": (entry >> 30) & 1,
        "Read":       (entry >> 29) & 1,
        "Write":      (entry >> 28) & 1,
        "Execute":    (entry >> 27) & 1,
        "Dirty":      (entry >> 26) & 1,
        "Accessed":   (entry >> 25) & 1
    }

    return flags


PGDIndex = 0xBFC // 4
PTEIndex = 0xFFC // 4

VA = (PGDIndex << 22) | (PTEIndex << 12)

print(f"Virtual Address: 0x{VA:08X}")

PTE = 0xB804_5678

print(f"Physical Address: 0x{(PTE & 0xFFFFF) << 12:08X}")

for flag, value in getFlags(PTE).items():
    print(f"{flag}: {value}")
