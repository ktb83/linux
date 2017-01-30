#include <linux/compat.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "vchiq_ioctl.h"

#define VCHIQ_IOC_CREATE_SERVICE32 \
	_IOWR(VCHIQ_IOC_MAGIC, 2, VCHIQ_CREATE_SERVICE_T32)
#define VCHIQ_IOC_QUEUE_MESSAGE32 \
	_IOW(VCHIQ_IOC_MAGIC,  4, VCHIQ_QUEUE_MESSAGE_T32)
#define VCHIQ_IOC_QUEUE_BULK_TRANSMIT32 \
	_IOWR(VCHIQ_IOC_MAGIC, 5, VCHIQ_QUEUE_BULK_TRANSFER_T32)
#define VCHIQ_IOC_QUEUE_BULK_RECEIVE32 \
	_IOWR(VCHIQ_IOC_MAGIC, 6, VCHIQ_QUEUE_BULK_TRANSFER_T32)
#define VCHIQ_IOC_AWAIT_COMPLETION32 \
	_IOWR(VCHIQ_IOC_MAGIC, 7, VCHIQ_AWAIT_COMPLETION_T32)
#define VCHIQ_IOC_DEQUEUE_MESSAGE32 \
	_IOWR(VCHIQ_IOC_MAGIC, 8, VCHIQ_DEQUEUE_MESSAGE_T32)
#define VCHIQ_IOC_DUMP_PHYS_MEM32 \
	_IOW(VCHIQ_IOC_MAGIC,  15, VCHIQ_DUMP_MEM_T32)
#define VCHIQ_IOC_GET_CONFIG32 \
	_IOWR(VCHIQ_IOC_MAGIC, 10, VCHIQ_GET_CONFIG_T32)

typedef struct {
    s32 fourcc;
    u32 callback;
    u32 userdata;
    short version;       /* Increment for non-trivial changes */
    short version_min;   /* Update for incompatible changes */
} VCHIQ_SERVICE_PARAMS_T32;

typedef struct {
    VCHIQ_SERVICE_PARAMS_T32 params;
    s32 is_open;
    s32 is_vchi;
    u32 handle;       /* OUT */
} VCHIQ_CREATE_SERVICE_T32;

typedef struct {
	u32 data;
	u32 size;
} VCHIQ_ELEMENT_T32;

typedef struct {
	u32 handle;
	u32 count;
	u32 elements;
} VCHIQ_QUEUE_MESSAGE_T32;

typedef struct {
	u32 handle;
	u32 data;
	u32 size;
	u32 userdata;
	VCHIQ_BULK_MODE_T mode;
} VCHIQ_QUEUE_BULK_TRANSFER_T32;

typedef struct {
	VCHIQ_REASON_T reason;
	u32 header;
	u32 service_userdata;
	u32 bulk_userdata;
} VCHIQ_COMPLETION_DATA_T32;

typedef struct {
	u32 count;
	u32 buf;
	u32 msgbufsize;
	u32 msgbufcount; /* IN/OUT */
	u32 msgbufs;
} VCHIQ_AWAIT_COMPLETION_T32;

typedef struct {
	u32 handle;
	s32 blocking;
	u32 bufsize;
	u32 buf;
} VCHIQ_DEQUEUE_MESSAGE_T32;

typedef struct {
    u32 config_size;
    u32 pconfig;
} VCHIQ_GET_CONFIG_T32;

typedef struct {
	u32 virt_addr;
	u32 num_bytes;
} VCHIQ_DUMP_MEM_T32;

