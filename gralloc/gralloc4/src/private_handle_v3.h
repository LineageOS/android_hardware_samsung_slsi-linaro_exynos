/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * Gralloc3-wire-compatible private_handle_t for gralloc4.
 *
 * Only compiled when GRALLOC_LEGACY_V3_HANDLE is defined (soong config
 * arm_gralloc.gralloc3_compat_handle, set by BOARD_GRALLOC4_LEGACY_V3_HANDLE).
 * Included from mali_gralloc_buffer.h in place of the native gralloc4 struct.
 *
 * Why: prebuilt vendor blobs (libGLES_mali, camera, codecs, ...) of older
 * Exynos devices read the buffer handle directly and expect the gralloc3
 * private_handle_t layout. A gralloc4 handle makes them reject the buffer
 * ("mali_config_interface: Invalid handle").
 *
 * Layout:
 *   [native_handle header]
 *   [WIRE BLOCK]  byte-identical to gralloc3 (gralloc3/mali_gralloc_buffer.h)
 *                 Only this part is declared via numFds/numInts, so only this
 *                 part travels over binder / is cloned by libui.
 *   [EXTENSION]   gralloc4-only, process-local state. Never transported.
 *                 Rebuilt by rebuild_ext_from_wire() when a handle is imported.
 *
 * Fields that exist in both versions with the same type are stored once (wire
 * block, gralloc4 name). Fields that differ (plane_info, alloc_sizes, fds,
 * fd_count, attr_size, ...) exist in the extension with the gralloc4 name and
 * are mirrored into the wire block by sync_wire_from_ext().
 */

#ifndef PRIVATE_HANDLE_V3_H_
#define PRIVATE_HANDLE_V3_H_

#include <stddef.h>
#include <sys/types.h>

/* gralloc3 plane_info_t */
typedef struct plane_info_v3 {
	uint32_t offset;
	uint32_t byte_stride;
	uint32_t alloc_width;
	uint32_t alloc_height;
} plane_info_v3_t;

/*
 * gralloc3 attr_region (gralloc3/gralloc_buffer_priv.h), the first part of the
 * attribute region. The prebuilt blobs read the crop and the dataspace from here.
 * All fields are 0xff ("not set") until gralloc writes them.
 */
typedef struct attr_region_v3 {
	int crop_top;
	int crop_left;
	int crop_height;
	int crop_width;
	int use_yuv_transform;
	int use_sparse_alloc;
	uint8_t hdr_info[28];  /* mali_hdr_info */
	int dataspace;
	int force_dataspace;
} __attribute__((packed)) attr_region_v3_t;

/* gralloc3 mali_gralloc_yuv_info (values must match gralloc3) */
typedef enum {
	MALI_V3_YUV_NO_INFO,
	MALI_V3_YUV_BT601_NARROW,
	MALI_V3_YUV_BT601_WIDE,
	MALI_V3_YUV_BT709_NARROW,
	MALI_V3_YUV_BT709_WIDE,
	MALI_V3_YUV_BT2020_NARROW,
	MALI_V3_YUV_BT2020_WIDE
} mali_gralloc_v3_yuv_info;

#define MALI_V3_INTFMT_AFBC_V3_MODIFIERS 0x0000007e00000000ULL /* bits 33-38, gralloc3 */
#define MALI_V3_INTFMT_AFBC_V4_MODIFIERS 0x000000fc00000000ULL /* bits 34-39, gralloc4 */

static inline uint64_t alloc_format_v4_to_v3(uint64_t v4)
{
	/* Bits 40/41 are kept as is: gralloc3 never sets them and the blobs do not
	 * read them, so they survive the round trip to other gralloc4 processes. */
	return (v4 & ~(MALI_V3_INTFMT_AFBC_V4_MODIFIERS | MALI_V3_INTFMT_AFBC_V3_MODIFIERS)) |
	       ((v4 & MALI_V3_INTFMT_AFBC_V4_MODIFIERS) >> 1);
}

