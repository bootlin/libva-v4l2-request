/* SPDX-License-Identifier: GPL-2.0 */
/*
 * These are the HEVC state controls for use with stateless HEVC
 * codec drivers.
 *
 * It turns out that these structs are not stable yet and will undergo
 * more changes. So keep them private until they are stable and ready to
 * become part of the official public API.
 */

#ifndef _HEVC_CTRLS_H_
#define _HEVC_CTRLS_H_

/* The pixel format isn't stable at the moment and will likely be renamed. */
#define V4L2_PIX_FMT_HEVC_SLICE v4l2_fourcc('S', '2', '6', '5') /* HEVC parsed slices */

#define V4L2_CID_MPEG_VIDEO_HEVC_SPS		(V4L2_CID_MPEG_BASE + 1008)
#define V4L2_CID_MPEG_VIDEO_HEVC_PPS		(V4L2_CID_MPEG_BASE + 1009)
#define V4L2_CID_MPEG_VIDEO_HEVC_SLICE_PARAMS	(V4L2_CID_MPEG_BASE + 1010)

/* enum v4l2_ctrl_type type values */
#define V4L2_CTRL_TYPE_HEVC_SPS 0x0120
#define V4L2_CTRL_TYPE_HEVC_PPS 0x0121
#define V4L2_CTRL_TYPE_HEVC_SLICE_PARAMS 0x0122

#define V4L2_HEVC_SLICE_TYPE_B	0
#define V4L2_HEVC_SLICE_TYPE_P	1
#define V4L2_HEVC_SLICE_TYPE_I	2

/* The controls are not stable at the moment and will likely be reworked. */
struct v4l2_ctrl_hevc_sps {
	/* ISO/IEC 23008-2, ITU-T Rec. H.265: Sequence parameter set */
	__u8	chroma_format_idc;
	__u8	separate_colour_plane_flag;
	__u16	pic_width_in_luma_samples;
	__u16	pic_height_in_luma_samples;
	__u8	bit_depth_luma_minus8;
	__u8	bit_depth_chroma_minus8;
	__u8	log2_max_pic_order_cnt_lsb_minus4;
	__u8	sps_max_dec_pic_buffering_minus1;
	__u8	sps_max_num_reorder_pics;
	__u8	sps_max_latency_increase_plus1;
	__u8	log2_min_luma_coding_block_size_minus3;
	__u8	log2_diff_max_min_luma_coding_block_size;
	__u8	log2_min_luma_transform_block_size_minus2;
	__u8	log2_diff_max_min_luma_transform_block_size;
	__u8	max_transform_hierarchy_depth_inter;
	__u8	max_transform_hierarchy_depth_intra;
	__u8	scaling_list_enabled_flag;
	__u8	amp_enabled_flag;
	__u8	sample_adaptive_offset_enabled_flag;
	__u8	pcm_enabled_flag;
	__u8	pcm_sample_bit_depth_luma_minus1;
	__u8	pcm_sample_bit_depth_chroma_minus1;
	__u8	log2_min_pcm_luma_coding_block_size_minus3;
	__u8	log2_diff_max_min_pcm_luma_coding_block_size;
	__u8	pcm_loop_filter_disabled_flag;
	__u8	num_short_term_ref_pic_sets;
	__u8	long_term_ref_pics_present_flag;
	__u8	num_long_term_ref_pics_sps;
	__u8	sps_temporal_mvp_enabled_flag;
	__u8	strong_intra_smoothing_enabled_flag;
};

struct v4l2_ctrl_hevc_pps {
	/* ISO/IEC 23008-2, ITU-T Rec. H.265: Picture parameter set */
	__u8	dependent_slice_segment_flag;
	__u8	output_flag_present_flag;
	__u8	num_extra_slice_header_bits;
	__u8	sign_data_hiding_enabled_flag;
	__u8	cabac_init_present_flag;
	__s8	init_qp_minus26;
	__u8	constrained_intra_pred_flag;
	__u8	transform_skip_enabled_flag;
	__u8	cu_qp_delta_enabled_flag;
	__u8	diff_cu_qp_delta_depth;
	__s8	pps_cb_qp_offset;
	__s8	pps_cr_qp_offset;
	__u8	pps_slice_chroma_qp_offsets_present_flag;
	__u8	weighted_pred_flag;
	__u8	weighted_bipred_flag;
	__u8	transquant_bypass_enabled_flag;
	__u8	tiles_enabled_flag;
	__u8	entropy_coding_sync_enabled_flag;
	__u8	num_tile_columns_minus1;
	__u8	num_tile_rows_minus1;
	__u8	column_width_minus1[20];
	__u8	row_height_minus1[22];
	__u8	loop_filter_across_tiles_enabled_flag;
	__u8	pps_loop_filter_across_slices_enabled_flag;
	__u8	deblocking_filter_override_enabled_flag;
	__u8	pps_disable_deblocking_filter_flag;
	__s8	pps_beta_offset_div2;
	__s8	pps_tc_offset_div2;
	__u8	lists_modification_present_flag;
	__u8	log2_parallel_merge_level_minus2;
	__u8	slice_segment_header_extension_present_flag;
	__u8	padding;
};

