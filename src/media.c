/*
 * Copyright (C) 2018 Paul Kocialkowski <paul.kocialkowski@bootlin.com>
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
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/media.h>
#include <linux/videodev2.h>

#include "media.h"
#include "utils.h"
#include "v4l2.h"

enum media_gobj_type {
	MEDIA_GRAPH_ENTITY,
	MEDIA_GRAPH_PAD,
	MEDIA_GRAPH_LINK,
	MEDIA_GRAPH_INTF_DEVNODE,
};

static inline void *media_get_uptr(__u64 arg)
{
	return (void *)(uintptr_t)arg;
}

static inline const char *media_interface_type(uint32_t intf_type)
{
	switch (intf_type) {
	case MEDIA_INTF_T_DVB_FE:
		return "frontend";
	case MEDIA_INTF_T_DVB_DEMUX:
		return "demux";
	case MEDIA_INTF_T_DVB_DVR:
		return "DVR";
	case MEDIA_INTF_T_DVB_CA:
		return  "CA";
	case MEDIA_INTF_T_DVB_NET:
		return "dvbnet";

	case MEDIA_INTF_T_V4L_VIDEO:
		return "video";
	case MEDIA_INTF_T_V4L_VBI:
		return "vbi";
	case MEDIA_INTF_T_V4L_RADIO:
		return "radio";
	case MEDIA_INTF_T_V4L_SUBDEV:
		return "v4l2-subdev";
	case MEDIA_INTF_T_V4L_SWRADIO:
		return "swradio";

	case MEDIA_INTF_T_ALSA_PCM_CAPTURE:
		return "pcm-capture";
	case MEDIA_INTF_T_ALSA_PCM_PLAYBACK:
		return "pcm-playback";
	case MEDIA_INTF_T_ALSA_CONTROL:
		return "alsa-control";
	case MEDIA_INTF_T_ALSA_COMPRESS:
		return "compress";
	case MEDIA_INTF_T_ALSA_RAWMIDI:
		return "rawmidi";
	case MEDIA_INTF_T_ALSA_HWDEP:
		return "hwdep";
	case MEDIA_INTF_T_ALSA_SEQUENCER:
		return "sequencer";
	case MEDIA_INTF_T_ALSA_TIMER:
		return "ALSA timer";
	default:
		return "unknown_intf";
	}
}

/*
static inline uint32_t media_localid(uint32_t id)
{
	return id & 0xffffff;
}
*/