/****************************************************************************
*
*   vchiq_compat_ioctl
*
***************************************************************************/
long
vchiq_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	/* Convert arguments to 64bit types */
	switch (cmd) {
		case VCHIQ_IOC_CREATE_SERVICE32: {
			VCHIQ_CREATE_SERVICE_T32 args32;
			VCHIQ_CREATE_SERVICE_T __user *args;

			args = compat_alloc_user_space(sizeof(*args));
			if (!args)
				return -EFAULT;

			if (copy_from_user(&args32, (const void __user *)arg,
				sizeof(args32)) != 0)
					return -EFAULT;

			if (put_user(args32.params.fourcc, &args->params.fourcc) != 0 ||
				put_user(args32.params.callback, (uintptr_t *)(&args->params.callback)) != 0 ||
				put_user(args32.params.userdata, (uintptr_t *)(&args->params.userdata)) != 0 ||
				put_user(args32.params.version, &args->params.version) != 0 ||
				put_user(args32.params.version_min, &args->params.version_min) != 0	||
				put_user(args32.is_open, &args->is_open) != 0 ||
				put_user(args32.is_vchi, &args->is_vchi) != 0 ||
				put_user(args32.handle, &args->handle) != 0)
					return -EFAULT;

			ret = vchiq_ioctl(file, VCHIQ_IOC_CREATE_SERVICE, (unsigned long)args);
			if (ret < 0)
				return ret;

			if (get_user(args32.handle, &args->handle) != 0)
				return -EFAULT;

			if (copy_to_user((void __user *)arg, &args32,
				sizeof(args32)) != 0)
					return -EFAULT;
		} break;

		case VCHIQ_IOC_QUEUE_MESSAGE32: {
			VCHIQ_ELEMENT_T32 element32;
			VCHIQ_QUEUE_MESSAGE_T32 message32;
			VCHIQ_ELEMENT_T __user *element;
			VCHIQ_QUEUE_MESSAGE_T __user *message;
			size_t count;

			if (copy_from_user(&message32, (const void __user *)arg,
				sizeof(message32)) != 0)
					return -EFAULT;

			message = compat_alloc_user_space(sizeof(*message) + sizeof(*element) * message32.count);
			if (!message)
				return -EFAULT;

			element = (VCHIQ_ELEMENT_T __user *)(((u8 *)message) + sizeof(*message));

			/* Copy the message */
			if (put_user(message32.handle, &message->handle) != 0 ||
				put_user(message32.count, &message->count) != 0 ||
				put_user(element, &message->elements) != 0)
					return -EFAULT;

			for (count = 0; count < message32.count; count++) {
				/* Copy the current element into kernel space and fill the element array */
				if (copy_from_user(&element32, (const void __user *)
					 &((VCHIQ_ELEMENT_T32 *)(uintptr_t)message32.elements)[count], sizeof(element32)) != 0)
						return -EFAULT;

				if (put_user(element32.data, (uintptr_t *)(&element[count].data)) != 0 ||
					put_user(element32.size, &element[count].size) != 0)
						return -EFAULT;
			}

			ret = vchiq_ioctl(file, VCHIQ_IOC_QUEUE_MESSAGE, (unsigned long)message);
			if (ret < 0)
				return ret;
		} break;

		case VCHIQ_IOC_QUEUE_BULK_TRANSMIT32:
		case VCHIQ_IOC_QUEUE_BULK_RECEIVE32: {
			VCHIQ_QUEUE_BULK_TRANSFER_T32 args32;
			VCHIQ_QUEUE_BULK_TRANSFER_T __user *args;

			args = compat_alloc_user_space(sizeof(*args));
			if (!args)
				return -EFAULT;

			if (copy_from_user(&args32, (const void __user *)arg,
				sizeof(args32)) != 0)
					return -EFAULT;

			if (put_user(args32.handle, &args->handle) != 0 ||
				put_user(args32.data, (uintptr_t *)(&args->data)) != 0 ||
				put_user(args32.size, &args->size) != 0 ||
				put_user(args32.userdata, (uintptr_t *)(&args->userdata)) != 0 ||
				put_user(args32.mode, &args->mode) != 0)
					return -EFAULT;

			ret = vchiq_ioctl(file, cmd == VCHIQ_IOC_QUEUE_BULK_TRANSMIT32 ?
				VCHIQ_IOC_QUEUE_BULK_TRANSMIT : VCHIQ_IOC_QUEUE_BULK_RECEIVE, (unsigned long)args);
			if (ret < 0)
				return ret;

			if (get_user(args32.mode, &args->mode) != 0)
				return -EFAULT;

			if (copy_to_user((void __user *)arg, &args32,
				sizeof(args32)) != 0)
					return -EFAULT;
		} break;

		case VCHIQ_IOC_AWAIT_COMPLETION32: {
			VCHIQ_AWAIT_COMPLETION_T32 await32;
			VCHIQ_COMPLETION_DATA_T data;
			VCHIQ_COMPLETION_DATA_T32 __user *buf32;
			VCHIQ_AWAIT_COMPLETION_T __user *await;
			void __user **msgbufs;
			VCHIQ_COMPLETION_DATA_T __user *buf;
			u32 *msgbufs32;
			size_t i;

			if (copy_from_user(&await32, (const void __user *)arg,
				sizeof(await32)) != 0)
					return -EFAULT;

			await = compat_alloc_user_space(sizeof(*await) + sizeof(*msgbufs) * await32.msgbufcount +
				sizeof(*buf) * await32.count);
			if (!await)
				return -EFAULT;

			if (put_user(await32.count, &await->count) != 0 ||
				put_user(await32.msgbufsize, &await->msgbufsize) != 0 ||
				put_user(await32.msgbufcount, &await->msgbufcount) != 0)
					return -EFAULT;

			/* await32.msgbufs is an array of await32.msgbufcount 32 bits pointers.
			** Allocate a new array of 64 bits pointers and copy them to the new array.
			** NOTE: Each of these pointers is supposed to point to a memory buffer large
			** enough to store a VCHIQ_HEADER_T. Fortunately the size of this struct
			** is the same for 32 bits and 64 bits platforms, so the buffers don't need
			** to be reallocated */
			msgbufs = (void __user **)(((u8 *)await) + sizeof(*await));

			msgbufs32 = (u32 *)(uintptr_t)await32.msgbufs;
			for (i = 0; i < await32.msgbufcount; i++) {
				msgbufs[i] = (void *)(uintptr_t)msgbufs32[i];
			}

			if (put_user(msgbufs, &await->msgbufs) != 0)
				return -EFAULT;

			/* await32.buf is an array of await32.count VCHIQ_COMPLETION_DATA_T. The size
			** of this struct is not the same for 32 bits and 64 bits platforms so a new
			** array must be allocated. This is an out parameter so it does not need
			** to be copied */
			buf = (VCHIQ_COMPLETION_DATA_T __user *)(((u8 *)await) + sizeof(*await) + sizeof(*msgbufs) * await32.msgbufcount);

			if (put_user(buf, &await->buf) != 0)
				return -EFAULT;

			ret = vchiq_ioctl(file, VCHIQ_IOC_AWAIT_COMPLETION, (unsigned long)await);
			if (ret < 0)
				return ret;

			buf32 = (VCHIQ_COMPLETION_DATA_T32 __user *)(uintptr_t)await32.buf;

			for (i = 0; i < await32.count; i++) {
				if (copy_from_user(&data, &buf[i], sizeof(*buf)) != 0)
					return -EFAULT;

				if (put_user(data.reason, &buf32[i].reason) != 0 ||
					put_user((u32)(uintptr_t)data.header, &buf32[i].header) != 0 ||
					put_user((u32)(uintptr_t)data.service_userdata, &buf32[i].service_userdata) != 0 ||
					put_user((u32)(uintptr_t)data.bulk_userdata, &buf32[i].bulk_userdata) != 0)
						return -EFAULT;
			}

			if (get_user(await32.msgbufcount, &await->msgbufcount) != 0)
				return -EFAULT;

			if (copy_to_user((void __user *)arg, &await32,
				sizeof(await32)) != 0)
					return -EFAULT;
		} break;

		case VCHIQ_IOC_DEQUEUE_MESSAGE32: {
			VCHIQ_DEQUEUE_MESSAGE_T32 args32;
			VCHIQ_DEQUEUE_MESSAGE_T __user *args;

			args = compat_alloc_user_space(sizeof(*args));
			if (!args)
				return -EFAULT;

			if (copy_from_user(&args32, (const void __user *)arg,
				sizeof(args32)) != 0)
				return -EFAULT;

			if (put_user(args32.handle, &args->handle) != 0 ||
				put_user(args32.blocking, &args->blocking) != 0 ||
				put_user(args32.bufsize, &args->bufsize) != 0 ||
				put_user(args32.buf, (uintptr_t *)(&args->buf)) != 0)
					return -EFAULT;

			ret = vchiq_ioctl(file, VCHIQ_IOC_DEQUEUE_MESSAGE, (unsigned long)args);
			if (ret < 0)
				return ret;
		} break;

		case VCHIQ_IOC_GET_CONFIG32: {
			VCHIQ_GET_CONFIG_T32 args32;
			VCHIQ_GET_CONFIG_T __user *args;

			args = compat_alloc_user_space(sizeof(*args));
			if (!args)
				return -EFAULT;

			if (copy_from_user(&args32, (const void __user *)arg,
				sizeof(args32)) != 0)
				return -EFAULT;

			if (put_user(args32.config_size, &args->config_size) != 0 ||
				put_user(args32.pconfig, (uintptr_t *)(&args->pconfig)) != 0)
					return -EFAULT;

			ret = vchiq_ioctl(file, VCHIQ_IOC_GET_CONFIG, (unsigned long)args);
			if (ret < 0)
				return ret;
		} break;

		case VCHIQ_IOC_DUMP_PHYS_MEM32: {
			VCHIQ_DUMP_MEM_T32 args32;
			VCHIQ_DUMP_MEM_T __user *args;

			args = compat_alloc_user_space(sizeof(*args));
			if (!args)
				return -EFAULT;

			if (copy_from_user(&args32, (const void __user *)arg,
				sizeof(args32)) != 0)
				return -EFAULT;

			if (put_user(args32.virt_addr, (uintptr_t *)(&args->virt_addr)) != 0 ||
				put_user(args32.num_bytes, &args->num_bytes))
					return -EFAULT;

			ret = vchiq_ioctl(file, VCHIQ_IOC_DUMP_PHYS_MEM, (unsigned long)args);
			if (ret < 0)
				return ret;
		} break;

		default: {
			ret = vchiq_ioctl(file, cmd, arg);
		} break;
	}

	return ret;
}
