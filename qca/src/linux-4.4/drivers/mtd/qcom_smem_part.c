/*
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/spi/spi.h>
#include <linux/module.h>

#include <linux/soc/qcom/smem.h>

/* Processor/host identifier for the application processor */
#define SMEM_HOST_APPS			0

/* SMEM items index */
#define SMEM_AARM_PARTITION_TABLE	9
#define SMEM_BOOT_FLASH_TYPE		421
#define SMEM_BOOT_FLASH_BLOCK_SIZE	424

/* SMEM Flash types */
#define SMEM_FLASH_NAND			2
#define SMEM_FLASH_SPI			6

#define SMEM_PART_NAME_SZ		16
#define SMEM_PARTS_MAX			32

struct smem_partition {
	char name[SMEM_PART_NAME_SZ];
	__le32 start;
	__le32 size;
	__le32 attr;
};

struct smem_partition_table {
	u8 magic[8];
	__le32 version;
	__le32 len;
	struct smem_partition parts[SMEM_PARTS_MAX];
};

/* SMEM Magic values in partition table */
static const u8 SMEM_PTABLE_MAGIC[] = {
	0xaa, 0x73, 0xee, 0x55,
	0xdb, 0xbd, 0x5e, 0xe3,
};

static int qcom_smem_get_flash_blksz(u64 **smem_blksz)
{
	size_t size;
	void *p;

	p = qcom_smem_get(SMEM_HOST_APPS, SMEM_BOOT_FLASH_BLOCK_SIZE,
			    &size);

	if (PTR_ERR(p) == -EPROBE_DEFER) {
		pr_err("Unable to read flash blksz from SMEM\n");
		return PTR_ERR(p);
	}

	if (size != sizeof(**smem_blksz)) {
		pr_err("Invalid flash blksz size in SMEM\n");
		return -EINVAL;
	}

	*smem_blksz = (u64 *) p;
	return 0;
}

static int qcom_smem_get_flash_type(u64 **smem_flash_type)
{
	size_t size;
	void *p;

	p = qcom_smem_get(SMEM_HOST_APPS, SMEM_BOOT_FLASH_TYPE,
			  &size);

	if (PTR_ERR(p) == -EPROBE_DEFER) {
		pr_err("Unable to read flash type from SMEM\n");
		return PTR_ERR(p);
	}

	if (size != sizeof(**smem_flash_type)) {
		pr_err("Invalid flash type size in SMEM\n");
		return -EINVAL;
	}

	*smem_flash_type = (u64 *) p;
	return 0;
}

static int qcom_smem_get_flash_partitions(struct smem_partition_table **pparts)
{
	size_t size;
	void *p;

	p = qcom_smem_get(SMEM_HOST_APPS, SMEM_AARM_PARTITION_TABLE,
			  &size);

	if (PTR_ERR(p) == -EPROBE_DEFER) {
		pr_err("Unable to read partition table from SMEM\n");
		return PTR_ERR(p);
	}

	*pparts = (u64 *) p;
	return 0;
}

static int of_dev_node_match(struct device *dev, void *data)
{
	return dev->of_node == data;
}

static bool is_spi_device(struct device_node *np)
{
	struct device *dev = NULL;

#ifdef CONFIG_SPI_MASTER
	dev = bus_find_device(&spi_bus_type, NULL, np, of_dev_node_match);
#endif

	if (!dev)
		return false;

	put_device(dev);
	return true;
}

static int parse_qcom_smem_partitions(struct mtd_info *master,
				      struct mtd_partition **pparts,
				      struct mtd_part_parser_data *data)
{
	struct smem_partition_table *smem_parts;
	u64 *smem_flash_type, *smem_blksz;
	struct mtd_partition *mtd_parts;
	struct device_node *of_node = data->of_node;
	int i, ret;

	/*
	 * SMEM will only store the partition table of the boot device.
	 * If this is not the boot device, do not return any partition.
	 */
	ret = qcom_smem_get_flash_type(&smem_flash_type);
	if (ret < 0)
		return ret;

	if ((*smem_flash_type == SMEM_FLASH_NAND && !mtd_type_is_nand(master))
	    || (*smem_flash_type == SMEM_FLASH_SPI && !is_spi_device(of_node)))
		return 0;

	/*
	 * Just for sanity purpose, make sure the block size in SMEM matches the
	 * block size of the MTD device
	 */
	ret = qcom_smem_get_flash_blksz(&smem_blksz);
	if (ret < 0)
		return ret;

	if (*smem_blksz != master->erasesize) {
		pr_err("SMEM block size differs from MTD block size\n");
		return -EINVAL;
	}

	/* Get partition pointer from SMEM */
	ret = qcom_smem_get_flash_partitions(&smem_parts);
	if (ret < 0)
		return ret;

	if (memcmp(SMEM_PTABLE_MAGIC, smem_parts->magic,
		   sizeof(SMEM_PTABLE_MAGIC))) {
		pr_err("SMEM partition magic invalid\n");
		return -EINVAL;
	}

	/* Allocate and populate the mtd structures */
	mtd_parts = kcalloc(le32_to_cpu(smem_parts->len), sizeof(*mtd_parts),
			    GFP_KERNEL);
	if (!mtd_parts)
		return -ENOMEM;

	for (i = 0; i < smem_parts->len; i++) {
		struct smem_partition *s_part = &smem_parts->parts[i];
		struct mtd_partition *m_part = &mtd_parts[i];

		m_part->name = s_part->name;
		m_part->size = le32_to_cpu(s_part->size) * (*smem_blksz);
		m_part->offset = le32_to_cpu(s_part->start) * (*smem_blksz);

		/* "rootfs" conflicts with OpenWrt auto mounting */
		if (mtd_type_is_nand(master) && !strcmp(m_part->name, "rootfs"))
			m_part->name = "ubi";

		/*
		 * The last SMEM partition may have its size marked as
		 * something like 0xffffffff, which means "until the end of the
		 * flash device". In this case, truncate it.
		 */
		if (m_part->offset + m_part->size > master->size)
			m_part->size = master->size - m_part->offset;
	}

	*pparts = mtd_parts;

	return smem_parts->len;
}

static struct mtd_part_parser qcom_smem_parser = {
	.owner = THIS_MODULE,
	.parse_fn = parse_qcom_smem_partitions,
	.name = "qcom-smem",
};

static int __init qcom_smem_parser_init(void)
{
	register_mtd_parser(&qcom_smem_parser);
	return 0;
}

static void __exit qcom_smem_parser_exit(void)
{
	deregister_mtd_parser(&qcom_smem_parser);
}

module_init(qcom_smem_parser_init);
module_exit(qcom_smem_parser_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Olivari <mathieu@codeaurora.org>");
MODULE_DESCRIPTION("Parsing code for SMEM based partition tables");