#define V4L2_HEVC_DPB_ENTRY_RPS_ST_CURR_BEFORE	0x01
#define V4L2_HEVC_DPB_ENTRY_RPS_ST_CURR_AFTER	0x02
#define V4L2_HEVC_DPB_ENTRY_RPS_LT_CURR		0x03

#define V4L2_HEVC_DPB_ENTRIES_NUM_MAX		16

struct v4l2_hevc_dpb_entry {
	__u64	timestamp;
	__u8	rps;
	__u8	field_pic;
	__u16	pic_order_cnt[2];
	__u8	padding[2];
};

struct v4l2_hevc_pred_weight_table {
	__u8	luma_log2_weight_denom;
	__s8	delta_chroma_log2_weight_denom;

	__s8	delta_luma_weight_l0[V4L2_HEVC_DPB_ENTRIES_NUM_MAX];
	__s8	luma_offset_l0[V4L2_HEVC_DPB_ENTRIES_NUM_MAX];
	__s8	delta_chroma_weight_l0[V4L2_HEVC_DPB_ENTRIES_NUM_MAX][2];
	__s8	chroma_offset_l0[V4L2_HEVC_DPB_ENTRIES_NUM_MAX][2];

	__s8	delta_luma_weight_l1[V4L2_HEVC_DPB_ENTRIES_NUM_MAX];
	__s8	luma_offset_l1[V4L2_HEVC_DPB_ENTRIES_NUM_MAX];
	__s8	delta_chroma_weight_l1[V4L2_HEVC_DPB_ENTRIES_NUM_MAX][2];
	__s8	chroma_offset_l1[V4L2_HEVC_DPB_ENTRIES_NUM_MAX][2];

	__u8	padding[2];
};

struct v4l2_ctrl_hevc_slice_params {
	__u32	bit_size;
	__u32	data_bit_offset;

	/* ISO/IEC 23008-2, ITU-T Rec. H.265: NAL unit header */
	__u8	nal_unit_type;
	__u8	nuh_temporal_id_plus1;

	/* ISO/IEC 23008-2, ITU-T Rec. H.265: General slice segment header */
	__u8	slice_type;
	__u8	colour_plane_id;
	__u16	slice_pic_order_cnt;
	__u8	slice_sao_luma_flag;
	__u8	slice_sao_chroma_flag;
	__u8	slice_temporal_mvp_enabled_flag;
	__u8	num_ref_idx_l0_active_minus1;
	__u8	num_ref_idx_l1_active_minus1;
	__u8	mvd_l1_zero_flag;
	__u8	cabac_init_flag;
	__u8	collocated_from_l0_flag;
	__u8	collocated_ref_idx;
	__u8	five_minus_max_num_merge_cand;
	__u8	use_integer_mv_flag;
	__s8	slice_qp_delta;
	__s8	slice_cb_qp_offset;
	__s8	slice_cr_qp_offset;
	__s8	slice_act_y_qp_offset;
	__s8	slice_act_cb_qp_offset;
	__s8	slice_act_cr_qp_offset;
	__u8	slice_deblocking_filter_disabled_flag;
	__s8	slice_beta_offset_div2;
	__s8	slice_tc_offset_div2;
	__u8	slice_loop_filter_across_slices_enabled_flag;

	/* ISO/IEC 23008-2, ITU-T Rec. H.265: Picture timing SEI message */
	__u8	pic_struct;

	/* ISO/IEC 23008-2, ITU-T Rec. H.265: General slice segment header */
	struct v4l2_hevc_dpb_entry dpb[V4L2_HEVC_DPB_ENTRIES_NUM_MAX];
	__u8	num_active_dpb_entries;
	__u8	ref_idx_l0[V4L2_HEVC_DPB_ENTRIES_NUM_MAX];
	__u8	ref_idx_l1[V4L2_HEVC_DPB_ENTRIES_NUM_MAX];

	__u8	num_rps_poc_st_curr_before;
	__u8	num_rps_poc_st_curr_after;
	__u8	num_rps_poc_lt_curr;

	/* ISO/IEC 23008-2, ITU-T Rec. H.265: Weighted prediction parameter */
	struct v4l2_hevc_pred_weight_table pred_weight_table;

	__u8	padding[2];
};

#endif
