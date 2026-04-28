/*
 * Copyright (c) 2020, STMICROELECTRONICS
 * Copyright (c) 2025, Arm Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* https://github.com/zephyrproject-rtos/zephyr/blob/main/samples/subsys/ipc/openamp_rsc_table/src/main_remote.c */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/drivers/ipm.h>
#include <zephyr/drivers/gpio.h>

#include <openamp/open_amp.h>
#include <metal/sys.h>
#include <metal/io.h>
#include <resource_table.h>
#include <addr_translation.h>
#include <openamp/rpmsg.h>

#ifdef CONFIG_SHELL_BACKEND_RPMSG
#include <zephyr/shell/shell_rpmsg.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(openamp_rsc_table);

#define RPMSG_ENDPOINT_NAME "rpmsg-tty"
/* Observed OpenAMP error code for "address not bound" (-2003). */
#define RPMSG_SEND_ADDR_ERR (-2003)

#define SHM_DEVICE_NAME	"shm"

#if !DT_HAS_CHOSEN(zephyr_ipc_shm)
#error "Sample requires definition of shared memory for rpmsg"
#endif

#if CONFIG_IPM_MAX_DATA_SIZE > 0

#define	IPM_SEND(dev, w, id, d, s) ipm_send(dev, w, id, d, s)
#else
#define IPM_SEND(dev, w, id, d, s) ipm_send(dev, w, id, NULL, 0)
#endif

/* Constants derived from device tree */
#define SHM_NODE		DT_CHOSEN(zephyr_ipc_shm)
#define SHM_START_ADDR	DT_REG_ADDR(SHM_NODE)
#define SHM_SIZE		DT_REG_SIZE(SHM_NODE)

#define APP_TASK_STACK_SIZE (1024)
#define APP_GPIO_TASK_STACK_SIZE (1024)

K_THREAD_STACK_DEFINE(thread_mng_stack, APP_TASK_STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_gpio_stack, APP_GPIO_TASK_STACK_SIZE);

static struct k_thread thread_mng_data;
static struct k_thread thread_gpio_data;

static const struct device *const ipm_handle =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc));

static metal_phys_addr_t shm_physmap = SHM_START_ADDR;
static metal_phys_addr_t rsc_tab_physmap;

static struct metal_io_region shm_io_data; /* shared memory */
static struct metal_io_region rsc_io_data; /* rsc_table memory */

static struct metal_io_region *shm_io = &shm_io_data;

static struct metal_io_region *rsc_io = &rsc_io_data;
static struct rpmsg_virtio_device rvdev;

static void *rsc_table;
static struct rpmsg_device *rpdev;

static struct rpmsg_endpoint gpio_ept;

static K_SEM_DEFINE(data_sem, 0, 1);
static K_SEM_DEFINE(data_gpio_sem, 0, 1);
static K_SEM_DEFINE(gpio_channel_bound_sem, 0, 1);

static bool gpio_channel_bound;
static uint32_t gpio_peer_addr = RPMSG_ADDR_ANY;

static const struct gpio_dt_spec switch_pin0 = GPIO_DT_SPEC_GET(DT_NODELABEL(switch_pin0), gpios);

static void platform_ipm_callback(const struct device *dev, void *context,
				  uint32_t id, volatile void *data)
{
	LOG_DBG("%s: msg received from mb %d", __func__, id);
	k_sem_give(&data_sem);
}

static int rpmsg_recv_gpio_callback(struct rpmsg_endpoint *ept, void *data,
				    size_t len, uint32_t src, void *priv)
{
	ARG_UNUSED(len);
	ARG_UNUSED(priv);
	ARG_UNUSED(data);

	uint32_t prev_addr = gpio_peer_addr;
	gpio_peer_addr = src;

	if (!gpio_channel_bound) {
		gpio_channel_bound = true;
		k_sem_give(&gpio_channel_bound_sem);
		LOG_INF("rpmsg peer bound via callback (addr=%u)", src);
	} else if (prev_addr != src) {
		LOG_INF("rpmsg peer address updated via callback (addr=%u)", src);
	}

	return RPMSG_SUCCESS;
}

static void receive_message(unsigned char **msg, unsigned int *len)
{
	int status = k_sem_take(&data_sem, K_FOREVER);

	if (status == 0) {
		rproc_virtio_notified(rvdev.vdev, VRING1_ID);
	}
}

