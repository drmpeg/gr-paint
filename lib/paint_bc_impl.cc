/* -*- c++ -*- */
/* 
 * Copyright 2015 Ron Economos.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "paint_bc_impl.h"
#include <volk/volk.h>
#include <stdio.h>

namespace gr {
  namespace paint {

    paint_bc::sptr
    paint_bc::make(int width, int repeats)
    {
      return gnuradio::get_initial_sptr
        (new paint_bc_impl(width, repeats));
    }

    /*
     * The private constructor
     */
    paint_bc_impl::paint_bc_impl(int width, int repeats)
      : gr::block("paint_bc",
              gr::io_signature::make(1, 1, sizeof(unsigned char)),
              gr::io_signature::make(1, 1, sizeof(gr_complex)))
    {
        line_repeat = repeats;
        image_width = width;
        ofdm_fft_size = 4096;
        ofdm_fft = new fft::fft_complex(ofdm_fft_size, false, 1);
        normalization = 0.000001;
        pixel_repeat = ofdm_fft_size / image_width;
        int nulls = ofdm_fft_size - (image_width * pixel_repeat);
        left_nulls = nulls / 2;
        right_nulls = nulls / 2;
        if (nulls % 2 == 1)
        {
            left_nulls++;
        }
        set_output_multiple(ofdm_fft_size * line_repeat);
    }

    /*
     * Our virtual destructor.
     */
    paint_bc_impl::~paint_bc_impl()
    {
        delete ofdm_fft;
    }

    void
    paint_bc_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
        ninput_items_required[0] = (noutput_items * (image_width / line_repeat)) / ofdm_fft_size;
    }

    int
    paint_bc_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
        const unsigned char *in = (const unsigned char *) input_items[0];
        gr_complex *out = (gr_complex *) output_items[0];
        int consumed = 0;
        float angle, magnitude;
        gr_complex zero;
        gr_complex *dst;

        zero.real() = 0.0;
        zero.imag() = 0.0;

        for (int i = 0; i < noutput_items; i += (ofdm_fft_size * line_repeat))
        {
            for (int j = 0; j < left_nulls; j++)
            {
                *out++ = zero;
            }
            for (int j = 0; j < image_width; j++)
            {
                angle = rand();
                angle = angle - (RAND_MAX / 2);
                magnitude = in[consumed++];
                magnitude += 256.0;
                magnitude = pow(magnitude, 5.0);
                magnitude /= 10000000000.0;
                m_point[0].real() = (magnitude * cos(angle * M_PI / (RAND_MAX / 2)));
                m_point[0].imag() = (magnitude * sin(angle * M_PI / (RAND_MAX / 2)));
                for (int repeat = 0; repeat < pixel_repeat; repeat++)
                {
                    *out++ = m_point[0];
                }
            }
            for (int j = 0; j < right_nulls; j++)
            {
                *out++ = zero;
            }
            out -= ofdm_fft_size;
            dst = ofdm_fft->get_inbuf();
            memcpy(&dst[ofdm_fft_size / 2], &out[0], sizeof(gr_complex) * ofdm_fft_size / 2);
            memcpy(&dst[0], &out[ofdm_fft_size / 2], sizeof(gr_complex) * ofdm_fft_size / 2);
            ofdm_fft->execute();
            for (int repeat = 0; repeat < line_repeat; repeat++)
            {
                volk_32fc_s32fc_multiply_32fc(out, ofdm_fft->get_outbuf(), normalization, ofdm_fft_size);
                out += ofdm_fft_size;
            }
        }
        // Tell runtime system how many input items we consumed on
        // each input stream.
        consume_each (consumed);

        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

  } /* namespace paint */
} /* namespace gr */

