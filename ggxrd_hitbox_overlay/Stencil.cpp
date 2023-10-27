#include "pch.h"
#include "Stencil.h"
#include "logging.h"

void Stencil::onEndSceneStart() {
	initialized = false;
}

void Stencil::initialize(IDirect3DDevice9* device) {
	if (direct3DError) return;
	if (!direct3DSuccess) {

		CComPtr<IDirect3D9> d3d = NULL;
		D3DCAPS9 caps;
		D3DDISPLAYMODE d3dDisplayMode{ 0 };

		if (FAILED(device->GetDirect3D(&d3d)) || !d3d) {
			log(fputs("GetDirect3D failed\n", logfile));
			direct3DError = true;
			return;
		}
		else {
			log(fputs("GetDirect3D succeeded\n", logfile));
		}

		SecureZeroMemory(&caps, sizeof(caps));
		if (FAILED(device->GetDeviceCaps(&caps))) {
			log(fputs("GetDeviceCaps failed\n", logfile));
			direct3DError = true;
			return;
		}
		else {
			log(fputs("GetDeviceCaps succeeded\n", logfile));
		}

		if (FAILED(device->GetDisplayMode(0, &d3dDisplayMode))) {
			log(fputs("GetDisplayMode failed\n", logfile));
			direct3DError = true;
			return;
		}
		else {
			log(fputs("GetDisplayMode succeeded\n", logfile));
		}

		if (FAILED(d3d->CheckDeviceFormat(
			caps.AdapterOrdinal,
			caps.DeviceType,
			d3dDisplayMode.Format,
			D3DUSAGE_DEPTHSTENCIL,
			D3DRTYPE_SURFACE,
			D3DFMT_D24S8))) {
			log(fputs("CheckDeviceFormat failed\n", logfile));
			direct3DError = true;
			return;
		}
		else {
			log(fputs("CheckDeviceFormat succeeded\n", logfile));
		}

		D3DSURFACE_DESC renderTargetDesc;
		SecureZeroMemory(&renderTargetDesc, sizeof(renderTargetDesc));
		DWORD renderTargetIndex = 0;
		for (; ; ++renderTargetIndex) {
			CComPtr<IDirect3DSurface9> renderTarget;
			HRESULT getRenderTargetResult = device->GetRenderTarget(renderTargetIndex, &renderTarget);
			if (getRenderTargetResult == D3DERR_NOTFOUND) {
				break;
			}
			if (FAILED(getRenderTargetResult)) {
				log(fputs("GetRenderTarget failed\n", logfile));
				direct3DError = true;
				return;
			}

			if (FAILED(renderTarget->GetDesc(&renderTargetDesc))) {
				log(fputs("GetDesc failed\n", logfile));
				direct3DError = true;
				return;
			}
		}
		log(fprintf(logfile, "GetRenderTargetResult returned %d targets\n", renderTargetIndex));

		if (FAILED(d3d->CheckDepthStencilMatch(caps.AdapterOrdinal,
			caps.DeviceType,
			d3dDisplayMode.Format,
			renderTargetDesc.Format,
			D3DFMT_D24S8))) {
			log(fputs("CheckDepthStencilMatch failed\n", logfile));
			direct3DError = true;
			return;
		}
		else {
			log(fputs("CheckDepthStencilMatch succeeded\n", logfile));
		}

		if (FAILED(device->CreateDepthStencilSurface(
			renderTargetDesc.Width,
			renderTargetDesc.Height,
			D3DFMT_D24S8,
			renderTargetDesc.MultiSampleType,
			renderTargetDesc.MultiSampleQuality,
			TRUE,
			&surface,
			NULL))) {
			log(fputs("CreateDepthStencilSurface failed\n", logfile));
			direct3DError = true;
			return;
		}
		else {
			log(fputs("CreateDepthStencilSurface succeeded\n", logfile));
		}

		direct3DSuccess = true;
	}

	if (!initialized) {
		if (FAILED(device->SetDepthStencilSurface(surface))) {
			log(fputs("SetDepthStencilSurface failed\n", logfile));
			direct3DError = true;
			return;
		}
		else {
			log(fputs("SetDepthStencilSurface succeeded\n", logfile));
		}
		device->Clear(0, NULL, D3DCLEAR_STENCIL, D3DCOLOR{}, 1.f, 0);

		initialized = true;

	}
}

void Stencil::onEndSceneEnd(IDirect3DDevice9* device) {
	if (initialized) {
		if (FAILED(device->SetDepthStencilSurface(NULL))) {
			log(fputs("SetDepthStencilSurface to NULL failed\n", logfile));
			direct3DError = true;
		}
		else {
			log(fputs("SetDepthStencilSurface to NULL succeeded\n", logfile));
		}
		device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
	}
}

void Stencil::clearRegion(IDirect3DDevice9* device, const BoundingRect& boundingRect) {
	if (boundingRect.emptyX || !initialized) return;

	D3DRECT rect{ (LONG)boundingRect.left - 1, (LONG)boundingRect.top - 1, (LONG)boundingRect.right + 1, (LONG)boundingRect.bottom + 1 };
	device->Clear(1, &rect, D3DCLEAR_STENCIL, D3DCOLOR{}, 1.f, 0);
}
