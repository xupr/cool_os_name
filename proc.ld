OUTPUT_FORMAT(binary)
ENTRY(_start)
/*phys = 0x7c00;*/
lma = 0x600000;

/*MEMORY{
	flash(RX): ORIGIN = 0x7c00, LENGTH = 1K
	ram(RX): ORIGIN = 0x10000, LENGTH = 8K
}*/

SECTIONS{
	/*.boot phys : {
		boot = .;
		*(.boot)
	 } >flash*/
	GROUP lma : {
		.text : {
			*(.load)
			*(.text)	
			*(.rodata)
		}
	
		.data : {
			*(.data)
		} 
		
		.bss : {
			*(.bss)
		}
		
		.rest : {
			*(*)
		}
	} 
}

