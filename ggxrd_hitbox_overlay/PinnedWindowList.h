#pragma once

#define pinnableWindowsEnum \
	pinnableWindowsFunc(MainWindow, "##ggxrd_hitbox_overlay_main_window") \
	pinnableWindowsFunc(TensionData, "Tension Data") \
	pinnableWindowsFunc(BurstGain, "Burst Gain") \
	pinnableWindowsPairFunc(ComboDamage, "  Combo Damage & Combo Stun (P%d)") \
	pinnableWindowsFunc(SpeedHitstunProration, "Speed/Hitstun Proration/...") \
	pinnableWindowsFunc(Projectiles, "Projectiles") \
	pinnableWindowsPairFunc(CharSpecific, "  Character Specific (P%d)") \
	pinnableWindowsFunc(BoxExtents, "Box Extents") \
	pinnableWindowsPairFunc(Cancels, "  Cancels (P%d)") \
	pinnableWindowsPairFunc(DamageCalculation, "  Damage/RISC/Stun Calculation (P%d)") \
	pinnableWindowsPairFunc(StunMash, "  Stun/Stagger Mash (P%d)") \
	pinnableWindowsPairFunc(ComboRecipe, "  Combo Recipe (P%d)") \
	pinnableWindowsFunc(HighlightedCancels, "  Highlighted Cancels") \
	pinnableWindowsFunc(FrameAdvantageHelp, "Frame Advantage Help") \
	pinnableWindowsFunc(StartupFieldHelp, "'Startup' Field Help") \
	pinnableWindowsFunc(ActiveFieldHelp, "'Active' Field Help") \
	pinnableWindowsFunc(TotalFieldHelp, "'Total' Field Help") \
	pinnableWindowsFunc(InvulFieldHelp, "Invul Help") \
	pinnableWindowsFunc(HitboxesHelp, "Hitboxes Help") \
	pinnableWindowsFunc(FramebarHelp, "Framebar Help") \
	pinnableWindowsFunc(Search, "Search") \
	pinnableWindowsFunc(Error, "Error") \
	pinnableWindowsFunc(ShaderCompilationError, "Shader compilation error") \
	pinnableWindowsFunc(RankIconDrawingHookError, "Failed to hook rank icon drawing") \
	pinnableWindowsFunc(HitboxEditor, "Hitbox Editor")

enum PinnedWindowEnum {
	#define pinnableWindowsFunc(name, title) PinnedWindowEnum_##name,
	#define pinnableWindowsPairFunc(name, titleFmtString) PinnedWindowEnum_##name##_1, PinnedWindowEnum_##name##_2,
	pinnableWindowsEnum
	#undef pinnableWindowsFunc
	#undef pinnableWindowsPairFunc
	PinnedWindowEnum_Last
};

struct PinnedWindowElement {
	bool isPinned = false;
	int order = 0;
};

struct PinnedWindowList {
	PinnedWindowElement elements[PinnedWindowEnum_Last] { { false, 0 } };
	inline PinnedWindowElement& operator[] (int index) { return elements[index]; }
	inline const PinnedWindowElement& operator[] (int index) const { return elements[index]; }
	inline operator void* () { return elements; }
	inline operator void const* () const { return elements; }
};
