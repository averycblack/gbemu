#include <gb/gbinput.h>
#include <gb/gb.h>

int gbInput::readByte(UINT16 addr) {
	if (addr != 0xFF00) return -1;

	if (!enableActionBtns && !enableDirectionBtns)
		return 0xFF;

	// Top 2 bits are unused
	UINT8 ret = 0b11000000;
	if (enableActionBtns) ret |= actionVal | ACTION_SELECT_MASK;
	if (enableDirectionBtns) ret |= directionVal | DIRECTION_SELECT_MASK;
	return ret;
}

int gbInput::writeByte(UINT16 addr, UINT8 val) {
	if (addr != 0xFF00) return -1;

	// Enable = 0, Disable = 1
	enableActionBtns = !(val & ACTION_SELECT_MASK);
	enableDirectionBtns = !(val & DIRECTION_SELECT_MASK);
	return 0;
}

void gbInput::setKey(gbInputType type, bool inputDown) {
	// Buttons pull down to zero when pressed
	bool inputUp = !inputDown;
	UINT8 oldDir = directionVal;
	UINT8 oldAction = actionVal;

	switch (type) {
	case gbInputType::RIGHT: right = inputUp; break;
	case gbInputType::LEFT: left = inputUp; break;
	case gbInputType::UP: up = inputUp; break;
	case gbInputType::DOWN: down = inputUp; break;

	case gbInputType::A: a = inputUp; break;
	case gbInputType::B: b = inputUp; break;
	case gbInputType::SELECT: select = inputUp; break;
	case gbInputType::START: start = inputUp; break;
	}

	bool actionInterrupt = enableActionBtns && oldAction == actionVal;
	bool directionInterrupt = enableDirectionBtns && oldDir == directionVal;
	if (actionInterrupt || directionInterrupt) {
		g_gb->cpu->interruptFlags |= JOYPAD_INTR;
	}
}