int media_scan_topology(struct driver *driver, int id, const char *path)
{
	/* https://www.kernel.org/doc/html/v5.0/media/uapi/mediactl/media-ioc-g-topology.html */
	struct media_v2_topology *topology = NULL;
	struct media_device_info *device = NULL;
	struct decoder *decoder = NULL;
	int media_fd = -1;
	int video_fd = -1;
	int i = 0;
	int j = 0;
	int rc = -1;
	bool is_decoder = false;
	__u64 topology_version;
	unsigned int capabilities;
	unsigned int capabilities_required;


	decoder = driver->decoder[id];
	request_log("Scan topology for media-device %s ...\n",
		    path);

	media_fd = open(path, O_RDWR | O_NONBLOCK, 0);
	if (media_fd < 0) {
		request_log("Unable to open media node: %s (%d)\n",
			strerror(errno), errno);
		return rc;
	}

	device = calloc(1, sizeof(struct media_device_info));
	rc = ioctl(media_fd, MEDIA_IOC_DEVICE_INFO, device);
	if (rc < 0) {
		request_log(" error: media device info can't be initialized!\n");
		return rc;
	}

	request_log(" driver: %s (model: %s, bus: %s, api-version: %d, driver-version: %d)\n",
	       device->driver,
	       device->model,
	       device->bus_info,
	       device->media_version,
	       device->driver_version);

	/*
	 * Initialize topology structure
	 * a) zero the topology structure
	 * b) ioctl to get amount of elements
	 */
	topology = calloc(1, sizeof(struct media_v2_topology));

	rc = ioctl(media_fd, MEDIA_IOC_G_TOPOLOGY, topology);
	if (rc < 0) {
		request_log(" error: topology can't be initialized!\n");
		return rc;
	}

	topology_version = topology->topology_version;
	request_log(" topology: version %lld (entries: %d, interfaces: %d, pads: %d, links: %d\n",
	       topology->topology_version,
	       topology->num_entities,
	       topology->num_interfaces,
	       topology->num_pads,
	       topology->num_links);

	/*
	 * Update topology structures
	 * a) set pointers to media structures we are interested in (mem-allocation)
	 * b) ioctl to update structure element values
	 */
	do {
		if(topology->num_entities > 0)
			topology->ptr_entities = (__u64)calloc(topology->num_entities,
							      sizeof(struct media_v2_entity));
		if (topology->num_entities && !topology->ptr_entities)
			goto error;

		if(topology->num_interfaces > 0)
			topology->ptr_interfaces = (__u64)calloc(topology->num_interfaces,
								sizeof(struct media_v2_interface));
		if (topology->num_interfaces && !topology->ptr_interfaces)
			goto error;

		/* not interested in other structures */
		topology->ptr_pads = 0;
		topology->ptr_links = 0;

		/* 2nd call: get updated topology structure elements */
		rc = ioctl(media_fd, MEDIA_IOC_G_TOPOLOGY, topology);
		if (rc < 0) {
			if (topology->topology_version != topology_version) {
				request_log(" Topology changed from version %lld to %lld. Trying again.\n",
					topology_version,
					topology->topology_version);
				free((void *)topology->ptr_entities);
				free((void *)topology->ptr_interfaces);
				topology_version = topology->topology_version;
				continue;
			}
			request_log(" Topology %lld update error!\n",
				topology_version);
			goto error;
		}
	} while (rc < 0);

	/*
	 * Pick up all available video decoder entities, that support
	 * function 'MEDIA_ENT_F_PROC_VIDEO_DECODER':
	 * -> decompresing a compressd video stream into uncompressed video frames
	 * -> has one sink
	 * -> at least one source
	 */
	struct media_v2_entity *entities = media_get_uptr(topology->ptr_entities);
	for(i=0; i < topology->num_entities; i++) {
		struct media_v2_entity *entity = &entities[i];
		if (entity->function == MEDIA_ENT_F_PROC_VIDEO_DECODER) {
			is_decoder = true;
			decoder->id = entity->id;
			asprintf(&decoder->name, "%s", entity->name);
			request_log(" entity: %s\n", entity->name);
		}
	}

	/*
	 * Pick interface
	 * -> type: 'MEDIA_INTF_T_V4L_VIDEO'
	 * -> device: typically /dev/video?
	 */
	if (is_decoder) {
		struct media_v2_interface *interfaces = media_get_uptr(topology->ptr_interfaces);
		for (j=0; j < topology->num_interfaces; j++) {
			struct media_v2_interface *interface = &interfaces[j];
			struct media_v2_intf_devnode *devnode = &interface->devnode;
			char *video_path;

			if (interface->intf_type == MEDIA_INTF_T_V4L_VIDEO) {
				/* TODO: cedurs proc doesn't assing devnode->minor
				 * Testing: hardcode to /dev/video5 (81, 10)
				 */
				if (devnode->minor == -1)
					devnode->minor = 10;
				/*
				  request_log(" devnode: major->%d, minor->%d\n",
					    devnode->major,
					    devnode->minor);
				*/


				video_path = udev_get_devpath(devnode);
				asprintf(&decoder->video_path, "%s", video_path);

				request_log(" interface: type %s, device %s\n",
					media_interface_type(interface->intf_type),
					video_path);

				/* check capabilities */
				video_fd = open(decoder->video_path, O_RDWR | O_NONBLOCK);
				if (video_fd < 0)
					goto error;

				rc = v4l2_query_capabilities(video_fd, &capabilities);
				if (rc < 0) {
					goto error;
				}

				capabilities_required = V4L2_CAP_STREAMING;

				if ((capabilities & capabilities_required) != capabilities_required) {
					request_log("Missing required driver capabilities\n");
					goto error;
				}
				else {
					decoder->capabilities = capabilities;
					request_log(" capabilities: mask %ld\n",
						    decoder->capabilities);
				}
			}
		}
		rc = 1;
	}

	goto complete;

error:
	rc = -1;

complete:
	if (topology->ptr_entities) {
		free((void *)topology->ptr_entities);
		topology->ptr_entities = 0;
	}
	if (topology->ptr_interfaces) {
		free((void *)topology->ptr_interfaces);
		topology->ptr_interfaces = 0;
		}
	/*
	if (topology->ptr_pads)
		free((void *)topology->ptr_pads);
	if (topology->ptr_links)
		free((void *)topology->ptr_links);
	*/

	//topology->ptr_pads = 0;
	//topology->ptr_links = 0;

	if (device)
		free((void *)device);

	if (media_fd >= 0)
		close(media_fd);

	if (video_fd >= 0)
		close(video_fd);

	return rc;
}


int media_request_alloc(int media_fd)
{
	int fd;
	int rc;

	rc = ioctl(media_fd, MEDIA_IOC_REQUEST_ALLOC, &fd);
	if (rc < 0) {
		request_log("Unable to allocate media request: %s\n",
			    strerror(errno));
		return -1;
	}

	return fd;
}

int media_request_reinit(int request_fd)
{
	int rc;

	rc = ioctl(request_fd, MEDIA_REQUEST_IOC_REINIT, NULL);
	if (rc < 0) {
		request_log("Unable to reinit media request: %s\n",
			    strerror(errno));
		return -1;
	}

	return 0;
}

int media_request_queue(int request_fd)
{
	int rc;

	rc = ioctl(request_fd, MEDIA_REQUEST_IOC_QUEUE, NULL);
	if (rc < 0) {
		request_log("Unable to queue media request: %s\n",
			    strerror(errno));
		return -1;
	}

	return 0;
}

int media_request_wait_completion(int request_fd)
{
	struct timeval tv = { 0, 300000 };
	fd_set except_fds;
	int rc;

	FD_ZERO(&except_fds);
	FD_SET(request_fd, &except_fds);

	rc = select(request_fd + 1, NULL, NULL, &except_fds, &tv);
	if (rc == 0) {
		request_log("Timeout when waiting for media request\n");
		return -1;
	} else if (rc < 0) {
		request_log("Unable to select media request: %s\n",
			    strerror(errno));
		return -1;
	}

	return 0;
}

static inline __u32 media_type(__u32 id)
{
	return id >> 24;
}
