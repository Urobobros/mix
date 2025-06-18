#include "hw.h"
#include "pc.h"
#include "pci.h"
#include "console.h"
#include "vga_int.h"

typedef struct PCIS3VIRGEVGAState {
    PCIDevice dev;
    VGAState vga;
} PCIS3VIRGEVGAState;

static void s3_virge_map(PCIDevice *pci_dev, int region_num,
                         uint32_t addr, uint32_t size, int type)
{
    PCIS3VIRGEVGAState *d = (PCIS3VIRGEVGAState *)pci_dev;
    VGAState *s = &d->vga;
    if (region_num == PCI_ROM_SLOT) {
        cpu_register_physical_memory(addr, s->bios_size, s->bios_offset);
    } else {
        cpu_register_physical_memory(addr, s->vram_size, s->vram_offset);
    }
}

static void pci_s3_virge_write_config(PCIDevice *d,
                                      uint32_t address, uint32_t val, int len)
{
    PCIS3VIRGEVGAState *s = container_of(d, PCIS3VIRGEVGAState, dev);
    vga_dirty_log_stop(&s->vga);
    pci_default_write_config(d, address, val, len);
    vga_dirty_log_start(&s->vga);
}

void pci_s3_virge_vga_init(PCIBus *bus, uint8_t *vga_ram_base,
                           unsigned long vga_ram_offset, int vga_ram_size,
                           unsigned long vga_bios_offset, int vga_bios_size)
{
    PCIS3VIRGEVGAState *d;
    VGAState *s;
    uint8_t *pci_conf;

    d = (PCIS3VIRGEVGAState *)pci_register_device(bus, "S3 ViRGE",
                                                  sizeof(PCIS3VIRGEVGAState),
                                                  -1, NULL,
                                                  pci_s3_virge_write_config);
    if (!d)
        return;
    s = &d->vga;

    vga_common_init(s, vga_ram_base, vga_ram_offset, vga_ram_size);
    vga_init(s);

    s->ds = graphic_console_init(s->update, s->invalidate,
                                 s->screen_dump, s->text_update, s);
    s->pci_dev = &d->dev;

    pci_conf = d->dev.config;
    pci_config_set_vendor_id(pci_conf, 0x5333);
    pci_config_set_device_id(pci_conf, 0x8a01);
    pci_config_set_class(pci_conf, PCI_CLASS_DISPLAY_VGA);
    pci_conf[0x0e] = 0x00;

    pci_register_io_region(&d->dev, 0, vga_ram_size,
                           PCI_ADDRESS_SPACE_MEM_PREFETCH, s3_virge_map);
    if (vga_bios_size != 0) {
        unsigned int bios_total_size = 1;
        s->bios_offset = vga_bios_offset;
        s->bios_size = vga_bios_size;
        while (bios_total_size < vga_bios_size)
            bios_total_size <<= 1;
        pci_register_io_region(&d->dev, PCI_ROM_SLOT, bios_total_size,
                               PCI_ADDRESS_SPACE_MEM_PREFETCH, s3_virge_map);
    }
}

