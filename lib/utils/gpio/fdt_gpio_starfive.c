/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Starfive
 *
 * Authors:
 *   Minda.chen <Minda.chen@starfivetech.com>
 */

#include <sbi/riscv_io.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/gpio/fdt_gpio.h>

#define STARFIVE_GPIO_CHIP_MAX		2
#define STARFIVE_GPIO_PINS_DEF		64
#define STARFIVE_GPIO_OUTVAL		0x40
#define STARFIVE_GPIO_MASK      	0xff
#define STARFIVE_GPIO_REG_SHIFT_MASK	0x3
#define STARFIVE_GPIO_SHIFT_BITS     	0x3

struct starfive_gpio_chip {
	unsigned long addr;
	struct gpio_chip chip;
};

static unsigned int starfive_gpio_chip_count;
static struct starfive_gpio_chip starfive_gpio_chip_array[STARFIVE_GPIO_CHIP_MAX];

static int starfive_gpio_direction_output(struct gpio_pin *gp, int value)
{
	uint32_t val;
	unsigned long reg_addr;
	uint32_t bit_mask, shift_bits;
	struct starfive_gpio_chip *chip =
		container_of(gp->chip, struct starfive_gpio_chip, chip);

	/* set out en*/
	reg_addr = chip->addr + gp->offset;
	reg_addr &= ~(STARFIVE_GPIO_REG_SHIFT_MASK);

	val = readl((volatile void *)(reg_addr));
	shift_bits = (gp->offset & STARFIVE_GPIO_REG_SHIFT_MASK)
		<< STARFIVE_GPIO_SHIFT_BITS;
	bit_mask = STARFIVE_GPIO_MASK << shift_bits;

	val = readl((volatile void *)reg_addr);
	val &= ~bit_mask;
	writel(val, (volatile void *)reg_addr);

	return 0;
}

static void starfive_gpio_set(struct gpio_pin *gp, int value)
{
	uint32_t val;
	unsigned long reg_addr;
	uint32_t bit_mask, shift_bits;
	struct starfive_gpio_chip *chip =
		container_of(gp->chip, struct starfive_gpio_chip, chip);

	reg_addr = chip->addr + gp->offset;
	reg_addr &= ~(STARFIVE_GPIO_REG_SHIFT_MASK);

	shift_bits = (gp->offset & STARFIVE_GPIO_REG_SHIFT_MASK)
		<< STARFIVE_GPIO_SHIFT_BITS;
	bit_mask = STARFIVE_GPIO_MASK << shift_bits;
	/* set output value */
	val = readl((volatile void *)(reg_addr + STARFIVE_GPIO_OUTVAL));
	val &= ~bit_mask;
	val |= value << shift_bits;
	writel(val, (volatile void *)(reg_addr + STARFIVE_GPIO_OUTVAL));
}

extern struct fdt_gpio fdt_gpio_starfive;

static int starfive_gpio_init(void *fdt, int nodeoff, u32 phandle,
			    const struct fdt_match *match)
{
	int rc;
	struct starfive_gpio_chip *chip;
	uint64_t addr;

	if (STARFIVE_GPIO_CHIP_MAX <= starfive_gpio_chip_count)
		return SBI_ENOSPC;
	chip = &starfive_gpio_chip_array[starfive_gpio_chip_count];

	rc = fdt_get_node_addr_size(fdt, nodeoff, 0, &addr, NULL);
	if (rc)
		return rc;

	chip->addr = addr;
	chip->chip.driver = &fdt_gpio_starfive;
	chip->chip.id = phandle;
	chip->chip.ngpio = STARFIVE_GPIO_PINS_DEF;
	chip->chip.direction_output = starfive_gpio_direction_output;
	chip->chip.set = starfive_gpio_set;
	rc = gpio_chip_add(&chip->chip);
	if (rc)
		return rc;

	starfive_gpio_chip_count++;
	return 0;
}

static const struct fdt_match starfive_gpio_match[] = {
	{ .compatible = "starfive,jh7110-sys-pinctrl" },
	{ },
};

struct fdt_gpio fdt_gpio_starfive = {
	.match_table = starfive_gpio_match,
	.xlate = fdt_gpio_simple_xlate,
	.init = starfive_gpio_init,
};
