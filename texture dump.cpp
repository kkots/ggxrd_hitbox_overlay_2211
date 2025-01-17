
char* fnameArray = nullptr;
const char* readFName(unsigned int fnameId) {
    char* fnamePtr = *(char**)(fnameArray + fnameId * 4);
    return (const char*)(fnamePtr + 0x10);
}

void Graphics::dumpTextures(IDirect3DDevice9* device) {
	static bool didThisAlready = false;
	if (didThisAlready) return;
	didThisAlready = true;
	
	uintptr_t start, end;
	getModuleBounds("GuiltyGearXrd.exe:all", &start, &end);
	// Font* StaticFont: 0x01f34f5c
	// Font::0x3c - TArray<FontCharacter>. (For StaticFont) First three chars unknown, then space, then !.
	// Font* StaticREDFont: 0x01f34f60
	// Font* ProfileFont: 0x01f34f64
	
	fnameArray = *(char**)(start + 0x16C42D0);
	struct TextureArrayItem {
		unsigned int fName;
		unsigned int garbage;
		char* texture;  // UTexture2D* texture;
	};
	struct REDTexture2DArray {
		void* vtable;
		char uobject[0x38];
		TextureArrayItem* Data;
		int ArrayNum;
		int ArrayMax;
	};
	
	char* gamePtr = *(char**)(start + 0x01A4059C);  // this is gameDataPtr from Game.h, it's an REDGameCommon
	// 0x68: Font* StaticFont;
	// 0x6c: Font* StaticREDFont;
	// 0x70: Font* StaticJapanFont;
	// 0x74: Font* ProfileFont;
	// 0x114: REDTexture2DArray* StaticTextures;
	// 0x118: REDTexture2DArray* LoadedStaticTexture;
	// 0x11c: REDTexture2DArray* ExternalTextures[4];
	REDTexture2DArray* textureArray = *(REDTexture2DArray**)(gamePtr + 0x118);
	if (textureArray == nullptr) return;
	for (int i = 0; i < textureArray->ArrayNum; ++i) {
		TextureArrayItem* elem = textureArray->Data + i;
		std::wstring name = L"your dump path\\";
		const char* fname = readFName(elem->fName);
		while (*fname != '\0') {
			name += (wchar_t)*fname;
			++fname;
		}
		name += L".png";
		char* firstTexture2D = elem->texture;
		if (!firstTexture2D) continue;
		
	    char* fTextureResource = *(char**)(firstTexture2D + 0x3c + 0x84);  // FTextureResource* Resource
	    if (!fTextureResource) continue;
	    char* fTextureRHIRef = fTextureResource + 0x14;  // pointer to the FTextureRHIRef TextureRHI member (I haven't read its value yet)
	    char* tDynamicRHIResource = *(char**)fTextureRHIRef;  // TDynamicRHIResource<RRT_Texture>* Reference
	    if (!tDynamicRHIResource) continue;
	    // pointers to instances of TDynamicRHIResource<RRT_Texture> class are pointing at offset 0xc into a TD3D9Texture<IDirect3DBaseTexture9,RRT_Texture>
	    char* td3dTexture = tDynamicRHIResource - 0xc;
	    IDirect3DTexture9* texture = *(IDirect3DTexture9**)(td3dTexture + 0x8);
	    D3DSURFACE_DESC desc;
	    memset(&desc, 0, sizeof desc);
	    texture->GetLevelDesc(0, &desc);
	    
		RECT rect;
		rect.left = 0;
		rect.right = desc.Width;
		rect.top = 0;
		rect.bottom = desc.Height;
		
		bool failed = false;
		CComPtr<IDirect3DSurface9> oldRenderTarget = nullptr;
		failed = FAILED(device->GetRenderTarget(0, &oldRenderTarget));
		CComPtr<IDirect3DSurface9> surface1 = nullptr;
		if (!failed) {
			failed = FAILED(device->CreateRenderTarget(
				desc.Width,
				desc.Height,
				D3DFMT_A8R8G8B8,
				D3DMULTISAMPLE_NONE,
				0,
				TRUE,
				&surface1,
				NULL));
		}
		if (!failed) {
			device->SetRenderTarget(0, surface1);
		}
		CComPtr<IDirect3DVertexBuffer9> textureVertexBuffer = nullptr;
		CComPtr<IDirect3DStateBlock9> oldState = nullptr;
		struct TextureVertex {
			float x, y, z, rhw;
			float u, v;
			TextureVertex() = default;
			TextureVertex(float x, float y, float z, float rhw, float u, float v)
				: x(x), y(y), z(z), rhw(rhw), u(u), v(v) { };
		};
		if (!failed) {
			failed = FAILED(device->CreateVertexBuffer(
				sizeof(TextureVertex) * 4,
				D3DUSAGE_DYNAMIC,
				D3DFVF_XYZRHW | D3DFVF_TEX1,
				D3DPOOL_DEFAULT,
				&textureVertexBuffer,
				NULL));
		}
		TextureVertex vertices[4] {
			{ 0.0F, 0.0F, 0.0F, 1.F, 0.F, 0.F },
			{ 0.0F, (float)desc.Height, 0.0F, 1.F, 0.F, 1.F },
			{ (float)desc.Width, 0.0F, 0.0F, 1.F, 1.F, 0.F },
			{ (float)desc.Width, (float)desc.Height, 0.0F, 1.F, 1.F, 1.F }
		};
		TextureVertex* buffer = nullptr;
		if (!failed) {
			failed = FAILED(textureVertexBuffer->Lock(0, 0, (void**)&buffer, D3DLOCK_DISCARD));
		}
		if (!failed) {
			memcpy(buffer, vertices, sizeof(TextureVertex) * 4);
			textureVertexBuffer->Unlock();
			device->CreateStateBlock(D3DSBT_ALL, &oldState);
			device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0xff), 1.F, 0);
			device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			device->SetTexture(0, texture);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
			device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO);
			device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE);
			device->SetVertexShader(nullptr);
			device->SetPixelShader(nullptr);
			device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
			device->SetStreamSource(0, textureVertexBuffer, 0, sizeof(TextureVertex));
			device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		}
		if (oldState) {
			oldState->Apply();
		}
		if (!failed) {
			this->device = device;
			//takeScreenshotSimple(device);
			takeScreenshotDebug(device, name.c_str());
		}
		if (oldRenderTarget) {
			device->SetRenderTarget(0, oldRenderTarget);
		}
	}
	
}
