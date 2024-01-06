#pragma pack(1)
struct spare_boot_head_t  uboot_spare_head = 
{
    {
        /* jump_instruction */          
        ( 0xEA000000 | ( ( ( sizeof( struct spare_boot_head_t ) + sizeof(uboot_hash_value)+ sizeof( int ) - 1 ) / sizeof( int ) - 2 ) & 0x00FFFFFF ) ),
        UBOOT_MAGIC,
        STAMP_VALUE,
        ALIGN_SIZE,
        0,
        0,
        UBOOT_VERSION,
        UBOOT_PLATFORM,
        {CONFIG_SYS_TEXT_BASE}
    },
    {
        { 0 },		//dram para
        1008,			//run core clock
        1200,			//run core vol
        0,			//uart port
        {             //uart gpio
            {0}, {0}
        },
        0,			//twi port
        {             //twi gpio
        	{0}, {0}
        },
        0,			//work mode
        0,			//storage mode
        { {0} },		//nand gpio
        { 0 },		//nand spare data
        { {0} },		//sdcard gpio
        { 0 }, 		//sdcard spare data
        0,                          //secure os 
        UBOOT_START_SECTOR_IN_SDMMC, //OTA flag
        0,                           //dtb offset
        0,                           //boot_package_size
		0,							//dram_scan_size
        { 0 }			//reserved data
    }
};

#pragma pack()