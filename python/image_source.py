#!/usr/bin/env python
# vim: tabstop=4:softtabstop=4:shiftwidth=4:noexpandtab:
# -*- coding: utf-8 -*-
# 
# Copyright 2015 Chris Kuethe <chris.kuethe@gmail.com>
# 
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

import numpy
from gnuradio import gr
from PIL import Image
from PIL import ImageOps
import pmt

class image_source(gr.sync_block):
	"""
	Given an image file readable by Python-Imaging, this block produces
	monochrome lines suitable for input to spectrum_paint
	"""
	def __init__(self, image_file, image_flip=False, bt709_map=True, image_invert=False, autocontrast=False):
		gr.sync_block.__init__(self,
			name="image_source",
			in_sig=None,
			out_sig=[numpy.uint8])

		im = Image.open(image_file)
		im = ImageOps.grayscale(im)

		if autocontrast:
			# may or may not improve the look of the transmitted spectrum
			im = ImageOps.autocontrast(im)

		if image_invert:
			# may or may not improve the look of the transmitted spectrum
			im = ImageOps.invert(im)

		if image_flip:
			# set to true for waterfalls that scroll from the top
			im = ImageOps.flip(im)

		(self.image_width, self.image_height) = im.size
		max_width = 2048.0
		if self.image_width > max_width:
			scaling = max_width / self.image_width
			newsize = (int(self.image_width * scaling), int(self.image_height * scaling))
			(self.image_width, self.image_height) = newsize
			im = im.resize(newsize)
		self.set_output_multiple(self.image_width)

		self.image_data = list(im.getdata())
		if bt709_map:
			# scale brightness according to ITU-R BT.709
			self.image_data = map( lambda x: x * 219 / 255 + 16,  self.image_data)
		self.image_len = len(self.image_data)
		print "paint.image_source: %d bytes, %dpx width" % (self.image_len, self.image_width)
		self.line_num = 0

	def work(self, input_items, output_items):
		out = output_items[0]
		self.add_item_tag(0, self.nitems_written(0), pmt.intern("image_width"), pmt.from_long(self.image_width))
		self.add_item_tag(0, self.nitems_written(0), pmt.intern("line_num"), pmt.from_long(self.line_num))
		out[:self.image_width] = self.image_data[self.image_width*self.line_num: self.image_width*(1+self.line_num)]

		self.line_num += 1
		if self.line_num >= self.image_height:
			self.line_num = 0
		return self.image_width
