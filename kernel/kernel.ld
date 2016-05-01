OUTPUT_FORMAT(binary)
ENTRY(_start)
phys = 0x7c00;
lma = 0x100000;

SECTIONS{ /*link everything so that the load section is first*/
	GROUP lma : {
		.text : {
			*(.load)
			*(.text)	
			*(.rodata)
		}

		.rest : {
			*(*)
		}
		
		.data : {
			*(.data)
		} 
		
		.bss : {
			*(.bss)
		}
	} 
}

