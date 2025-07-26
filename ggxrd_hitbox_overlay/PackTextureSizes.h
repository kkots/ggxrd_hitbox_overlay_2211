#pragma once

struct PackTextureSizes {
	int frameHeight = 0;
	int frameWidth = 0;  // rounded down total W / num
	bool everythingWiderByDefault = false;  // if total W / num decimal part >= 0.5
	int digitThickness = 0;
	bool drawDigits = false;
	bool condenseIntoOneProjectileMiniFramebar = false;
	inline bool operator==(const PackTextureSizes& other) const noexcept {
		return frameHeight == other.frameHeight
			&& frameWidth == other.frameWidth
			&& everythingWiderByDefault == other.everythingWiderByDefault
			&& digitThickness == other.digitThickness
			&& drawDigits == other.drawDigits
			&& condenseIntoOneProjectileMiniFramebar == other.condenseIntoOneProjectileMiniFramebar;
	}
	inline bool operator!=(const PackTextureSizes& other) const noexcept { return !(*this == other); }
};
