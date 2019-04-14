/*
 * Copyright (C) 2016 Florent Revest <florent.revest@free-electrons.com>
 * Copyright (C) 2018 Paul Kocialkowski <paul.kocialkowski@bootlin.com>
 * Copyright (C) 2019 Ralf Zerres <ralf.zerres@networkx.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define _GNU_SOURCE
#include <libudev.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include <linux/media.h>
#include <linux/videodev2.h>
#include "request.h"
#include "media.h"
#include "utils.h"

int driver_add(struct driver *driver, struct decoder *decoder)
{
	int rc = -1;

	/* make sure there's room to expand into */
	if (driver->capacity == driver->num_decoders) {
		rc = driver_extend(driver);
		if (rc < 0)
			return rc;
	}

	/* add the decoder and increment element counter */
	driver->decoder[driver->num_decoders++] = decoder;

	return decoder->id;
}

void driver_delete(struct driver *driver, int id)
{
    if (id < 0 || id >= driver->num_decoders)
	return;

    driver->decoder[id] = NULL;

    for (int i = id; i < driver->capacity - 1; i++) {
	driver->decoder[i] = driver->decoder[i + 1];
	driver->decoder[i + 1] = NULL;
    }

    driver->num_decoders--;

    driver_reduce(driver);
}

int driver_extend(struct driver *driver)
{
  //struct decoder *decoder;
	int new_capacity = driver->capacity + 1; // decoder increment = 1
	int decoder_index = new_capacity - 1;

	if (decoder_index < driver->capacity) {
		/* pointer to hold new pointer to 'struct decoder' */
		void **new_decoder;

		/*
		 * extend driver->capacity to hold another 'struct decoder'
		 * and resize the allocated memory accordingly
		 */
		request_log("driver_extend(): prepare new decoder entity '%i'\n", decoder_index);
		new_decoder =  realloc(driver->decoder,
					  sizeof(struct decoder) * new_capacity);
		if (new_decoder == NULL)
			return -1;

		driver->capacity = new_capacity;
		driver->decoder = new_decoder;
	}

	//decoder = calloc(1, sizeof(struct decoder));
	return 0;
}

void driver_free(struct driver *driver)
{
	int i;

	for (i = 0; i < driver->num_decoders; i++)
		free(driver->decoder[i]);

	free(driver->decoder);
	driver->decoder = NULL;
}

struct decoder *driver_get(struct driver *driver, int id)
{
	if (id > driver->num_decoders || id < 0) {
		printf("Id %d out of bounds for driver of %d entities\n", id, driver->num_decoders);
		return NULL;
	}

	return driver->decoder[id];
}

__u32 driver_get_capabilities(struct driver *driver, int id)
{
	struct decoder *decoder;

	if (id >= driver->num_decoders || id < 0) {
		request_log("Id '%d' out of bounds. Only handle %d decoders yet.\n",
		       id, driver->num_decoders);
		return -1;
	}

	decoder = (struct decoder *) driver->decoder[id];

	return decoder->capabilities;
}

int driver_get_first_id(struct driver *driver)
{
	struct decoder *decoder;

	decoder = (struct decoder *) driver->decoder[0];

	return decoder->id;
}

int driver_get_last_id(struct driver *driver)
{
	struct decoder *decoder;

	decoder = (struct decoder *)driver->decoder[driver->num_decoders];

	return decoder->id;
}

int driver_get_num_decoders(struct driver *driver)
{
	return driver->num_decoders;
}

void driver_init(struct driver *driver)
{
	driver->num_decoders = 0;
	driver->capacity = 1;

	// allocate memory for first decoder array
	driver->decoder = calloc(driver->capacity, sizeof(struct decoder));
}

void driver_print(struct driver *driver, int id)
{
	if (id >= driver->num_decoders || id < 0) {
		request_log("Id '%d' out of bounds. Only handle %d decoders yet.\n",
		       id, driver->num_decoders);
	}

	struct decoder *decoder = driver_get(driver, id);
	request_log(" decoder[%i]: %s (id: %i, media_path: %s, video_path: %s, capabilities: %ld)\n",
		    id,
		    decoder->name,
		    decoder->id,
		    decoder->media_path,
		    decoder->video_path,
		    decoder->capabilities);
}

void driver_print_all(struct driver *driver)
{
	request_log("Driver: num_decoders: %d, capacity: %d\n",
		    driver->num_decoders,
		    driver->capacity);
	for (int i = 0; i < driver_get_num_decoders(driver); i++) {
		struct decoder *decoder = driver_get(driver, i);
		request_log("decoder[%i]: %s (id: %i, media_path: %s, video_path: %s, capabilities: %ld)\n",
			    i,
			    decoder->name,
			    decoder->id,
			    decoder->media_path,
			    decoder->video_path,
			    decoder->capabilities);
	}
}

