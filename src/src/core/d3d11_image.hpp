// Pure D3D11 + WIC replacement for D3DX11CreateShaderResourceViewFromMemory
// No DirectX SDK required 
#pragma once
#include <vector>
#include <memory>
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>

#pragma comment(lib, "windowscodecs.lib")

inline HRESULT CreateSRVFromMemory(
    ID3D11Device*           device,
    const void*             data,
    size_t                  data_size,
    ID3D11ShaderResourceView** out_srv)
{
    if (!device || !data || !data_size || !out_srv) return E_INVALIDARG;
    *out_srv = nullptr;

    using Microsoft::WRL::ComPtr;

    static ComPtr<IWICImagingFactory> g_wic;
    if (!g_wic) {
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_wic));
        if (FAILED(hr)) return hr;
    }

    ComPtr<IWICStream> stream;
    HRESULT hr = g_wic->CreateStream(&stream);
    if (FAILED(hr)) return hr;

    hr = stream->InitializeFromMemory(
        const_cast<BYTE*>(static_cast<const BYTE*>(data)),
        static_cast<DWORD>(data_size));
    if (FAILED(hr)) return hr;

    ComPtr<IWICBitmapDecoder> decoder;
    hr = g_wic->CreateDecoderFromStream(
        stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr)) return hr;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) return hr;

    UINT width = 0, height = 0;
    frame->GetSize(&width, &height);

    ComPtr<IWICFormatConverter> converter;
    hr = g_wic->CreateFormatConverter(&converter);
    if (FAILED(hr)) return hr;

    hr = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone, nullptr, 0.f,
        WICBitmapPaletteTypeMedianCut);
    if (FAILED(hr)) return hr;

    UINT row_bytes = width * 4;
    UINT total_bytes = row_bytes * height;
    std::vector<BYTE> pixels(total_bytes);
    hr = converter->CopyPixels(nullptr, row_bytes, total_bytes, pixels.data());
    if (FAILED(hr)) return hr;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width              = width;
    desc.Height             = height;
    desc.MipLevels          = 1;
    desc.ArraySize          = 1;
    desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count   = 1;
    desc.Usage              = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem            = pixels.data();
    init.SysMemPitch        = row_bytes;

    ComPtr<ID3D11Texture2D> tex;
    hr = device->CreateTexture2D(&desc, &init, &tex);
    if (FAILED(hr)) return hr;

    return device->CreateShaderResourceView(tex.Get(), nullptr, out_srv);
}
