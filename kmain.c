__asm__(
    ".global _start\n"
    "_start:\n"
    "mov $0x10000,%esp\n"
    "call _kmain"
);

void kmain(){
    __asm__(
        "mov $0xbeefbeef,%eax\n"
        "mov $0xf00d1234,%ebx\n"
        "hlt"
    );
}