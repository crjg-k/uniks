# switch.S

#include <asm.inc>

# void switch_to(struct context *old, struct context *new);
.global switch_to

.section .text
switch_to:
	tinyctxt_reg_save a0

	tinyctxt_reg_restore a1

	# will return to another ctxt according to ra since ra had been modified above
        ret