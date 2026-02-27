SECTION_PROLOGUE(.resource_table,, SUBALIGN(4))
{
	KEEP(*(.resource_table*))
} GROUP_DATA_LINK_IN(RAM, FLASH)
