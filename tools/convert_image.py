# -*- coding: utf-8 -*-
from PIL import Image
import random
import argparse
import sys



def convert(input_file, output_file):
	im = Image.open(input_file)
	im = im.convert('RGB')

	output_file.write('U')
	for y in range(480):
		for x in range(320):
			r, g, b = im.getpixel((x, y))
			r = r*252//256 + random.randint(0, 4)
			g = g*252//256 + random.randint(0, 4)
			b = b*252//256 + random.randint(0, 4)
			output_file.write('{:02x}{:02x}{:02x}'.format(r, g, b))


def main():
	parser = argparse.ArgumentParser(description='Convert image to uploadable data (cat data.raw > /dev/ttyUSB0)')
	parser.add_argument('infile', type=argparse.FileType('rb'), default=sys.stdin)
	parser.add_argument('outfile', type=argparse.FileType('w'), default=sys.stdout)
	args = parser.parse_args()
	convert(args.infile, args.outfile)

if __name__ == "__main__":
	main()
