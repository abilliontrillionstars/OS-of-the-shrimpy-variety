#include "utils.h"
#include "pci.h"

u32 pci_scan_for_device(unsigned class, unsigned subclass)
{
    for(int bus=0;bus<256;++bus)
        for(int dev=0;dev<32;++dev)
            for(int func=0;func<8;++func)
            {
                unsigned tmp = pci_read(bus,dev,func,2);
                unsigned devclass = tmp>>24;
                unsigned devsubclass = (tmp>>16)&0xff;
                if(class == devclass && subclass == devsubclass)
                    return 1<<31 |  //PCI expects bit 31==1
                        (bus<<16) | (dev<<11) | (func<<8);
            }
    return 0;   //not found
}

u32 pci_read_addr(u32 addr, u32 reg)
{
    addr |= (reg<<2);
    outl(0xcf8,addr);       //select PCI location to read
    u32 val = inl(0xcfc);   //read it
    return val;
}
void pci_write_addr(u32 addr, u32 reg, u32 val)
{
    addr |= (reg<<2);
    outl(0xcf8,addr);   //select PCI location to write
    outl(0xcfc,val);    //write it
}

//convenience function if we have bus/dev/func separately
u32 pci_read(u32 bus, u32 dev, u32 func, u32 reg)
{
    //high bit must always be set
    unsigned addr = 0x80000000;
    addr |= (bus<<16);
    addr |= (dev<<11);
    addr |= (func<<8);
    return pci_read_addr(addr,reg);
}