static void rpmsg_announce_channel(struct rpmsg_device *rdev, const char *name,
			   uint32_t src)
{
	LOG_INF("Name service event: %s (src=%u)", name, src);

	if (strcmp(name, RPMSG_ENDPOINT_NAME) == 0) {
		uint32_t prev_addr = gpio_peer_addr;
		gpio_peer_addr = src;
		if (!gpio_channel_bound) {
			gpio_channel_bound = true;
			k_sem_give(&gpio_channel_bound_sem);
			LOG_INF("rpmsg peer bound via name service (addr=%u)", src);
		} else if (prev_addr != src) {
			LOG_INF("rpmsg peer address updated via name service (addr=%u)", src);
		}
	}
}

int mailbox_notify(void *priv, uint32_t id)
{
	ARG_UNUSED(priv);

	LOG_DBG("%s: notify vring ID %u", __func__, id);
	int ret = IPM_SEND(ipm_handle, 0, id, &id, 4);
	if (ret) {
		LOG_ERR("IPM send failed: %d", ret);
	}

	return 0;
}

int init_shared_resources(void)
{
	int rsc_size;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	int status;

	status = metal_init(&metal_params);
	if (status) {
		LOG_ERR("metal_init: failed: %d", status);
		return -1;
	}

	/* declare shared memory region */
	metal_io_init(shm_io, (void *)SHM_START_ADDR, &shm_physmap,
		      SHM_SIZE, -1, 0, addr_translation_get_ops(shm_physmap));

	/* declare resource table region */
	rsc_table_get(&rsc_table, &rsc_size);
	rsc_tab_physmap = (uintptr_t)rsc_table;

	metal_io_init(rsc_io, rsc_table,
		      &rsc_tab_physmap, rsc_size, -1, 0, NULL);

	/* setup IPM */
	if (!device_is_ready(ipm_handle)) {
		LOG_ERR("IPM device is not ready");
		return -1;
	}

	ipm_register_callback(ipm_handle, platform_ipm_callback, NULL);

	status = ipm_set_enabled(ipm_handle, 1); /*  */
	if (status) {
		LOG_ERR("ipm_set_enabled failed: %d", status);
		return -1;
	}
	LOG_INF("IPM initialized");

	return 0;
}

static void cleanup_system(void)
{
	ipm_set_enabled(ipm_handle, 0);
	rpmsg_deinit_vdev(&rvdev);
	metal_finish();
}

struct  rpmsg_device *
platform_create_rpmsg_vdev(unsigned int vdev_index,
			   unsigned int role,
			   void (*rst_cb)(struct virtio_device *vdev),
			   rpmsg_ns_bind_cb ns_cb)
{
	struct fw_rsc_vdev_vring *vring_rsc;
	struct virtio_device *vdev;
	int ret;

	vdev = rproc_virtio_create_vdev(VIRTIO_DEV_DEVICE, VDEV_ID,
					rsc_table_to_vdev(rsc_table),
					rsc_io, NULL, mailbox_notify, NULL);

	if (!vdev) {
		LOG_ERR("failed to create vdev");
		return NULL;
	}

	rproc_virtio_wait_remote_ready(vdev); /* block on virtio init */

	vring_rsc = rsc_table_get_vring0(rsc_table);
	ret = rproc_virtio_init_vring(vdev, 0, vring_rsc->notifyid,
				      (void *)vring_rsc->da, rsc_io,
				      vring_rsc->num, vring_rsc->align);
	if (ret) {
		LOG_ERR("failed to init vring 0: %d", ret);
		goto failed;
	}

	vring_rsc = rsc_table_get_vring1(rsc_table);
	ret = rproc_virtio_init_vring(vdev, 1, vring_rsc->notifyid,
				      (void *)vring_rsc->da, rsc_io,
				      vring_rsc->num, vring_rsc->align);
	if (ret) {
		LOG_ERR("failed to init vring 1: %d", ret);
		goto failed;
	}

	ret = rpmsg_init_vdev(&rvdev, vdev, ns_cb, shm_io, NULL);
	if (ret) {
		LOG_ERR("failed rpmsg_init_vdev: %d", ret);
		goto failed;
	}
	LOG_INF("rpmsg_vdev initialized");

	return rpmsg_virtio_get_rpmsg_device(&rvdev);

failed:
	rproc_virtio_remove_vdev(vdev);

	return NULL;
}

