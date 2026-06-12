#pragma once

#if !defined(_WIN32)
namespace Rendering
{
    template<typename CreateRootFn>
    inline int RunRoot(const wchar_t*, uint32_t, uint32_t, CreateRootFn, const float* = nullptr)
    {
        return 1;
    }

    inline int RunGallery()
    {
        return 1;
    }
}
#else

#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>

#include "FriendlyControls.h"
#include "FriendlyInputWin32.h"
#include "GameWindow.h"
#include "GalleryCommon.h"
#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"
#include <cctype>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <array>
#include <unordered_map>
#include <algorithm>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")




#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
    class DescriptorHeap
    {
    public:
        bool Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible)
        {
            Destroy();
            if (!device || numDescriptors == 0)
                return false;

            D3D12_DESCRIPTOR_HEAP_DESC desc {};
            desc.Type = type;
            desc.NumDescriptors = numDescriptors;
            desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap))))
                return false;

            m_descriptorSize = device->GetDescriptorHandleIncrementSize(type);
            m_descriptorCount = numDescriptors;
            m_nextDescriptor = 0;
            return true;
        }

        void Destroy()
        {
            if (m_heap)
            {
                m_heap->Release();
                m_heap = nullptr;
            }
            m_descriptorSize = 0;
            m_descriptorCount = 0;
            m_nextDescriptor = 0;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const
        {
            D3D12_CPU_DESCRIPTOR_HANDLE handle = m_heap ? m_heap->GetCPUDescriptorHandleForHeapStart() : D3D12_CPU_DESCRIPTOR_HANDLE {};
            handle.ptr += static_cast<SIZE_T>(index) * m_descriptorSize;
            return handle;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index) const
        {
            D3D12_GPU_DESCRIPTOR_HANDLE handle = m_heap ? m_heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE {};
            handle.ptr += static_cast<UINT64>(index) * m_descriptorSize;
            return handle;
        }

        bool Allocate(D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu)
        {
            if (!m_heap || m_nextDescriptor >= m_descriptorCount || !cpu || !gpu)
                return false;
            *cpu = GetCPUHandle(m_nextDescriptor);
            *gpu = GetGPUHandle(m_nextDescriptor);
            ++m_nextDescriptor;
            return true;
        }

        ID3D12DescriptorHeap* Get() const { return m_heap; }

    private:
        ID3D12DescriptorHeap* m_heap = nullptr;
        uint32_t m_descriptorSize = 0;
        uint32_t m_descriptorCount = 0;
        uint32_t m_nextDescriptor = 0;
    };
}

class Render
{
public:
    struct TextureDX12
    {
        FyGUI::TextureId Id = 0;
        uint32_t Width = 0;
        uint32_t Height = 0;
    };


public:
    Render() = default;

    HWND m_hwnd = nullptr;
    uint32_t m_Width { };
    uint32_t m_Height { };
    uint32_t m_FrameCount { 2 };


    ID3D12Device* device = nullptr;
    ID3D12CommandQueue* commandQueue = nullptr;
    IDXGISwapChain3* swapChain = nullptr;
    ID3D12Resource* renderTargets[2] {};
    ID3D12CommandAllocator* commandAlloc = nullptr;
    ID3D12GraphicsCommandList* commandList = nullptr;

    ID3D12Resource* depthStencilBuffer = nullptr;


    ID3D12PipelineState* pipelineState = nullptr;
    ID3D12RootSignature* rootSignature = nullptr;


    D3D12_VIEWPORT viewport = { };
    D3D12_RECT scissorRect = { };

    DescriptorHeap rtvDescriptorHeap {};
    DescriptorHeap dpvDescriptorHeap {};
    DescriptorHeap imguiSrvDescriptorHeap {};




    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ID3D12Fence* m_fence;
    UINT64 m_fenceValue;



    bool Initialize(HWND hwnd, uint32_t width, uint32_t Heigh)
    {
        m_hwnd = hwnd;
        m_input.Attach(hwnd);
        m_Width = width;
        m_Height = Heigh;


        viewport = { 0, 0, (float)m_Width, (float)m_Height, 0.0f, 1.0f };
        scissorRect = { 0, 0, (long)m_Width, (long)m_Height };


        IDXGIFactory4* factory = nullptr;
        CreateDXGIFactory1(IID_PPV_ARGS(&factory));

        D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));



        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));


        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = m_FrameCount;
        swapChainDesc.Width = m_Width;
        swapChainDesc.Height = m_Height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        IDXGISwapChain1* tempSwapChain = nullptr;
        factory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &tempSwapChain);


        tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain));
        factory->Release();


        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAlloc));
        device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAlloc, nullptr, IID_PPV_ARGS(&commandList));

        commandList->Close();


        CreateSynchronizationObjects();
        CreateRenderTargetViews();
        CreateDepthBuffer();




        if (!InitializeDearImGui(hwnd))
            throw std::runtime_error("Failed to initialize Dear ImGui DX12 render path.");


        m_buttonTexture = LoadAssets(L"../../Assets/Textures/button_brown_close.png");
        m_buttonTexture2 = LoadAssets(L"../../Assets/Textures/button_brown.png");
        m_sliderTexture = LoadAssets(L"../../Assets/Textures/pattern_diagonal_grey_large.png");
        m_sliderTexture2 = LoadAssets(L"../../Assets/Textures/pattern_grid_paper.png");
		m_bannerTexture = LoadAssets(L"../../Assets/Textures/banner_modern.png");
        m_panelGreyBoltsTexture = LoadAssets(L"../../Assets/Textures/panel_grey_bolts.png");
        m_panelGreyBlueTexture = LoadAssets(L"../../Assets/Textures/panel_grey_blue.png");
        m_panelGridBlueprintTexture = LoadAssets(L"../../Assets/Textures/panel_grid_blueprint.png");
        m_panelGreyDarkTexture = LoadAssets(L"../../Assets/Textures/panel_grey_dark.png");
        m_panelGreyBoltsDarkTexture = LoadAssets(L"../../Assets/Textures/panel_grey_bolts_dark.png");
        m_panelBorderGreyTexture = LoadAssets(L"../../Assets/Textures/panel_border_grey.png");
        m_buttonGreyTexture = LoadAssets(L"../../Assets/Textures/button_grey.png");
        m_buttonGreyCloseTexture = LoadAssets(L"../../Assets/Textures/button_grey_close.png");
        m_buttonRedTexture = LoadAssets(L"../../Assets/Textures/button_red.png");
        m_buttonRedCloseTexture = LoadAssets(L"../../Assets/Textures/button_red_close.png");
        m_progressBlueTexture = LoadAssets(L"../../Assets/Textures/progress_blue.png");
        m_progressBlueBorderTexture = LoadAssets(L"../../Assets/Textures/progress_blue_border.png");
        m_progressGreenTexture = LoadAssets(L"../../Assets/Textures/progress_green.png");
        m_progressRedTexture = LoadAssets(L"../../Assets/Textures/progress_red.png");
        m_scrollbarGreyTexture = LoadAssets(L"../../Assets/Textures/scrollbar_grey.png");
        m_scrollbarGreySmallTexture = LoadAssets(L"../../Assets/Textures/scrollbar_grey_small.png");
        m_minimapRingTexture = LoadAssets(L"../../Assets/Textures/minimap_ring_grey_detail.png");
        m_minimapArrowTexture = LoadAssets(L"../../Assets/Textures/minimap_arrow_a.png");
        m_minimapStarTexture = LoadAssets(L"../../Assets/Textures/minimap_icon_star_yellow.png");
        m_hexGreyTexture = LoadAssets(L"../../Assets/Textures/hexagon_grey.png");
        m_hexGreenTexture = LoadAssets(L"../../Assets/Textures/hexagon_grey_green.png");
        m_hexRedTexture = LoadAssets(L"../../Assets/Textures/hexagon_grey_red.png");
        m_roundGreyTexture = LoadAssets(L"../../Assets/Textures/round_grey.png");
        m_roundGreyDarkTexture = LoadAssets(L"../../Assets/Textures/round_grey_dark.png");
        m_titleTexture = LoadAssets(L"../../Assets/Textures/title.png");
        LoadGalleryIcons();
        m_galleryApp.SetIconResolver([this](const std::string& label) { return GalleryIconTexture(label); });
        m_galleryApp.SetTextureResolver([this](const std::string& name) { return ResolveXamlTexture(name); });
        m_galleryApp.SetInitialToggles(m_showFocusVisuals, m_showDragGhost);
        

        return true;
    }

    TextureDX12 m_buttonTexture;
    TextureDX12 m_buttonTexture2;
    TextureDX12 m_sliderTexture;
    TextureDX12 m_sliderTexture2;
    TextureDX12 m_bannerTexture;
    TextureDX12 m_panelGreyBoltsTexture;
    TextureDX12 m_panelGreyBlueTexture;
    TextureDX12 m_panelGridBlueprintTexture;
    TextureDX12 m_panelGreyDarkTexture;
    TextureDX12 m_panelGreyBoltsDarkTexture;
    TextureDX12 m_panelBorderGreyTexture;
    TextureDX12 m_buttonGreyTexture;
    TextureDX12 m_buttonGreyCloseTexture;
    TextureDX12 m_buttonRedTexture;
    TextureDX12 m_buttonRedCloseTexture;
    TextureDX12 m_progressBlueTexture;
    TextureDX12 m_progressBlueBorderTexture;
    TextureDX12 m_progressGreenTexture;
    TextureDX12 m_progressRedTexture;
    TextureDX12 m_scrollbarGreyTexture;
    TextureDX12 m_scrollbarGreySmallTexture;
    TextureDX12 m_minimapRingTexture;
    TextureDX12 m_minimapArrowTexture;
    TextureDX12 m_minimapStarTexture;
    TextureDX12 m_hexGreyTexture;
    TextureDX12 m_hexGreenTexture;
    TextureDX12 m_hexRedTexture;
    TextureDX12 m_roundGreyTexture;
    TextureDX12 m_roundGreyDarkTexture;
    TextureDX12 m_titleTexture;
    std::unordered_map<std::string, TextureDX12> m_galleryIconTextures;

    static void AllocateImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu)
    {
        auto* heap = static_cast<DescriptorHeap*>(info ? info->UserData : nullptr);
        if (!heap || !heap->Allocate(outCpu, outGpu))
        {
            if (outCpu) *outCpu = {};
            if (outGpu) *outGpu = {};
        }
    }

    static void FreeImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE)
    {
        // The gallery allocates from a monotonic heap for this comparison path.
    }

    bool InitializeDearImGui(HWND hwnd)
    {
        if (!imguiSrvDescriptorHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512, true))
            return false;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        const auto fontBuildStart = std::chrono::high_resolution_clock::now();
        io.Fonts->Clear();
        io.Fonts->TexGlyphPadding = 1;
        ImFontConfig fontConfig {};
        fontConfig.OversampleH = 0;
        fontConfig.OversampleV = 0;
        fontConfig.PixelSnapH = true;
        ImFont* font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 18.0f, &fontConfig);
        if (!font)
            font = io.Fonts->AddFontDefault();

        if (font)
        {
            static const ImWchar mdl2Ranges[] = { 0xE700, 0xF8FF, 0 };
            ImFontConfig iconConfig {};
            iconConfig.MergeMode = true;
            iconConfig.PixelSnapH = true;
            iconConfig.GlyphMinAdvanceX = 18.0f;
            iconConfig.GlyphOffset = ImVec2(0.0f, 1.0f);
            io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segmdl2.ttf", 18.0f, &iconConfig, mdl2Ranges);
        }
        FyGUI::SetDefaultFont(font);
        const auto fontBuildEnd = std::chrono::high_resolution_clock::now();
        m_fontAtlasBuildMs = std::chrono::duration<float, std::milli>(fontBuildEnd - fontBuildStart).count();

        ImGui::StyleColorsLight();
        if (!ImGui_ImplWin32_Init(hwnd))
            return false;

        ImGui_ImplDX12_InitInfo initInfo {};
        initInfo.Device = device;
        initInfo.CommandQueue = commandQueue;
        initInfo.NumFramesInFlight = static_cast<int>(m_FrameCount);
        initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        initInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        initInfo.UserData = &imguiSrvDescriptorHeap;
        initInfo.SrvDescriptorHeap = imguiSrvDescriptorHeap.Get();
        initInfo.SrvDescriptorAllocFn = AllocateImGuiSrvDescriptor;
        initInfo.SrvDescriptorFreeFn = FreeImGuiSrvDescriptor;
        return ImGui_ImplDX12_Init(&initInfo);
    }

    FyGUI::TextureId CreateImGuiTextureRGBA8(const UINT8* pixels, UINT width, UINT height)
    {
        if (!pixels || width == 0 || height == 0)
            return 0;

        D3D12_RESOURCE_DESC textureDesc {};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Alignment = 0;
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES defaultHeap {};
        defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
        defaultHeap.CreationNodeMask = 1;
        defaultHeap.VisibleNodeMask = 1;

        ID3D12Resource* texture = nullptr;
        if (FAILED(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture))) || !texture)
            return 0;

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint {};
        UINT numRows = 0;
        UINT64 rowSizeInBytes = 0;
        UINT64 uploadSize = 0;
        device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &uploadSize);

        D3D12_RESOURCE_DESC uploadDesc {};
        uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadDesc.Alignment = 0;
        uploadDesc.Width = uploadSize;
        uploadDesc.Height = 1;
        uploadDesc.DepthOrArraySize = 1;
        uploadDesc.MipLevels = 1;
        uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadDesc.SampleDesc.Count = 1;
        uploadDesc.SampleDesc.Quality = 0;
        uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES uploadHeap {};
        uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeap.CreationNodeMask = 1;
        uploadHeap.VisibleNodeMask = 1;

        ID3D12Resource* uploadBuffer = nullptr;
        if (FAILED(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer))) || !uploadBuffer)
        {
            texture->Release();
            return 0;
        }

        UINT8* mapped = nullptr;
        if (SUCCEEDED(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mapped))) && mapped)
        {
            const UINT sourcePitch = width * 4;
            UINT8* destination = mapped + footprint.Offset;
            for (UINT y = 0; y < height; ++y)
                std::memcpy(destination + static_cast<size_t>(y) * footprint.Footprint.RowPitch, pixels + static_cast<size_t>(y) * sourcePitch, sourcePitch);
            uploadBuffer->Unmap(0, nullptr);
        }

        commandAlloc->Reset();
        commandList->Reset(commandAlloc, nullptr);

        D3D12_TEXTURE_COPY_LOCATION src {};
        src.pResource = uploadBuffer;
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint = footprint;

        D3D12_TEXTURE_COPY_LOCATION dst {};
        dst.pResource = texture;
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = 0;

        commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        D3D12_RESOURCE_BARRIER barrier {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = texture;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);

        commandList->Close();
        ID3D12CommandList* lists[] = { commandList };
        commandQueue->ExecuteCommandLists(1, lists);
        WaitForPreviousFrame();
        uploadBuffer->Release();

        D3D12_CPU_DESCRIPTOR_HANDLE cpu {};
        D3D12_GPU_DESCRIPTOR_HANDLE gpu {};
        if (!imguiSrvDescriptorHeap.Allocate(&cpu, &gpu))
        {
            texture->Release();
            return 0;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(texture, &srvDesc, cpu);
        m_imguiTextures.push_back(texture);
        return static_cast<FyGUI::TextureId>(gpu.ptr);
    }

    TextureDX12 LoadAssets(const wchar_t* filename)
    {
        return LoadTexture(filename);
    }

    TextureDX12 LoadTextureScaled(const wchar_t* filename, UINT targetWidth, UINT targetHeight)
    {
        TextureDX12 result {};
        if (targetWidth == 0 || targetHeight == 0)
            return result;

        IWICImagingFactory* wicFactory = nullptr;
        CoInitialize(nullptr);
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory))) || !wicFactory)
            return result;

        IWICBitmapDecoder* decoder = nullptr;
        if (FAILED(wicFactory->CreateDecoderFromFilename(filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder)) || !decoder)
        {
            wicFactory->Release();
            return result;
        }

        IWICBitmapFrameDecode* frame = nullptr;
        if (FAILED(decoder->GetFrame(0, &frame)) || !frame)
        {
            decoder->Release();
            wicFactory->Release();
            return result;
        }

        IWICBitmapScaler* scaler = nullptr;
        wicFactory->CreateBitmapScaler(&scaler);
        if (!scaler || FAILED(scaler->Initialize(frame, targetWidth, targetHeight, WICBitmapInterpolationModeFant)))
        {
            if (scaler) scaler->Release();
            frame->Release();
            decoder->Release();
            wicFactory->Release();
            return LoadTexture(filename);
        }

        IWICFormatConverter* converter = nullptr;
        wicFactory->CreateFormatConverter(&converter);
        if (!converter || FAILED(converter->Initialize(scaler, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
        {
            if (converter) converter->Release();
            scaler->Release();
            frame->Release();
            decoder->Release();
            wicFactory->Release();
            return result;
        }

        result.Width = targetWidth;
        result.Height = targetHeight;
        std::vector<UINT8> pixels(static_cast<size_t>(targetWidth) * static_cast<size_t>(targetHeight) * 4);
        converter->CopyPixels(nullptr, targetWidth * 4, static_cast<UINT>(pixels.size()), pixels.data());
        result.Id = CreateImGuiTextureRGBA8(pixels.data(), targetWidth, targetHeight);

        converter->Release();
        scaler->Release();
        frame->Release();
        decoder->Release();
        wicFactory->Release();
        return result;
    }

    TextureDX12 LoadTexture(const wchar_t* filename)
    {
        TextureDX12 result {};
        IWICImagingFactory* wicFactory = nullptr;
        CoInitialize(nullptr);
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory))) || !wicFactory)
            return result;

        IWICBitmapDecoder* decoder = nullptr;
        if (FAILED(wicFactory->CreateDecoderFromFilename(filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder)) || !decoder)
        {
            wicFactory->Release();
            return result;
        }

        IWICBitmapFrameDecode* frame = nullptr;
        if (FAILED(decoder->GetFrame(0, &frame)) || !frame)
        {
            decoder->Release();
            wicFactory->Release();
            return result;
        }

        IWICFormatConverter* converter = nullptr;
        wicFactory->CreateFormatConverter(&converter);
        if (!converter || FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
        {
            if (converter) converter->Release();
            frame->Release();
            decoder->Release();
            wicFactory->Release();
            return result;
        }

        UINT width, height;
        frame->GetSize(&width, &height);
        result.Width = width;
        result.Height = height;

        std::vector<UINT8> pixels(width * height * 4);
        converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data());
        result.Id = CreateImGuiTextureRGBA8(pixels.data(), width, height);

        converter->Release();
        frame->Release();
        decoder->Release();
        wicFactory->Release();

        return result;
    }

    void LoadGalleryIcon(const std::string& key, const wchar_t* filename)
    {
        TextureDX12 texture = LoadTexture(filename);
        if (texture.Id)
            m_galleryIconTextures[key] = texture;
    }

    void LoadGalleryIcons()
    {
        LoadGalleryIcon("button", L"../../Assets/WinUI/ControlImages/Button.png");
        LoadGalleryIcon("checkbox", L"../../Assets/WinUI/ControlImages/Checkbox.png");
        LoadGalleryIcon("combobox", L"../../Assets/WinUI/ControlImages/ComboBox.png");
        LoadGalleryIcon("radiobutton", L"../../Assets/WinUI/ControlImages/RadioButton.png");
        LoadGalleryIcon("toggleswitch", L"../../Assets/WinUI/ControlImages/ToggleSwitch.png");
        LoadGalleryIcon("slider", L"../../Assets/WinUI/ControlImages/Slider.png");
        LoadGalleryIcon("progressbar", L"../../Assets/WinUI/ControlImages/ProgressBar.png");
        LoadGalleryIcon("textbox", L"../../Assets/WinUI/ControlImages/TextBox.png");
        LoadGalleryIcon("textblock", L"../../Assets/WinUI/ControlImages/TextBlock.png");
        LoadGalleryIcon("listbox", L"../../Assets/WinUI/ControlImages/ListBox.png");
        LoadGalleryIcon("tabview", L"../../Assets/WinUI/ControlImages/TabView.png");
        LoadGalleryIcon("treeview", L"../../Assets/WinUI/ControlImages/ListBox.png");
        LoadGalleryIcon("contentdialog", L"../../Assets/WinUI/ControlImages/ContentDialog.png");
        LoadGalleryIcon("flyout", L"../../Assets/WinUI/ControlImages/ContentDialog.png");
        LoadGalleryIcon("infobar", L"../../Assets/WinUI/ControlImages/InfoBar.png");
        LoadGalleryIcon("teachingtip", L"../../Assets/WinUI/ControlImages/TeachingTip.png");
        LoadGalleryIcon("navigationview", L"../../Assets/WinUI/ControlImages/NavigationView.png");
        LoadGalleryIcon("navigationviewitem", L"../../Assets/WinUI/ControlImages/NavigationView.png");
        LoadGalleryIcon("commandbar", L"../../Assets/WinUI/ControlImages/Button.png");
        LoadGalleryIcon("color", L"../../Assets/WinUI/ControlImages/Shape.png");
        LoadGalleryIcon("shapes", L"../../Assets/WinUI/ControlImages/Shape.png");
        LoadGalleryIcon("shape", L"../../Assets/WinUI/ControlImages/Shape.png");
        LoadGalleryIcon("border", L"../../Assets/WinUI/ControlImages/Border.png");
        LoadGalleryIcon("canvas", L"../../Assets/WinUI/ControlImages/Canvas.png");
        LoadGalleryIcon("expander", L"../../Assets/WinUI/ControlImages/Expander.png");
        LoadGalleryIcon("scrollviewer", L"../../Assets/WinUI/ControlImages/ScrollViewer.png");
        LoadGalleryIcon("inventorygrid", L"../../Assets/WinUI/ControlImages/Grid.png");
        LoadGalleryIcon("radialmenu", L"../../Assets/WinUI/ControlImages/Shape.png");
        LoadGalleryIcon("commandwheel", L"../../Assets/WinUI/ControlImages/Shape.png");
    }

    void CreateSynchronizationObjects()
    {
        m_frameIndex = swapChain->GetCurrentBackBufferIndex();
        m_fenceValue = 0;

        device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));

        m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);

        if (m_fenceEvent == nullptr)
        {
            throw std::runtime_error("Failed to create fence event.");
        }

        WaitForPreviousFrame();
    }

    void WaitForPreviousFrame()
    {
        const UINT64 fence = m_fenceValue;
        commandQueue->Signal(m_fence, fence);
        m_fenceValue++;


        if (m_fence->GetCompletedValue() < fence)
        {
            m_fence->SetEventOnCompletion(fence, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        m_frameIndex = swapChain->GetCurrentBackBufferIndex();
    }


    void CreateRenderTargetViews()
    {

        rtvDescriptorHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_FrameCount, false);

        for (uint32_t i = 0; i < m_FrameCount; ++i)
        {
            ID3D12Resource* backBuffer = nullptr;
            swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap.GetCPUHandle(i);
            device->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);
            renderTargets[i] = backBuffer;
        }

    }


    void CreateDepthBuffer()
    {

        D3D12_RESOURCE_DESC depthStencilDesc = {};
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Width = m_Width;
        depthStencilDesc.Height = m_Height;
        depthStencilDesc.DepthOrArraySize = 1;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;


        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;



        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;



        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&depthStencilBuffer));


        dpvDescriptorHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);



        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2D.MipSlice = 0;

        device->CreateDepthStencilView(depthStencilBuffer, &dsvDesc, dpvDescriptorHeap.GetCPUHandle(0));
    }



    void OnUpdate()
    {
    }

    HCURSOR CursorHandle(FyGUI::Cursor cursor) const
    {
        switch (cursor)
        {
        case FyGUI::Cursor::Hand: return LoadCursor(nullptr, IDC_HAND);
        case FyGUI::Cursor::IBeam: return LoadCursor(nullptr, IDC_IBEAM);
        case FyGUI::Cursor::Crosshair: return LoadCursor(nullptr, IDC_CROSS);
        case FyGUI::Cursor::SizeWE: return LoadCursor(nullptr, IDC_SIZEWE);
        case FyGUI::Cursor::SizeNS: return LoadCursor(nullptr, IDC_SIZENS);
        case FyGUI::Cursor::SizeAll: return LoadCursor(nullptr, IDC_SIZEALL);
        case FyGUI::Cursor::NotAllowed: return LoadCursor(nullptr, IDC_NO);
        default: return LoadCursor(nullptr, IDC_ARROW);
        }
    }

    void ApplyFriendlyCursor(const FyGUI::Context& context) const
    {
        SetCursor(CursorHandle(context.GetCursor()));
    }

    void ApplyFriendlyCursor() const
    {
        SetCursor(CursorHandle(m_fyContext.GetCursor()));
    }

    bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui::GetCurrentContext())
            ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
        m_input.HandleMessage(hwnd, msg, wParam, lParam);
        switch (msg)
        {
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT)
            {
                ApplyFriendlyCursor();
                return true;
            }
            return false;
        default:
            return false;
        }
    }

    FyGUI::InputSnapshot BuildInput(float deltaTime)
    {
        return m_input.Build(deltaTime);
    }

    FyGUI::Vec2 CurrentImGuiViewportSize() const
    {
        if (ImGui::GetCurrentContext())
        {
            const ImVec2 display = ImGui::GetIO().DisplaySize;
            if (display.x > 0.0f && display.y > 0.0f)
                return { display.x, display.y };
        }
        return { static_cast<float>(m_Width), static_cast<float>(m_Height) };
    }

    static FyGUI::Key MapKey(WPARAM key)
    {
        if (key >= 'A' && key <= 'Z')
            return static_cast<FyGUI::Key>(static_cast<size_t>(FyGUI::Key::A) + (key - 'A'));
        if (key >= '0' && key <= '9')
            return static_cast<FyGUI::Key>(static_cast<size_t>(FyGUI::Key::Num0) + (key - '0'));
        switch (key)
        {
        case VK_TAB: return FyGUI::Key::Tab;
        case VK_RETURN: return FyGUI::Key::Enter;
        case VK_ESCAPE: return FyGUI::Key::Escape;
        case VK_SPACE: return FyGUI::Key::Space;
        case VK_BACK: return FyGUI::Key::Backspace;
        case VK_DELETE: return FyGUI::Key::DeleteKey;
        case VK_LEFT: return FyGUI::Key::Left;
        case VK_RIGHT: return FyGUI::Key::Right;
        case VK_UP: return FyGUI::Key::Up;
        case VK_DOWN: return FyGUI::Key::Down;
        case VK_HOME: return FyGUI::Key::Home;
        case VK_END: return FyGUI::Key::End;
        case VK_PRIOR: return FyGUI::Key::PageUp;
        case VK_NEXT: return FyGUI::Key::PageDown;
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
            return FyGUI::Key::Shift;
        case VK_CONTROL:
        case VK_LCONTROL:
        case VK_RCONTROL:
            return FyGUI::Key::Control;
        case VK_MENU:
        case VK_LMENU:
        case VK_RMENU:
            return FyGUI::Key::Alt;
        default:
            return FyGUI::Key::None;
        }
    }

    void SetKeyDown(FyGUI::Key key, bool down)
    {
        const size_t index = static_cast<size_t>(key);
        if (index > 0 && index < FyGUI::KeyCount)
            m_keyDown[index] = down;
    }

    bool IsKeyDown(FyGUI::Key key) const
    {
        const size_t index = static_cast<size_t>(key);
        return index < FyGUI::KeyCount && m_keyDown[index];
    }

    bool GamepadPressed(WORD buttons, WORD button) const
    {
        return (buttons & button) != 0 && (m_previousGamepadButtons & button) == 0;
    }

    static float NormalizeStick(SHORT value, SHORT deadZone)
    {
        const int v = static_cast<int>(value);
        if (std::abs(v) < deadZone)
            return 0.0f;
        const float denom = v < 0 ? 32768.0f : 32767.0f;
        return std::clamp(static_cast<float>(v) / denom, -1.0f, 1.0f);
    }

    FyGUI::Context m_fyContext;
    Gallery::App m_galleryApp;
    FyBackend::FriendlyInputWin32 m_input;
    float m_lastDxRenderMs = 0.0f;
    float m_fontAtlasBuildMs = 0.0f;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime {};
    bool m_showFocusVisuals = true;
    bool m_showDragGhost = true;
    std::array<bool, 3> m_mouseDown {};
    std::array<bool, 3> m_prevMouseDown {};
    std::array<bool, FyGUI::KeyCount> m_keyDown {};
    std::array<bool, FyGUI::KeyCount> m_prevKeyDown {};
    std::vector<uint32_t> m_textInputQueue;
    std::vector<ID3D12Resource*> m_imguiTextures;
    FyGUI::Vec2 m_previousPointer = {};
    float m_wheelAccumulator = 0.0f;
    WORD m_previousGamepadButtons = 0;

    static std::vector<int> CommonEmojiCodepoints()
    {
        return {
            0x203C, 0x2049, 0x2122, 0x2139, 0x2194, 0x2195, 0x2196, 0x2197, 0x2198, 0x2199,
            0x231A, 0x231B, 0x2328, 0x23CF, 0x23E9, 0x23EA, 0x23EB, 0x23EC, 0x23ED, 0x23EE,
            0x23EF, 0x23F0, 0x23F3, 0x25AA, 0x25AB, 0x25B6, 0x25C0, 0x2600, 0x2601, 0x260E,
            0x2611, 0x2615, 0x261D, 0x263A, 0x2640, 0x2642, 0x2660, 0x2663, 0x2665, 0x2666,
            0x2668, 0x267B, 0x267F, 0x2694, 0x2699, 0x26A0, 0x26A1, 0x26AA, 0x26AB, 0x26BD,
            0x26BE, 0x26C4, 0x26C5, 0x26D4, 0x26EA, 0x26F2, 0x26F3, 0x26F5, 0x26FA, 0x26FD,
            0x2705, 0x2708, 0x2709, 0x270A, 0x270B, 0x270C, 0x270D, 0x2714, 0x2716, 0x2728,
            0x2733, 0x2734, 0x2744, 0x2747, 0x274C, 0x274E, 0x2753, 0x2754, 0x2755, 0x2757,
            0x2764, 0x2795, 0x2796, 0x2797, 0x27A1, 0x2B05, 0x2B06, 0x2B07, 0x2B1B, 0x2B1C,
            0x2B50, 0x2B55, 0x1F004, 0x1F0CF, 0x1F300, 0x1F301, 0x1F308, 0x1F30D, 0x1F30E,
            0x1F30F, 0x1F310, 0x1F319, 0x1F31F, 0x1F320, 0x1F383, 0x1F389, 0x1F3AE, 0x1F3AF,
            0x1F3C6, 0x1F3D9, 0x1F3E0, 0x1F40D, 0x1F41B, 0x1F431, 0x1F436, 0x1F44D, 0x1F44E,
            0x1F44F, 0x1F451, 0x1F4A1, 0x1F4A5, 0x1F4A9, 0x1F4AA, 0x1F4AC, 0x1F4AF, 0x1F4B0,
            0x1F4BB, 0x1F4CC, 0x1F4CD, 0x1F4DD, 0x1F4E6, 0x1F514, 0x1F525, 0x1F527, 0x1F52B,
            0x1F550, 0x1F575, 0x1F5A5, 0x1F5A8, 0x1F5B1, 0x1F5B2, 0x1F600, 0x1F601, 0x1F602,
            0x1F603, 0x1F604, 0x1F605, 0x1F606, 0x1F609, 0x1F60A, 0x1F60D, 0x1F60E, 0x1F610,
            0x1F914, 0x1F916, 0x1F680, 0x1F6A7, 0x1F6E1, 0x1F6F8
        };
    }

    static FyGUI::TextureId TextureId(const TextureDX12& texture)
    {
        return texture.Id;
    }

    static std::string NormalizeTextureKey(std::string key)
    {
        for (char& c : key)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (c == '\\')
                c = '/';
        }
        const size_t slash = key.find_last_of('/');
        if (slash != std::string::npos)
            key = key.substr(slash + 1);
        const size_t dot = key.find_last_of('.');
        if (dot != std::string::npos)
            key = key.substr(0, dot);
        key.erase(std::remove_if(key.begin(), key.end(), [](char c) { return c == ' ' || c == '-' || c == '_'; }), key.end());
        return key;
    }

    FyGUI::TextureId GalleryIconTexture(const std::string& label) const
    {
        const std::string key = NormalizeTextureKey(label);
        const auto found = m_galleryIconTextures.find(key);
        return found == m_galleryIconTextures.end() ? 0 : TextureId(found->second);
    }

    FyGUI::TextureId ResolveXamlTexture(const std::string& name) const
    {
        const std::string key = NormalizeTextureKey(name);

        if (const auto icon = m_galleryIconTextures.find(key); icon != m_galleryIconTextures.end())
            return TextureId(icon->second);

        if (key == "buttongrey") return TextureId(m_buttonGreyTexture);
        if (key == "buttongreyclose") return TextureId(m_buttonGreyCloseTexture);
        if (key == "buttonred") return TextureId(m_buttonRedTexture);
        if (key == "buttonredclose") return TextureId(m_buttonRedCloseTexture);
        if (key == "panelgreybolts") return TextureId(m_panelGreyBoltsTexture);
        if (key == "panelgreyboltsdark") return TextureId(m_panelGreyBoltsDarkTexture);
        if (key == "panelgreyblue") return TextureId(m_panelGreyBlueTexture);
        if (key == "panelgridblueprint") return TextureId(m_panelGridBlueprintTexture);
        if (key == "progressblue") return TextureId(m_progressBlueTexture);
        if (key == "progressblueborder") return TextureId(m_progressBlueBorderTexture);
        if (key == "progressgreen") return TextureId(m_progressGreenTexture);
        if (key == "progressred") return TextureId(m_progressRedTexture);
        if (key == "scrollbargrey") return TextureId(m_scrollbarGreyTexture);
        if (key == "scrollbargreysmall") return TextureId(m_scrollbarGreySmallTexture);
        if (key == "roundgrey") return TextureId(m_roundGreyTexture);
        if (key == "roundgreydark") return TextureId(m_roundGreyDarkTexture);
        if (key == "hexagongrey") return TextureId(m_hexGreyTexture);
        if (key == "hexagongreydark") return TextureId(m_hexGreyTexture);
        if (key == "hexagongreygreen") return TextureId(m_hexGreenTexture);
        if (key == "hexagongreyred") return TextureId(m_hexRedTexture);
        if (key == "minimaparrowa") return TextureId(m_minimapArrowTexture);
        if (key == "minimapringgreydetail") return TextureId(m_minimapRingTexture);
        if (key == "minimapringgrey") return TextureId(m_minimapRingTexture);
        if (key == "minimapiconstaryellow") return TextureId(m_minimapStarTexture);
        if (key == "minimapiconexclamationred") return TextureId(m_hexRedTexture);
        if (key == "minimapiconjewelyellow") return TextureId(m_minimapStarTexture);
        if (key == "title") return TextureId(m_titleTexture);
        return 0;
    }

    void OnRender()
    {
        uint32_t backBufferIndex = swapChain->GetCurrentBackBufferIndex();

        commandAlloc->Reset();
        commandList->Reset(commandAlloc, nullptr);

        D3D12_RESOURCE_BARRIER toRenderTarget {};
        toRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toRenderTarget.Transition.pResource = renderTargets[backBufferIndex];
        toRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        toRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &toRenderTarget);

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dpvDescriptorHeap.GetCPUHandle(0);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap.GetCPUHandle(backBufferIndex);

        commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


        float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        const auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = 1.0f / 60.0f;
        if (m_lastFrameTime.time_since_epoch().count() != 0)
            deltaTime = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;

        const auto inputStart = std::chrono::high_resolution_clock::now();
        FyGUI::InputSnapshot input = BuildInput(deltaTime);
        const auto inputEnd = std::chrono::high_resolution_clock::now();

        Gallery::FrameDiagnostics diagnostics {};
        diagnostics.InputMs = std::chrono::duration<float, std::milli>(inputEnd - inputStart).count();
        diagnostics.BackendMs = m_lastDxRenderMs;
        diagnostics.FontBootMs = m_fontAtlasBuildMs;
        diagnostics.SimdReady = false;
        diagnostics.SimdPassed = false;
        diagnostics.BackendName = "ImGui DX12";

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        m_galleryApp.UpdateAndRenderImGui(input, CurrentImGuiViewportSize(), deltaTime, diagnostics, ImGui::GetBackgroundDrawList());
        ApplyFriendlyCursor(m_galleryApp.GetContext());

		// redering ImGui with classic style to measure DX12 render time without FyGUI overhead
        //ImGui::StyleColorsClassic();
        //ImGui::Button("Test", ImVec2(100, 50));

        const auto dxStart = std::chrono::high_resolution_clock::now();
        ImGui::Render();
        ID3D12DescriptorHeap* heaps[] = { imguiSrvDescriptorHeap.Get() };
        commandList->SetDescriptorHeaps(1, heaps);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
        const auto dxEnd = std::chrono::high_resolution_clock::now();
        m_lastDxRenderMs = std::chrono::duration<float, std::milli>(dxEnd - dxStart).count();

        D3D12_RESOURCE_BARRIER toPresent {};
        toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toPresent.Transition.pResource = renderTargets[backBufferIndex];
        toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &toPresent);

        commandList->Close();


        ID3D12CommandList* ppCommandLists[] = { commandList };
        commandQueue->ExecuteCommandLists(1, ppCommandLists);


        swapChain->Present(1, 0);



        WaitForPreviousFrame();
    }

    void OnRenderCurrentRoot(const float clearColor[4])
    {
        uint32_t backBufferIndex = swapChain->GetCurrentBackBufferIndex();

        commandAlloc->Reset();
        commandList->Reset(commandAlloc, nullptr);

        D3D12_RESOURCE_BARRIER toRenderTarget {};
        toRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toRenderTarget.Transition.pResource = renderTargets[backBufferIndex];
        toRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        toRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &toRenderTarget);

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dpvDescriptorHeap.GetCPUHandle(0);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap.GetCPUHandle(backBufferIndex);

        commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        const auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = 1.0f / 60.0f;
        if (m_lastFrameTime.time_since_epoch().count() != 0)
            deltaTime = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;

        FyGUI::InputSnapshot input = BuildInput(deltaTime);

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        m_fyContext.Update(input, CurrentImGuiViewportSize(), deltaTime, true);
        m_fyContext.Render(*ImGui::GetBackgroundDrawList());
        ImGui::Render();
        ID3D12DescriptorHeap* heaps[] = { imguiSrvDescriptorHeap.Get() };
        commandList->SetDescriptorHeaps(1, heaps);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

        D3D12_RESOURCE_BARRIER toPresent {};
        toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toPresent.Transition.pResource = renderTargets[backBufferIndex];
        toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &toPresent);

        commandList->Close();
        ID3D12CommandList* ppCommandLists[] = { commandList };
        commandQueue->ExecuteCommandLists(1, ppCommandLists);
        swapChain->Present(1, 0);
        WaitForPreviousFrame();
    }





    void OnResize(uint32_t newWidth, uint32_t newHeight)
    {
        m_Width = newWidth;
        m_Height = newHeight;

        commandQueue->Signal(m_fence, ++m_fenceValue);
        WaitForPreviousFrame();

        for (uint32_t i = 0; i < m_FrameCount; ++i)
        {
            if (renderTargets[i]) renderTargets[i]->Release();
        }
        if (depthStencilBuffer) depthStencilBuffer->Release();


        swapChain->ResizeBuffers(m_FrameCount, m_Width, m_Height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

        CreateRenderTargetViews();

        CreateDepthBuffer();


        viewport = { 0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, 1.0f };
        scissorRect = { 0, 0, static_cast<long>(m_Width), static_cast<long>(m_Height) };

    }


    void Cleanup()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        if (ImGui::GetCurrentContext())
            ImGui::DestroyContext();
        for (ID3D12Resource* texture : m_imguiTextures)
            if (texture) texture->Release();
        m_imguiTextures.clear();
        imguiSrvDescriptorHeap.Destroy();

        if (depthStencilBuffer)
            depthStencilBuffer->Release();


        if (pipelineState)
            pipelineState->Release();

        for (uint32_t i = 0; i < 2; ++i)
            if (renderTargets[i])
                renderTargets[i]->Release();

        rtvDescriptorHeap.Destroy();
        dpvDescriptorHeap.Destroy();

        if (swapChain)
            swapChain->Release();

        if (commandQueue)
            commandQueue->Release();

        if (device)
            device->Release();

        if (commandAlloc)
            commandAlloc->Release();

        if (commandList)
            commandList->Release();
    }
};







namespace Rendering
{
template<typename CreateRootFn>
inline int RunRoot(const wchar_t* title, uint32_t width, uint32_t height, CreateRootFn createRoot, const float* clearColor = nullptr)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    FyBackend::GameWindow window(width, height, title);
    if (!window.Initialize(GetModuleHandleW(nullptr)))
        return 1;

    Render render {};
    render.Initialize(window.GetHWND(), window.GetWidth(), window.GetHeight());

    std::shared_ptr<FyGUI::UIElement> root = createRoot();
    if (!root)
        return 2;
    render.m_fyContext.SetRoot(root);

    window.SetOnMessage([&render](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        return render.HandleMessage(hwnd, msg, wParam, lParam);
    });

    const float defaultClear[] = { 0.98f, 0.976f, 0.972f, 1.0f };
    const float* activeClear = clearColor ? clearColor : defaultClear;
    window.SetOnRender([&render, activeClear]() {
        render.OnRenderCurrentRoot(activeClear);
    });

    window.SetOnResize([&render](uint32_t w, uint32_t h) {
        render.OnResize(w, h);
    });

    return window.Run();
}

inline int RunGallery()
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    FyBackend::GameWindow win = { 2000, 1000, L"Gallery" };
    win.Initialize(GetModuleHandle(nullptr));

    Render render {};
    render.Initialize(win.GetHWND(), win.GetWidth(), win.GetHeight());

    win.SetOnMessage([&render](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            return render.HandleMessage(hwnd, msg, wParam, lParam);
        });

    win.SetOnUpdate([&render]
        {
        });




    win.SetOnRender([&render]
        {
            render.OnUpdate();
            render.OnRender();
        });


    win.SetOnResize([&render](UINT w, UINT h)
        {
            render.OnResize(w, h);
        });


    win.Run();

    return 0;
}
}
#endif
