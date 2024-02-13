#include <linux/module.h>
#include <linux/io.h>

MODULE_LICENSE("GPL");

#define PHY_ADDR 0x80000000

static int __init mem_read_init(void)
{
    volatile uint32_t *vaddr;
    uint32_t value;

    vaddr = (volatile uint32_t *)__va(PHY_ADDR);
    value = *vaddr;

    *vaddr = 0xdeadbeef;

    pr_info("Physical address 0x%08lx: 0x%08x -> 0x%08x\n", PHY_ADDR, value, *vaddr);

    return -1;
}

static void __exit mem_read_exit(void)
{
    pr_info("Unloading module\n");
}

module_init(mem_read_init);
module_exit(mem_read_exit);