void main_loop(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret = 1;
	const char *active_msg = "on\n";
	int active_len = strlen(active_msg);

	const char *inactive_msg = "off\n";
	int inactive_len = strlen(inactive_msg);


	k_sem_take(&data_gpio_sem, K_FOREVER);

	gpio_channel_bound = false;
	gpio_peer_addr = RPMSG_ADDR_ANY;
	while (k_sem_take(&gpio_channel_bound_sem, K_NO_WAIT) == 0) {
		/* burn up anything left over */
	}


	ret = rpmsg_create_ept(&gpio_ept, rpdev, RPMSG_ENDPOINT_NAME,
			       RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			       rpmsg_recv_gpio_callback, NULL);
	if (ret) {
		LOG_ERR("Could not create endpoint '%s': %d", RPMSG_ENDPOINT_NAME, ret);
		goto task_end;
	}

	LOG_INF("Endpoint '%s' created at addr %d", RPMSG_ENDPOINT_NAME, gpio_ept.addr);

	if (!gpio_channel_bound || gpio_peer_addr == RPMSG_ADDR_ANY) {
		LOG_INF("Waiting for rpmsg peer to bind...");
		k_sem_take(&gpio_channel_bound_sem, K_FOREVER);
	}
	LOG_INF("rpmsg peer ready (dest=%u)", gpio_peer_addr);

	/* Configure GPIO pins */
	if (!gpio_is_ready_dt(&switch_pin0)) {
		LOG_ERR("GPIO pin 0 device is not ready");
		goto task_end;
	}

	ret = gpio_pin_configure_dt(&switch_pin0, GPIO_INPUT | GPIO_PULL_UP);
	if (ret != 0) {
		LOG_ERR("Failed to configure GPIO pin 0: %d", ret);
		goto task_end;
	}

	LOG_INF("Starting main message loop");
	LOG_INF("Monitoring pin 33");

	const int refresh_period_ms = 1000;
	int last_pin_value = -1;
	while (gpio_ept.addr != RPMSG_ADDR_ANY) { /* While rpmsg channel is open */
		int new_pin_value = gpio_pin_get_dt(&switch_pin0);
		if (new_pin_value < 0) {
			LOG_ERR("Error reading pin value: %d", new_pin_value);
			k_msleep(refresh_period_ms);
			continue;
		}

		if (!gpio_channel_bound || gpio_peer_addr == RPMSG_ADDR_ANY) { /* Can we no longer send messages? */
			k_sem_take(&gpio_channel_bound_sem, K_FOREVER);
			LOG_INF("rpmsg peer announced late (dest=%u)", gpio_peer_addr);
			continue;
		}

		if (new_pin_value != last_pin_value){
			LOG_INF("State changed: %d -> %d", last_pin_value, new_pin_value);
			last_pin_value = new_pin_value;
			const char *msg = new_pin_value == 0 ? inactive_msg : active_msg;
			int msg_len = new_pin_value == 0 ? inactive_len : active_len;
			ret = rpmsg_sendto(&gpio_ept, msg, msg_len, gpio_peer_addr);
			if (ret < 0) {
				LOG_ERR("Failed to send message: %d", ret);
				if (ret == RPMSG_SEND_ADDR_ERR) {
					LOG_WRN("Peer address invalid; waiting for rebind");
					gpio_channel_bound = false;
					gpio_peer_addr = RPMSG_ADDR_ANY;
					continue;
				}
			}
		}
		k_msleep(refresh_period_ms);
	}
task_end:
	rpmsg_destroy_ept(&gpio_ept);
	LOG_INF("GPIO Switch Monitor ended");
}

void openamp_init(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	unsigned char *msg;
	unsigned int len;
	int ret = 0;

	/* Initialize platform */
	ret = init_shared_resources();
	if (ret) {
		LOG_ERR("Failed to initialize platform");
		ret = -1;
		goto task_end;
	}
	LOG_INF("Openamp remote initilised");

	rpdev = platform_create_rpmsg_vdev(0, VIRTIO_DEV_DEVICE, NULL,
					   rpmsg_announce_channel);
	if (!rpdev) {
		LOG_ERR("Failed to create rpmsg virtio device");
		ret = -1;
		goto task_end;
	}

#ifdef CONFIG_SHELL_BACKEND_RPMSG
	(void)shell_backend_rpmsg_init_transport(rpdev);
#endif

	/* Process initial virtio notifications to ensure device is ready */
	for (int i = 0; i < 10; i++) {
		if (k_sem_take(&data_sem, K_MSEC(100)) == 0) {
			rproc_virtio_notified(rvdev.vdev, VRING1_ID);
		}
	}

	/* Start the GPIO switch monitor */
	k_sem_give(&data_gpio_sem);

	while (1) {
		receive_message(&msg, &len);
	}

task_end:
	cleanup_system();

	LOG_INF("OpenAMP demo ended");
}

int main(void)
{
	LOG_INF("Starting application threads!");
	k_thread_create(&thread_mng_data, thread_mng_stack, APP_TASK_STACK_SIZE,
			openamp_init,
			NULL, NULL, NULL, K_PRIO_COOP(8), 0, K_NO_WAIT);
	k_thread_create(&thread_gpio_data, thread_gpio_stack, APP_GPIO_TASK_STACK_SIZE,
			main_loop,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	return 0;
}
