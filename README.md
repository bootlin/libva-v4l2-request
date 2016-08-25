Sunxi Cedrus VA backend
=======================

This libVA video driver is designed to work with the v4l2 "sunxi-cedrus" kernel
driver. However, the only sunxi-cedrus specific part is the format conversion
from tiled to planar, otherwise it would be generic enough to be used with other
v4l2 drivers using the "Frame API".

You can try this driver with VLC but don't forget to tell libVA to use this
backend:

	export LIBVA_DRIVER_NAME=sunxi_cedrus
	vlc big_buck_bunny_480p_MPEG2_MP2_25fps_1800K.MPG

Sample media files can be found here:

	http://samplemedia.linaro.org/MPEG2/