void driver_reduce(struct driver *driver)
{
	if (driver->num_decoders > 0 &&
	    driver->num_decoders == driver->capacity -1) {
		/*
		 * reduce driver->capacity
		 * and resize the allocated memory accordingly
		 */
		driver->capacity--;
		driver->decoder = realloc(driver->decoder, sizeof(struct decoder) * driver->capacity);
	}
}

void driver_set(struct driver *driver, int id, void *decoder)
{
	/* zero fill the driver up to the desired id */
	while (id >= driver->num_decoders) {
		driver_add(driver, decoder);
	}

	/* memcopy the given source structure at the desired id */
	driver->decoder[id] = decoder;
}

__u32 driver_set_capabilities(struct driver *driver, int id, unsigned int capabilities)
{
	struct decoder *decoder;

	if (id >= driver->num_decoders || id < 0) {
		request_log("Id '%d' out of bounds. Only handle %d decoders yet.\n",
		       id, driver->num_decoders);
		return -1;
	}

	decoder = (struct decoder *) driver->decoder[id];

	/* memcopy the given source structure at the desired id */
	decoder->capabilities = capabilities;

	return 0;
}

void request_log(const char *format, ...)
{
	va_list arguments;

	fprintf(stderr, "%s: ", V4L2_REQUEST_STR_VENDOR);

	va_start(arguments, format);
	vfprintf(stderr, format, arguments);
	va_end(arguments);
}

char *udev_get_devpath(struct media_v2_intf_devnode *devnode)
{
	struct udev *udev;
	struct udev_device *device;
	dev_t devnum;
	const char *ptr_devname;
	char *devname = NULL;
	int rs;

	udev = udev_new();
	if (!udev) {
		request_log("Can’t create udev object.\n");
		return NULL;
	}

	devnum = makedev(devnode->major, devnode->minor);
	device = udev_device_new_from_devnum(udev, 'c', devnum);
	if (device) {
		ptr_devname = udev_device_get_devnode(device);
		if (ptr_devname) {
			rs = asprintf(&devname, "%s", ptr_devname);
			if (rs < 0)
				return NULL;
		}
	}

	udev_device_unref(device);

	return devname;
}

int udev_scan_subsystem(struct driver *driver, char *subsystem)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;
	struct decoder *decoder;
	int rc = -1;
	int i = 0;
	int y = 0;

	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		printf("Can’t create udev\n");
		return -1;
	}

	/* Create a list of sys devices for given subsystem */
	enumerate = udev_enumerate_new(udev);
	rc = udev_enumerate_add_match_subsystem(enumerate, subsystem);
	if (rc < 0) {
		request_log("udev: can't filter sysbsystem %s.\n",
			subsystem);
		return rc;
	}
	rc = udev_enumerate_scan_devices(enumerate);
	if (rc < 0) {
		request_log("udev: can't scan devices.\n");
		return rc;
	}
	devices = udev_enumerate_get_list_entry(enumerate);

	/* For each item enumerated, print out its information.
	   udev_list_entry_foreach is a macro which expands to
	   a loop. The loop will be executed for each member in
	   devices, setting dev_list_entry to a list entry
	   which contains the device’s path in /sys.
	*/
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;
		const char *node_path;
		const char *node_name;

		/* Get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it
		*/
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);
		node_path = udev_device_get_devnode(dev);
		node_name = udev_device_get_sysname(dev);

		/* From here, we can call get_sysattr_value() for each file
		   in the device’s /sys entry. The strings passed into these
		   functions (idProduct, idVendor, serial, etc.) correspond
		   directly to the files in the directory which represents
		   the device attributes. Strings returned from
		   udev_device_get_sysattr_value() are UTF-8 encoded.
		*/

		/* initilize decoder pointer */
		decoder = calloc(1, sizeof(struct decoder));

		if (strncmp(node_name, "media", 5) == 0) {
			asprintf(&decoder->media_path, "%s", node_path);
			udev_device_get_sysattr_value(dev, "model");
			request_log("udev node: [%i]\n", i);

			// update media_path in driver structure
			driver_set(driver, y, decoder);

			/* use media topology to select capable video-decoders. */
			rc = media_scan_topology(driver, y, node_path);
			if (rc <= 0) {
				request_log("model '%s' doesn't offer streaming via v4l2 video-decoder.\n",
					udev_device_get_sysattr_value(dev, "model"));
				driver_delete(driver, y);
				}
			else if (rc == 1) {
				request_log(" model '%s' offers streaming via v4l2 video-decoder.\n",
					udev_device_get_sysattr_value(dev, "model"));
				driver_print(driver, y++);
			}
		}

		i++;
		udev_device_unref(dev);
	}

	/* Free unnedded stuctures */
	if (driver)
		free((void *)driver);
	if (decoder)
		free((void *)decoder);

	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	return rc;
}
