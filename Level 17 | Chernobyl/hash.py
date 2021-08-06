VALUE_AT = {  # maps the location of the hash list address in memory to the hash list address
    int("5016", 16): int("5042", 16),
    int("5018", 16): int("50a2", 16),
    int("501a", 16): int("5102", 16),
    int("501c", 16): int("5162", 16),
    int("501e", 16): int("51c2", 16),
    int("5020", 16): int("5222", 16),
    int("5022", 16): int("5282", 16),
    int("5024", 16): int("52e2", 16),
}


def get_hash(string):  # calculates the hash of a string of bytes in the same way the level does
    ret = 0
    ret_hex = ""
    for byte in string:
        old_hash_plus_byte = ret + byte
        ret += byte
        ret_hex = hex(ret)
        ret *= 32
        ret_hex = hex(ret)
        ret -= old_hash_plus_byte
        ret_hex = hex(ret)

    return ret, ret_hex


def get_offset(string_hash):  # calculates the location in memory a value will end up at, given its key's hash
    r12 = 2 * (string_hash & 7)  # 7 = 2^3 - 1 | only valid until a rehash is run, but that's good enough
    r15 = int("502c", 16) + r12
    r11 = int("5016", 16) + r12

    r14 = VALUE_AT[r15] if r15 in VALUE_AT else 0
    r12 = 2 * (8 * r14 + r14)
    r12 += VALUE_AT[r11]

    r14 += 1
    VALUE_AT[r15] = r14

    return r12


def main(name):
    h, hs = get_hash(name)
    print(hs)
    print(hex(get_offset(h)))
    print()


if __name__ == '__main__':
    main([8])
    main([16])
    main([24])
    main([40])
    main([48])
    main([int("ca", 16), int("3d", 16), int("fc", 16), int("50", 16), int("ab", 16), int("f4", 16)])  # overwrite headers for second chunk

    """# add more useless entries to trigger rehash
    main([3])
    main([11])
    main([19])
    main([27])
    main([35])
    main([43])
    main([51]) #rehash!"""
