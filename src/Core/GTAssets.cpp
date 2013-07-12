// Copyright (c) 2013 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0

#include "Core/GTAssets.h"

using namespace std;
using namespace sf;
using namespace ssvu::FileSystem;
using namespace ssvs;
using namespace ssvs::Utils;
using namespace ssvuj;

namespace gt
{
	GTAssets::GTAssets()
	{
		assetManager.loadFolder("Data/");
		assetManager.loadBitmapFont("limeStroked", assetManager.getTexture("limeStroked.png"), as<BitmapFontData>(getRootFromFile("Data/lime.json")));
	}

	// Getters
	AssetManager& GTAssets::getAssetManager()			{ return assetManager; }
	Texture& GTAssets::getTexture(const string& mId)	{ return assetManager.getTexture(mId); }
}
