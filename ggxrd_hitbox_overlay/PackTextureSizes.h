#pragma once

struct PackTextureSizes {
	int frameHeight;
	int frameWidth;  // rounded down total W / num
	bool everythingWiderByDefault;  // if total W / num decimal part >= 0.5
	inline bool operator==(const PackTextureSizes& other) const noexcept {
		return frameHeight == other.frameHeight
			&& frameWidth == other.frameWidth
			&& everythingWiderByDefault == other.everythingWiderByDefault;
	}
	inline bool operator!=(const PackTextureSizes& other) const noexcept { return !(*this == other); }
};
