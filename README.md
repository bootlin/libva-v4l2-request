# v4l2-request libVA Backend

## About

This libVA backend is designed to work with the Linux Video4Linux2
Request API that is used by a number of video codecs drivers,
including the Video Engine found in most Allwinner SoCs.

## Status

The v4l2-request libVA backend currently supports the following formats:
* MPEG2 (Simple and Main profiles)
* H264 (Baseline, Main and High profiles)

## Instructions

In order to use this libVA backend, the `v4l2_request` driver has to
be specified through the `LIBVA_DRIVER_NAME` environment variable, as
such:

	export LIBVA_DRIVER_NAME=v4l2_request

A media player that supports VAAPI (such as VLC) can then be used to decode a
video in a supported format:

	vlc path/to/video.mpg

Sample media files can be obtained from:

	http://samplemedia.linaro.org/MPEG2/
	http://samplemedia.linaro.org/MPEG4/SVT/

## Technical Notes

### Surface

A Surface is an internal data structure never handled by the VA's user
containing the output of a rendering. Usualy, a bunch of surfaces are created
at the begining of decoding and they are then used alternatively. When
created, a surface is assigned a corresponding v4l capture buffer and it is
kept until the end of decoding. Syncing a surface waits for the v4l buffer to
be available and then dequeue it.

Note: since a Surface is kept private from the VA's user, it can ask to
directly render a Surface on screen in an X Drawable. Some kind of
implementation is available in PutSurface but this is only for development
purpose.

### Context

A Context is a global data structure used for rendering a video of a certain
format. When a context is created, input buffers are created and v4l's output
(which is the compressed data input queue, since capture is the real output)
format is set.

### Picture

A Picture is an encoded input frame made of several buffers. A single input
can contain slice data, headers and IQ matrix. Each Picture is assigned a
request ID when created and each corresponding buffer might be turned into a
v4l buffers or extended control when rendered. Finally they are submitted to
kernel space when reaching EndPicture.

The real rendering is done in EndPicture instead of RenderPicture
because the v4l2 driver expects to have the full corresponding
extended control when a buffer is queued and we don't know in which
order the different RenderPicture will be called.

### Image

An Image is a standard data structure containing rendered frames in a usable
pixel format. Here we only use NV12 buffers which are converted from sunxi's
proprietary tiled pixel format with tiled_yuv when deriving an Image from a
Surface.
