from alien_functions import *


def decode_alien(encoded):
    prefix = encoded[0:2]
    remainder = encoded[2:]
    if prefix == "11":
        # list
        a, cont = decode_alien(remainder)
        b, cont2 = decode_alien(cont)
        return cons(a)(b), cont2
    elif prefix == "01":
        # positive
        pos = remainder.find("0")
        if pos == 0:
            return 0, remainder[1:]
        else:
            number = remainder[pos + 1: pos + 1 + 4 * pos]
            return int(number, 2), remainder[pos + 1 + 4 * pos:]
    elif prefix == "10":
        # negative
        pos = remainder.find("0")
        if pos == 0:
            return 0, remainder[1:]
        else:
            number = remainder[pos + 1: pos + 1 + 4 * pos]
            return -int(number, 2), remainder[pos + 1 + 4 * pos:]
    elif prefix == "00":
        # empty list
        return nil, remainder


def encode_int(decoded):
    if decoded == 0:
        return '010'

    prefix = '01' if decoded >= 0 else '10'
    number = "{0:b}".format(abs(decoded))  # can be made faster?
    not_padding_size = len(number) % 4
    padding_size = 0 if not_padding_size == 0 else 4 - not_padding_size
    padded_number = '0' * padding_size + number
    length = len(padded_number) // 4
    return prefix + ('1' * length) + '0' + padded_number


def encode_alien(decoded) -> str:
    decoded = extract(decoded)
    if isinstance(decoded, int):
        # int
        return encode_int(decoded)
    elif isnil(decoded)(True)(False):
        # empty list
        return '00'
    else:
        # pair
        return '11' + encode_alien(car(decoded)) + encode_alien(cdr(decoded))


def decode_to_python(encoded):
    prefix = encoded[0:2]
    remainder = encoded[2:]
    if prefix == "11":
        # list
        a, cont = decode_to_python(remainder)
        b, cont2 = decode_to_python(cont)

        if isinstance(b, list):
            b.insert(0, a)
            return b, cont2
        else:
            return (a, b), cont2
    elif prefix == "01":
        # positive
        pos = remainder.find("0")
        if pos == 0:
            return 0, remainder[1:]
        else:
            number = remainder[pos + 1: pos + 1 + 4 * pos]
            return int(number, 2), remainder[pos + 1 + 4 * pos:]
    elif prefix == "10":
        # negative
        pos = remainder.find("0")
        if pos == 0:
            return 0, remainder[1:]
        else:
            number = remainder[pos + 1: pos + 1 + 4 * pos]
            return -int(number, 2), remainder[pos + 1 + 4 * pos:]
    elif prefix == "00":
        # empty list
        return [], remainder


def encode_from_python(decoded):
    if isinstance(decoded, list):
        if len(decoded) == 0:
            return '00'
        else:
            return '11' + encode_from_python(decoded[0]) + encode_from_python(decoded[1:])  # can optimize
    elif isinstance(decoded, tuple):
        return '11' + encode_from_python(decoded[0]) + encode_from_python(decoded[1])
    else:
        assert isinstance(decoded, int)
        return encode_int(decoded)