#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
# SPDX-FileCopyrightText: Sven Eckelmann <sven@narfation.org>

import argparse
import json
import math
import sys


def load(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def numbers_equal(a, b, abs_tol):
    # exact match for integer fields (frequency, tsf, rssi, noise, ...)
    if isinstance(a, int) and isinstance(b, int):
        return a == b
    return math.isclose(a, b, rel_tol=0.0, abs_tol=abs_tol)


def is_number(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def compare(expected, actual, abs_tol, path, errors):
    if is_number(expected) and is_number(actual):
        if not numbers_equal(expected, actual, abs_tol):
            errors.append(f"{path}: expected {expected!r}, got {actual!r}")
        return

    if type(expected) is not type(actual):
        errors.append(
            f"{path}: expected type {type(expected).__name__}, "
            f"got {type(actual).__name__}"
        )
        return

    if isinstance(expected, list):
        if len(expected) != len(actual):
            errors.append(
                f"{path}: expected {len(expected)} entries, got {len(actual)}"
            )
            return
        for i, (e, a) in enumerate(zip(expected, actual)):
            compare(e, a, abs_tol, f"{path}[{i}]", errors)
    elif isinstance(expected, dict):
        if expected.keys() != actual.keys():
            errors.append(
                f"{path}: expected keys {sorted(expected)}, got {sorted(actual)}"
            )
            return
        for key in expected:
            compare(expected[key], actual[key], abs_tol, f"{path}.{key}", errors)
    elif expected != actual:
        errors.append(f"{path}: expected {expected!r}, got {actual!r}")


def main():
    parser = argparse.ArgumentParser(
        description="Compare fft_eval_json output with float tolerance"
    )
    parser.add_argument("expected", help="reference JSON file")
    parser.add_argument("actual", help="JSON file to check")
    parser.add_argument(
        "--abs-tol",
        type=float,
        default=1e-3,
        help="allowed absolute difference of floats (default: %(default)s)",
    )
    parser.add_argument(
        "--max-errors",
        type=int,
        default=10,
        help="number of differences to print (default: %(default)s)",
    )
    args = parser.parse_args()

    try:
        expected = load(args.expected)
        actual = load(args.actual)
    except (OSError, ValueError) as e:
        print(f"{sys.argv[0]}: {e}", file=sys.stderr)
        return 2

    errors = []
    compare(expected, actual, args.abs_tol, "$", errors)
    if not errors:
        return 0

    print(
        f"{args.actual} differs from {args.expected} ({len(errors)} differences):",
        file=sys.stderr,
    )
    for error in errors[: args.max_errors]:
        print(f"  {error}", file=sys.stderr)
    if len(errors) > args.max_errors:
        print("  ...", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
