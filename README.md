# Sunxi-Cedrus libVA Backend

## About

This libVA backend is designed to work with the Sunxi-Cedrus VPU kernel driver,
that supports the Video Engine found in most Allwinner SoCs.

## Status

The Sunxi-Cedrus libVA backend currently only supports MPEG2 video decoding.

## Instructions

In order to use the Sunxi-Cedrus libVA backend, the `sunxi_cedrus` driver has to
be specified through the `LIBVA_DRIVER_NAME` environment variable, as such:

	export LIBVA_DRIVER_NAME=sunxi_cedrus

A media player that supports VAAPI (such as VLC) can then be used to decode a
video in a supported format:

	vlc path/to/video.mpg

Sample media files can be obtained from:

	http://samplemedia.linaro.org/MPEG2/
	http://samplemedia.linaro.org/MPEG4/SVT/
