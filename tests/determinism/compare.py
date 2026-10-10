
"""Runs the determinism scenario in the native build and in the WebAssembly
build (with Node) and fails unless both print the same lines."""

import argparse
import subprocess
import sys


def run(command):
	return subprocess.run(command, check=True, capture_output=True,
		text=True).stdout.splitlines()


def main():
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--native", required=True)
	parser.add_argument("--node", required=True)
	parser.add_argument("--web", required=True)
	arguments = parser.parse_args()
	native = run([arguments.native])
	web = run([arguments.node, arguments.web])
	if not native:
		print("the native build printed nothing")
		return 1
	for number, (a, b) in enumerate(zip(native, web), start=1):
		if a != b:
			print(f"line {number} differs: native {a!r}, web {b!r}")
			return 1
	if len(native) != len(web):
		print(f"the native build printed {len(native)} lines, the web "
			f"build {len(web)}")
		return 1
	print(f"{len(native)} lines, identical; last: {native[-1]}")
	return 0


if __name__ == "__main__":
	sys.exit(main())
