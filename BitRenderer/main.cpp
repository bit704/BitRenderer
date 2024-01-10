#include <chrono>
#include <string>
#include <thread>

#include "image.h"
#include "logger.h"
#include "scene.h"
#include "ui_helper.h"
#include "resource.h"
#include "status.h"

using std::chrono::steady_clock;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::minutes;
using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;

std::atomic_ullong hit_count(0); // 击中次数统计
std::atomic_ullong sample_count(0); // 采样次数统计
std::atomic_bool rendering(false); // 标志是否正在渲染
std::string rendering_info = "";

int main()
{

    // 创建应用程序窗口
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, 
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui", nullptr };

    wc.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON_LOGO));
    wc.hIconSm = wc.hIcon;

    ::RegisterClassExW(&wc);

    // 获取桌面可用区域大小，最大化窗口
    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"BitRenderer", WS_OVERLAPPEDWINDOW, 
        rect.left, rect.top, width, height, nullptr, nullptr, wc.hInstance, nullptr);

    // 初始化Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // 因为需要将D3D12_CPU_DESCRIPTOR_HANDLE传给ImTextureID, 确保适合
    static_assert(sizeof(ImTextureID) >= sizeof(D3D12_CPU_DESCRIPTOR_HANDLE), 
        "D3D12_CPU_DESCRIPTOR_HANDLE is too large to fit in an ImTextureID");

    ID3D12Resource* my_texture = nullptr;

    // 获取着色器资源视图的CPU/GPU句柄
    UINT handle_increment = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    int descriptor_index = 1; // 使用第二个描述符
    D3D12_CPU_DESCRIPTOR_HANDLE my_texture_srv_cpu_handle = g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
    my_texture_srv_cpu_handle.ptr += (handle_increment * descriptor_index);
    D3D12_GPU_DESCRIPTOR_HANDLE my_texture_srv_gpu_handle = g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
    my_texture_srv_gpu_handle.ptr += (handle_increment * descriptor_index);

    // 显示窗口
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // 设置IMGUI上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); 
    (void)io;
    io.Fonts->AddFontFromFileTTF("./misc/DroidSans.ttf", 16);
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 键盘
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // 手柄

    // 设置IMGUI的样式
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();
    style.TabRounding    = 4.f;
    style.FrameRounding  = 4.f;
    style.GrabRounding   = 4.f;
    style.WindowRounding = 4.f;
    style.PopupRounding  = 4.f;

    // 设置Win32和DirectX 12后端
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(g_pd3dDevice, NUM_FRAMES_IN_FLIGHT,
        DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
        g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

    // 窗口设置项
    ImGuiComboFlags flags = 0;
    //ImGuiWindowFlags_ custom_window_flag = ImGuiWindowFlags_None;
    ImGuiWindowFlags_ custom_window_flag = (ImGuiWindowFlags_)(ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImVec4 clear_color = ImVec4(1.f, 1.f, 1.f, 1.f); // 背景颜色
    bool use_preset = true;

    // 渲染设置项
    const char* scenes[] = { "None", "scene_checker", "scene_cornell_box", "scene_composite1", "scene_composite2"};
    int scene_current_idx = 0;
    int image_width = 600;
    const char* aspect_ratios[] = { "16/9", "2/1", "3/2", "4/3", "1/1" };
    int aspect_ratio_idx = 0;
    double aspect_ratio = 16. / 9.;
    int samples_per_pixel = 100;
    int max_depth = 50;
    int vfov = 20;
    float lookfrom[3] = { 13, 2, 3 };
    float lookat[3] = { 0, 0, 0 };
    float vup[3] = { 0, 1, 0 };
    float background[3] = { .7f, .8f, 1 };

    // 渲染数据
    Camera cam;
    unsigned char* image_data = nullptr;
    std::thread t;

    // 统计数据
    auto rendering_start = steady_clock::now(); // 记录渲染开始时间
    nanoseconds duration(0); // 记录渲染用时
    float cpu_usage = 0.f;
    auto cpu_start = steady_clock::now(); // 记录CPU占用率计算间隔
    FILETIME cpu_idle_prev, cpu_kernel_prev, cpu_user_prev; // 记录CPU时间
    GetSystemTimes(&cpu_idle_prev, &cpu_kernel_prev, &cpu_user_prev);

    bool done = false;
    while (!done)
    {
        // 拉取并处理窗口消息
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // 开始IMGUI帧
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 设置
        {
            ImGui::Begin("SETUP", nullptr, custom_window_flag | ImGuiWindowFlags_MenuBar);

            // 菜单
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("Mode"))
                {
                    if (ImGui::MenuItem("Tracer"))
                    {

                    }
                    if (ImGui::MenuItem("Rasterizer"))
                    {

                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            
            ImGui::SeparatorText("scene");

            ImGui::Checkbox("use preset", &use_preset);            

            if (use_preset)
            {
                ImGui::SameLine();
                // 选择预置场景
                const char* combo_preview_value_scene = scenes[scene_current_idx];
                if (ImGui::BeginCombo("preset", combo_preview_value_scene, flags))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(scenes); n++)
                    {
                        const bool is_selected = (scene_current_idx == n);
                        if (ImGui::Selectable(scenes[n], is_selected))
                            scene_current_idx = n;

                        if (is_selected)
                            ImGui::SetItemDefaultFocus();

                        // 各场景推荐参数
                        if (scene_current_idx == 1)
                        {
                            image_width = 600;
                            aspect_ratio_idx = 0;
                            samples_per_pixel = 100;
                            max_depth = 50;
                            vfov = 20;
                            float lookfrom_t[3] = { 13, 2, 3 };
                            memcpy(lookfrom, lookfrom_t, 3 * sizeof(float));
                            float lookat_t[3] = { 0, 0, 0 };
                            memcpy(lookat, lookat_t, 3 * sizeof(float));
                            float vup_t[3] = { 0, 1, 0 };
                            memcpy(vup, vup_t, 3 * sizeof(float));
                            float background_t[3] = { .7f, .8f, 1 };
                            memcpy(background, background_t, 3 * sizeof(float));
                        }
                        else if (scene_current_idx == 2)
                        {
                            image_width = 600;
                            aspect_ratio_idx = 4;
                            samples_per_pixel = 1000;
                            max_depth = 50;
                            vfov = 40;
                            float lookfrom_t[3] = { 278, 278, -800 };
                            memcpy(lookfrom, lookfrom_t, 3 * sizeof(float));
                            float lookat_t[3] = { 278, 278, 0 };
                            memcpy(lookat, lookat_t, 3 * sizeof(float));
                            float vup_t[3] = { 0, 1, 0 };
                            memcpy(vup, vup_t, 3 * sizeof(float));
                            float background_t[3] = { 0, 0, 0 };
                            memcpy(background, background_t, 3 * sizeof(float));
                        }
                        else if (scene_current_idx == 3)
                        {
                            image_width = 600;
                            aspect_ratio_idx = 0;
                            samples_per_pixel = 100;
                            max_depth = 50;
                            vfov = 20;
                            float lookfrom_t[3] = { 13, 2, 3 };
                            memcpy(lookfrom, lookfrom_t, 3 * sizeof(float));
                            float lookat_t[3] = { 0, 0, 0 };
                            memcpy(lookat, lookat_t, 3 * sizeof(float));
                            float vup_t[3] = { 0, 1, 0 };
                            memcpy(vup, vup_t, 3 * sizeof(float));
                            float background_t[3] = { .7f, .8f, 1 };
                            memcpy(background, background_t, 3 * sizeof(float));
                        }
                        else if (scene_current_idx == 4)
                        {
                            image_width = 600;
                            aspect_ratio_idx = 4;
                            samples_per_pixel = 1500;
                            max_depth = 50;
                            vfov = 40;
                            float lookfrom_t[3] = { 478, 278, -600 };
                            memcpy(lookfrom, lookfrom_t, 3 * sizeof(float));
                            float lookat_t[3] = { 278, 278, 0 };
                            memcpy(lookat, lookat_t, 3 * sizeof(float));
                            float vup_t[3] = { 0, 1, 0 };
                            memcpy(vup, vup_t, 3 * sizeof(float));
                            float background_t[3] = { 0, 0, 0 };
                            memcpy(background, background_t, 3 * sizeof(float));
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            ImGui::SeparatorText("camera");

            // 输入图片宽度
            ImGui::InputInt("image width", &image_width);
            ImGui::SameLine(); 
            HelpMarker("100~4096");
            if (image_width < 100)
                image_width = 100;
            if (image_width > 4096)
                image_width = 4096;

            // 选择图片宽高比
            const char* combo_preview_value_ar = aspect_ratios[aspect_ratio_idx];
            if (ImGui::BeginCombo("aspect ratio", combo_preview_value_ar, flags))
            {
                for (int n = 0; n < IM_ARRAYSIZE(aspect_ratios); n++)
                {
                    const bool is_selected = (aspect_ratio_idx == n);
                    if (ImGui::Selectable(aspect_ratios[n], is_selected))
                        aspect_ratio_idx = n;

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            switch (aspect_ratio_idx)
            {
            case 0: aspect_ratio = 16. / 9.; break;
            case 1: aspect_ratio = 2. / 1.; break;
            case 2: aspect_ratio = 3. / 2.; break;
            case 3: aspect_ratio = 4. / 3.; break;
            case 4: aspect_ratio = 1.; break;
            }

            // 设置背景颜色
            ImGui::ColorEdit3("background color", background);
            ImGui::SameLine();
            HelpMarker(
                "Click on the color square to open a color picker.\n"
                "Click and hold to use drag and drop.\n"
                "Right-click on the color square to show options.\n"
                "CTRL+click on individual component to input value.\n");

            // 输入spp
            ImGui::InputInt("samples per pixel", &samples_per_pixel);
            ImGui::SameLine();
            HelpMarker("10~10000");
            if (samples_per_pixel < 10)
                samples_per_pixel = 10;
            if (samples_per_pixel > 10000)
                samples_per_pixel = 10000;

            // 输入最大深度
            ImGui::InputInt("max depth", &max_depth);
            ImGui::SameLine();
            HelpMarker("10~400");
            if (max_depth < 10)
                max_depth = 10;
            if (max_depth > 400)
                max_depth = 400;

            // 设置相机外参
            ImGui::InputFloat3("lookfrom", lookfrom, "%.1f");
            ImGui::InputFloat3("lookat", lookat, "%.1f");
            ImGui::InputFloat3("vup", vup, "%.1f");

            // 设置vfov
            ImGui::DragInt("vfov", &vfov, 1, 1, 179, "%d°", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SameLine();
            HelpMarker(
                "Drag to edit value.\n"
                "Hold SHIFT/ALT for faster/slower edit.\n"
                "Double-click or CTRL+click to input value.");

            ImGui::SeparatorText("command");

            // 开始渲染
            if (ImGui::Button("start") && !rendering.load())
            {
                if (scene_current_idx == 0)
                {
                    rendering_info = "No scene.";
                }
                else
                {
                    hit_count = 0;
                    sample_count = 0;
                    rendering_info = "Rendering...";

                    rendering_start = steady_clock::now();

                    cam.set_image_name(std::string(scenes[scene_current_idx]) + ".png");
                    cam.set_image_width(image_width);
                    cam.set_aspect_ratio(aspect_ratio);
                    cam.set_samples_per_pixel(samples_per_pixel);
                    cam.set_max_depth(max_depth);
                    cam.set_vfov(vfov);
                    cam.set_lookfrom(Point3(lookfrom));
                    cam.set_lookat(Point3(lookat));
                    cam.set_vup(Vec3(vup));
                    cam.set_background(Color(background));

                    image_data = cam.initialize();

                    switch (scene_current_idx)
                    {
                    case 1: t = std::thread(scene_checker,     std::ref(cam)); break;
                    case 2: t = std::thread(scene_cornell_box, std::ref(cam)); break;
                    case 3: t = std::thread(scene_composite1,  std::ref(cam)); break;
                    case 4: t = std::thread(scene_composite2,  std::ref(cam)); break;                   
                    }

                    if (t.joinable())
                        t.detach();
                }
            }

            ImGui::SameLine();
            // 结束渲染
            if (ImGui::Button("end") && rendering.load())
            {
                rendering.store(false);
            }      

            ImGui::SameLine();
            if (ImGui::Button("save"))
            {               
                if (image_data == nullptr)
                {
                    rendering_info = "No Image.";
                }
                else if (rendering.load())
                {
                    rendering_info = "Still rendering!";
                }                   
                else
                {                 
                    rendering_info = "Image has be saved to ./output/ folder.";
                    cam.save_image();
                }
            }   

            ImGui::Text(rendering_info.c_str());

            ImGui::End();
        }

        // 状态
        {
            ImGui::Begin("STATUS", nullptr, custom_window_flag);

            ImGui::Text("UI fps = %.2f", io.Framerate);
            ImGui::Text("UI 1/fps = %.2f ms", 1000. / io.Framerate);
            ImGui::Text("sys memory load = %lld", get_memory_load());

            // 每300ms统计一次cpu占用率
            auto cpu_now = steady_clock::now();
            if (duration_cast<milliseconds>(cpu_now - cpu_start).count() > 300)
            {
                cpu_usage = get_cpu_usage(cpu_idle_prev, cpu_kernel_prev, cpu_user_prev);
                cpu_start = steady_clock::now();
            }

            ImGui::Text("sys cpu usage = %.2f", cpu_usage);
            ImGui::End();
        }

        // 显示渲染过程
        {
            ImGui::Begin("rendering", nullptr, custom_window_flag);

            ImGui::Text("image size = %d x %d", cam.get_image_width(), cam.get_image_height());
            ImGui::Text(("hit count = " + format_num(hit_count.load())).c_str());
            ImGui::Text("average depth = %.2f", (double)hit_count.load() / (sample_count.load() + 1));
            
            if (rendering.load())
            {
                duration = steady_clock::now() - rendering_start;
            }
            auto duration_min = duration_cast<minutes>(duration);
            auto duration_sec = duration_cast<seconds>(duration);
            ImGui::Text("cal time = %d min %d sec", duration_min.count(), duration_sec.count() % 60);

            if (image_data != nullptr)
            {
                // 加载纹理
                bool ret = LoadTextureFromImageData(image_data, cam.get_image_width(), cam.get_image_height(), g_pd3dDevice, my_texture_srv_cpu_handle, &my_texture);
                IM_ASSERT(ret);
                // 传递SRV GPU句柄，而不是CPU句柄。传递内部指针值, 转换为ImTextureID。
                ImGui::Image((ImTextureID)my_texture_srv_gpu_handle.ptr, ImVec2((float)cam.get_image_width(), (float)cam.get_image_height()));
            }

            ImGui::End();
        }

        // 渲染
        ImGui::Render();

        FrameContext* frameCtx = WaitForNextFrameResources();
        UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
        frameCtx->CommandAllocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = g_mainRenderTargetResource[backBufferIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        g_pd3dCommandList->Reset(frameCtx->CommandAllocator, nullptr);
        g_pd3dCommandList->ResourceBarrier(1, &barrier);

        // 渲染IMGUI图形
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, nullptr);
        g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
        g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        g_pd3dCommandList->ResourceBarrier(1, &barrier);
        g_pd3dCommandList->Close();

        g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);

        g_pSwapChain->Present(1, 0); // 有vsync
        //g_pSwapChain->Present(0, 0); // 无vsync

        UINT64 fenceValue = g_fenceLastSignaledValue + 1;
        g_pd3dCommandQueue->Signal(g_fence, fenceValue);
        g_fenceLastSignaledValue = fenceValue;
        frameCtx->FenceValue = fenceValue;

    }

    WaitForLastSubmittedFrame();

    // 清理
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}