static inline uint64_t alloc_format_v3_to_v4(uint64_t v3)
{
	return (v3 & ~(MALI_V3_INTFMT_AFBC_V4_MODIFIERS | MALI_V3_INTFMT_AFBC_V3_MODIFIERS)) |
	       ((v3 & MALI_V3_INTFMT_AFBC_V3_MODIFIERS) << 1);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"

/* Wire size in ints (numFds + numInts); everything before the extension. */
#define PRIVATE_HANDLE_V3_WIRE_INTS \
	((__builtin_offsetof(struct private_handle_t, fd_count) - sizeof(native_handle)) / sizeof(int))

struct private_handle_t : public native_handle
{
	enum
	{
		PRIV_FLAGS_FRAMEBUFFER            = 1U << 0,
		PRIV_FLAGS_USES_ION_COMPOUND_HEAP = 1U << 1,
		PRIV_FLAGS_USES_ION               = 1U << 2,
		PRIV_FLAGS_USES_ION_DMA_HEAP      = 1U << 3,
		PRIV_FLAGS_USES_2PRIVATE_DATA     = 1U << 4,
		PRIV_FLAGS_USES_3PRIVATE_DATA     = 1U << 5,
		PRIV_FLAGS_USES_HFR_MODE          = 1U << 6,
	};

	enum
	{
		LOCK_STATE_WRITE = 1 << 31,
		LOCK_STATE_MAPPED = 1 << 30,
		LOCK_STATE_READ_MASK = 0x3FFFFFFF
	};

	/* ================= WIRE BLOCK (== gralloc3 layout) ================= */

	/* dma_buf fds: plane fds first, the shared attribute fd is the last one. */
	union {
		int share_fd;
		int fds[4];
	};

	short pad_w;
	short pad_h;

	int magic;
	int flags;

	int width;
	int height;
	int req_format;
	uint64_t producer_usage;
	uint64_t consumer_usage;

	union {
		uint32_t format;
		uint64_t internal_format;
	};
	uint64_t v3_alloc_format;

	plane_info_v3_t plane_info_v3[MAX_PLANES];

	union {
		int size;
		int sizes[3];
	};

	uint32_t stride;
	uint32_t layer_count;

	union {
		void *base;
		uint64_t bases[3];
	};

	uint64_t backing_store_id;

	short alignment_w;
	short alignment_h;

	int cpu_read;
	int cpu_write;
	int allocating_pid;
	int remote_pid;
	int ref_count;

	/*
	 * Owned by the prebuilt blobs: they map/unmap the attribute fd on demand
	 * (gralloc3 gralloc_buffer_attr_map/unmap) and reset this to MAP_FAILED.
	 * gralloc4 therefore keeps its own persistent mapping in 'attr_base' below.
	 */
	union {
		void *v3_attr_base;
		uint64_t padding3;
	};

	mali_gralloc_v3_yuv_info yuv_info;
	int fb_fd;

	union {
		off_t offset;
		uint64_t padding4;
	};

	/* UNUSED in gralloc3. Re-used to carry attr_size | reserved_region_size << 32
	 * to other processes (they are not part of the gralloc3 handle). */
	union {
		void *video_metadata_addr_UNUSED;
		uint64_t padding5;
	};

	int ion_handles[3];
	int min_pgsz;
	int is_compressible;
	int plane_count;

	/* ============ EXTENSION (gralloc4 only, process-local) ============ */

	/* Number of plane fds (without the attribute fd). */
	alignas(8) int fd_count;
	int attr_ion_handle;
	/* gralloc4 encoding, mirrored to the wire as v3_alloc_format */
	uint64_t alloc_format;
	/* gralloc4's persistent, process-local mapping of the attribute region */
	void *attr_base;
	uint64_t alloc_sizes[3];
	plane_info_t plane_info[MAX_PLANES];
	uint64_t attr_size;
	uint64_t reserved_region_size;
	uint64_t imapper_version;

	static const int sMagic = 0x3141592;

	/*
	 * Shared attribute region layout in compat mode:
	 *   [0, kAttrPrefix)   gralloc3 'attr_region' (crop, dataspace, hdr_info, ...),
	 *                      initialised to 0xff == "not set" like gralloc3 does.
	 *                      Prebuilt blobs read this through attr_base.
	 *   [kAttrPrefix, ...) gralloc4 shared_metadata, then the reserved region.
	 *   [8K, ...)          ROIINFO (unchanged)
	 */
	static const size_t kAttrPrefix = 256;

	void *metadata_base() const
	{
		return attr_base ? static_cast<char *>(attr_base) + kAttrPrefix : nullptr;
	}

	attr_region_v3_t *attr_region() const
	{
		return static_cast<attr_region_v3_t *>(attr_base);
	}

	private_handle_t(
		int _flags,
		uint64_t _alloc_sizes[3],
		uint64_t _consumer_usage, uint64_t _producer_usage,
		int _fds[5], int _fd_count,
		int _req_format, uint64_t _alloc_format,
		int _width, int _height, int _stride,
		uint32_t _layer_count, plane_info_t _plane_info[MAX_PLANES])
	{
		memset(reinterpret_cast<char *>(this) + sizeof(native_handle), 0,
		       sizeof(private_handle_t) - sizeof(native_handle));

		version = sizeof(native_handle);
		magic = sMagic;
		flags = _flags | PRIV_FLAGS_USES_ION;
		fd_count = _fd_count;
		width = _width;
		height = _height;
		req_format = _req_format;
		producer_usage = _producer_usage;
		consumer_usage = _consumer_usage;
		stride = _stride;
		alloc_format = _alloc_format;
		layer_count = _layer_count;
		allocating_pid = getpid();
		remote_pid = -1;
		ref_count = 1;
		attr_base = nullptr;
		v3_attr_base = MAP_FAILED;
		fb_fd = -1;
		yuv_info = MALI_V3_YUV_NO_INFO;

		for (int i = 0; i < 4; i++)
			fds[i] = -1;
		if (_fds)
		{
			for (int i = 0; i < 4 && i < 5; i++)
				fds[i] = _fds[i];
		}

		memcpy(plane_info, _plane_info, sizeof(plane_info_t) * MAX_PLANES);
		if (_alloc_sizes)
			memcpy(alloc_sizes, _alloc_sizes, sizeof(alloc_sizes));

		set_numfds(fd_count);
		sync_wire_from_ext();
	}

	~private_handle_t()
	{
		magic = 0;
	}

	/*
	 * Mirror gralloc4-only/different-typed state into the gralloc3 wire block.
	 * Must be called after anything in the extension changed that blobs may
	 * read (construction, attr_size/reserved_region_size update).
	 */
	void sync_wire_from_ext()
	{
		for (int i = 0; i < MAX_PLANES; i++)
		{
			plane_info_v3[i].offset = static_cast<uint32_t>(plane_info[i].offset);
			plane_info_v3[i].byte_stride = plane_info[i].byte_stride;
			plane_info_v3[i].alloc_width = plane_info[i].alloc_width;
			plane_info_v3[i].alloc_height = plane_info[i].alloc_height;
			sizes[i] = static_cast<int>(alloc_sizes[i]);
		}

		/* gralloc3: internal_format = legacy base format (no AFBC), alloc_format = full. */
		v3_alloc_format = alloc_format_v4_to_v3(alloc_format);
		internal_format = alloc_format & 0x00000000ffffffffULL;

		min_pgsz = 0x1000; /* SZ_4K */
		is_compressible = (alloc_format & (1ULL << 32)) ? 1 : 0; /* MALI_GRALLOC_INTFMT_AFBC_BASIC */

		int planes = 1;
		for (int i = 1; i < MAX_PLANES; i++)
		{
			if (plane_info[i].byte_stride != 0)
				planes = i + 1;
		}
		plane_count = planes;

		padding5 = (attr_size & 0xffffffffULL) | (reserved_region_size << 32);
	}

	/*
	 * Rebuild the gralloc4 extension from the wire block. Called on a handle
	 * that was just copied from the transported ints (importBuffer).
	 */
	void rebuild_ext_from_wire()
	{
		fd_count = numFds > 1 ? numFds - 1 : 1;
		attr_ion_handle = 0;
		attr_base = nullptr;
		v3_attr_base = MAP_FAILED;
		attr_size = padding5 & 0xffffffffULL;
		reserved_region_size = padding5 >> 32;
		imapper_version = 0;
		alloc_format = alloc_format_v3_to_v4(v3_alloc_format);

		for (int i = 0; i < MAX_PLANES; i++)
		{
			plane_info[i].offset = plane_info_v3[i].offset;
			plane_info[i].byte_stride = plane_info_v3[i].byte_stride;
			plane_info[i].alloc_width = plane_info_v3[i].alloc_width;
			plane_info[i].alloc_height = plane_info_v3[i].alloc_height;
			plane_info[i].fd_idx = (fd_count > 1 && i < fd_count) ? i : 0;
			alloc_sizes[i] = static_cast<uint32_t>(sizes[i]);
			plane_info[i].size = alloc_sizes[i];
		}
	}

	/*
	 * Allocate a full-size handle (wire block + extension) from the ints that
	 * were transported. Release with native_handle_close()/native_handle_delete()
	 * (plain malloc/free).
	 */
	static private_handle_t *clone_from_wire(const native_handle *src)
	{
		if (!src)
			return nullptr;

		private_handle_t *h = static_cast<private_handle_t *>(calloc(1, sizeof(private_handle_t)));
		if (!h)
			return nullptr;

		int *dst = reinterpret_cast<int *>(h) + (sizeof(native_handle) / sizeof(int));
		const int *s = reinterpret_cast<const int *>(src) + (sizeof(native_handle) / sizeof(int));

		h->version = sizeof(native_handle);
		h->numFds = src->numFds;
		h->numInts = src->numInts;
		memcpy(dst, s, (src->numFds + src->numInts) * sizeof(int));

		/* take ownership: duplicate the fds so the source stays valid */
		for (int i = 0; i < src->numFds; i++)
		{
			h->fds[i] = fcntl(src->data[i], F_DUPFD_CLOEXEC, 0);
			if (h->fds[i] < 0)
			{
				for (int j = 0; j < i; j++)
					close(h->fds[j]);
				free(h);
				return nullptr;
			}
		}

		h->rebuild_ext_from_wire();
		return h;
	}

	void set_numfds(int n)
	{
		numFds = n;
		numInts = PRIVATE_HANDLE_V3_WIRE_INTS - n;
	}

	int incr_numfds(int n)
	{
		numFds += n;
		numInts -= n;
		return numFds;
	}

	static int validate(const native_handle *h)
	{
		const private_handle_t *hnd = (const private_handle_t *)h;

		if (!h || h->version != sizeof(native_handle) || hnd->magic != sMagic ||
		    h->numFds + h->numInts != (int)PRIVATE_HANDLE_V3_WIRE_INTS)
		{
			return -EINVAL;
		}

		return 0;
	}

	bool is_multi_plane() const
	{
		return (plane_info[1].byte_stride != 0);
	}

	uint64_t get_usage() const
	{
		return producer_usage | consumer_usage;
	}

	int get_share_attr_fd_index() const
	{
		/* The wire block has room for 4 fds: up to 3 plane fds + the attribute fd. */
		if (fd_count <= 0 || fd_count > 3)
			return -1;

		return fd_count;
	}

	int get_share_attr_fd() const
	{
		int idx = get_share_attr_fd_index();

		if (idx <= 0)
			return -1;

		return fds[idx];
	}

	void set_share_attr_fd(int fd)
	{
		int idx = get_share_attr_fd_index();

		if (idx <= 0)
			return;

		fds[idx] = fd;
	}

	void close_share_attr_fd()
	{
		int fd = get_share_attr_fd();

		if (fd < 0)
			return;

		close(fd);
	}

	void dump(const char *str) const
	{
		ALOGE("[%s] "
			"unique_id(%" PRIu64 ") "
			"numInts(%d) numFds(%d) fd_count(%d) "
			"fd(%d %d %d %d) "
			"flags(%d) "
			"wh(%d %d) "
			"req_format(%#x) alloc_format(%#" PRIx64 ") "
			"usage_pc(0x%" PRIx64 " 0x%" PRIx64 ") "
			"stride(%u) "
			"byte_stride(%d) internal_wh(%d %d) plane_count(%d) "
			"sizes(%d %d %d) "
			"layer_count(%d) attr_size(%" PRIu64 ") "
			"bases(%p %p %p %p) "
			"\n",
			str,
			backing_store_id,
			numInts, numFds, fd_count,
			fds[0], fds[1], fds[2], fds[3],
			flags,
			width, height,
			req_format, alloc_format,
			producer_usage, consumer_usage,
			stride,
			plane_info[0].byte_stride, plane_info[0].alloc_width, plane_info[0].alloc_height,
			plane_count,
			sizes[0], sizes[1], sizes[2],
			layer_count, attr_size,
			(void*)bases[0], (void*)bases[1], (void*)bases[2], attr_base
		);
	}

	int get_alloc_format() const
	{
		return (int)(alloc_format & 0x00000000ffffffffULL);
	}

	static private_handle_t *dynamicCast(const native_handle *in)
	{
		if (validate(in) == 0)
		{
			return (private_handle_t *)in;
		}

		return NULL;
	}
};

#pragma clang diagnostic pop

#endif /* PRIVATE_HANDLE_V3_H_ */
