[section .rdata]

global _zresource_start, _zresource_end
_zresource_start:
	incbin "archive.gz"
_zresource_end:
