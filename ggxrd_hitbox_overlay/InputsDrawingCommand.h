#pragma once
enum InputsDrawingCommandIcon : char {
	INPUT_ICON_NONE,
	INPUT_ICON_RIGHT,
	INPUT_ICON_UPRIGHT,
	INPUT_ICON_UP,
	INPUT_ICON_UPLEFT,
	INPUT_ICON_LEFT,
	INPUT_ICON_DOWNLEFT,
	INPUT_ICON_DOWN,
	INPUT_ICON_DOWNRIGHT,
	INPUT_ICON_PUNCH,
	INPUT_ICON_KICK,
	INPUT_ICON_SLASH,
	INPUT_ICON_HEAVYSLASH,
	INPUT_ICON_DUST,
	INPUT_ICON_TAUNT,
	INPUT_ICON_SPECIAL,
	INPUT_ICON_LAST  // must be last
};

struct InputsDrawingCommand {
	InputsDrawingCommandIcon icon;
	bool dark;
	InputsDrawingCommand() = default;
	InputsDrawingCommand(InputsDrawingCommandIcon icon, bool dark) : icon(icon), dark(dark) { }
};
struct InputsDrawingCommandRow {
	unsigned char count = 0;
	InputsDrawingCommand cmds[8];
};
