#pragma once

extern "C" {
	void interrupts_enable();
	void interrupts_disable();
}

void interrupts_init();
