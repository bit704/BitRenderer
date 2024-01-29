#include "image.h"
#include "logger.h"
#include "scene.h"
#include "ui_helper.h"
#include "resource.h"
#include "status.h"

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
    int width  = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    HWND hwnd  = ::CreateWindowW(wc.lpszClassName, L"BitRenderer", WS_OVERLAPPEDWINDOW, 
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
    style.TabRounding    = 3.f;
    style.FrameRounding  = 3.f;
    style.GrabRounding   = 3.f;
    style.WindowRounding = 0.f;
    style.PopupRounding  = 3.f;

    // 设置Win32和DirectX 12后端
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(g_pd3dDevice, NUM_FRAMES_IN_FLIGHT,
        DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
        g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

    // 窗口设置项
    ImGuiComboFlags   flags = 0;
    ImGuiWindowFlags_ custom_window_flag = ImGuiWindowFlags_None;
    ImGuiInputTextFlags info_flag = ImGuiInputTextFlags_ReadOnly;
    ImVec4 clear_color = ImVec4(1.f, 1.f, 1.f, 1.f); // 窗口背景颜色
    bool use_preset = false;
    int rastering_preview_mode = 0;

    // 渲染参数
    std::vector<fs::path> objs = { "None" };
    int obj_current_idx = 0;
    const char* scenes[] = { "None", "scene_checker", "scene_cornell_box", "scene_composite1", "scene_composite2" };
    int scene_current_idx = 0;
    int image_width = 600;
    const char* aspect_ratios[] = { "1/1", "4/3", "3/2", "16/9", "2/1" };
    int aspect_ratio_idx = 0;
    double aspect_ratio = 1;
    int samples_per_pixel = 30;
    int max_depth = 10;
    int vfov = 20;
    float lookfrom[3] = { 0, 0, 1 };
    float lookat[3]   = { 0, 0, 0 };
    float vup[3]      = { 0, 1, 0 };
    float background[3] = { 1, 1, 1 };
    std::string image_name = "default.png";

    // 渲染数据
    Camera cam;
    unsigned char** image_data_p2p = nullptr; //指向图像数据指针的指针
    std::thread t;
     
    // 统计数据
    auto rendering_start = steady_clock::now(); // 记录渲染开始时间
    nanoseconds duration(0); // 记录渲染用时
    double cpu_usage = 0.;
    auto   cpu_start = steady_clock::now(); // 记录CPU占用率计算间隔
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
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always, ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(460, 800), ImGuiCond_Always);

            ImGui::Begin("SETUP", nullptr, custom_window_flag 
                | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

            ImGui::SeparatorText("scene");

            {
                ImGui::BeginDisabled(use_preset);

                // 根据kLoadPath文件夹下文件刷新objs数组
                auto objs_new = traverse_path(kLoadPath, std::regex(".*\\.obj"));
                if (objs != objs_new)
                    objs = std::move(objs_new);

                // 选择obj
                auto combo_obj_str = objs[obj_current_idx].filename().string();
                const char* combo_obj = combo_obj_str.c_str(); // 必须分两步，否则combo_obj指向已经销毁的实例
                if (ImGui::BeginCombo("load obj", combo_obj, flags))
                {
                    for (int n = 0; n < objs.size(); n++)
                    {
                        const bool is_selected = (obj_current_idx == n);
                        if (ImGui::Selectable(objs[n].filename().string().c_str(), is_selected))
                            obj_current_idx = n;

                        if (is_selected)
                            ImGui::SetItemDefaultFocus();

                        // 选择obj时将scene置为None
                        scene_current_idx = 0;

                        // obj推荐参数，None选项不指定
                        if (obj_current_idx != 0)
                        {
                            image_width = 600;
                            aspect_ratio_idx = 0;
                            samples_per_pixel = 30;
                            max_depth = 10;
                            vfov = 20;
                            float lookfrom_t[3] = { 5, 3, 5 };
                            memcpy(lookfrom, lookfrom_t, 3 * sizeof(float));
                            float lookat_t[3] = { 0, 0, 0 };
                            memcpy(lookat, lookat_t, 3 * sizeof(float));
                            float vup_t[3] = { 0, 1, 0 };
                            memcpy(vup, vup_t, 3 * sizeof(float));
                            float background_t[3] = { 1, 1, 1 };
                            memcpy(background, background_t, 3 * sizeof(float));

                            image_name = objs[obj_current_idx].stem().string() + ".png"; // 去掉末尾的".obj"再加上".png"
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                HelpMarker(
                    "Please put .obj and associated files in .\\load\\ folder.\n"
                    "Available .obj will automatically show in this Checkbox.\n");
            

                ImGui::EndDisabled();
            }

            // 是否使用预置场景
            ImGui::Checkbox("use preset", &use_preset);

            {
                ImGui::BeginDisabled(!use_preset);

                //ImGui::SetNextItemWidth(200);

                // 选择预置场景
                const char* combo_scene = scenes[scene_current_idx];
                if (ImGui::BeginCombo("preset", combo_scene, flags))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(scenes); n++)
                    {
                        const bool is_selected = (scene_current_idx == n);
                        if (ImGui::Selectable(scenes[n], is_selected))
                            scene_current_idx = n;

                        if (is_selected)
                            ImGui::SetItemDefaultFocus();

                        // 选择scene时将obj置为None
                        obj_current_idx = 0;

                        // 各场景推荐参数
                        if (scene_current_idx != 0)
                            image_name = std::string(scenes[scene_current_idx]) + ".png";

                        if (scene_current_idx == 1)
                        {
                            image_width = 600;
                            aspect_ratio_idx = 3;
                            samples_per_pixel = 30;
                            max_depth = 10;
                            vfov = 20;
                            float lookfrom_t[3] = { 10, 3, 10 };
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
                            aspect_ratio_idx = 0;
                            samples_per_pixel = 30;
                            max_depth = 10;
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
                            aspect_ratio_idx = 3;
                            samples_per_pixel = 30;
                            max_depth = 10;
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
                            aspect_ratio_idx = 0;
                            samples_per_pixel = 30;
                            max_depth = 10;
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
                ImGui::SameLine();
                HelpMarker(
                    "Choose preset scene.\n");

                ImGui::EndDisabled();
            }

            ImGui::SeparatorText("command");

            ImVec2 window_pos = ImGui::GetWindowPos();
            ImVec2 window_size = ImGui::GetWindowSize();
            ImVec2 window_center = ImVec2(window_pos.x + window_size.x * .5f, window_pos.y + window_size.y * .5f);
            
            // 选择光栅化预览模式
            ImGui::Text("Rastering Preview");
            ImGui::Text("            ");
            ImGui::SameLine(); ImGui::RadioButton("normal",    &rastering_preview_mode, 0); 
            ImGui::SameLine(); ImGui::RadioButton("wireframe", &rastering_preview_mode, 1);
            ImGui::SameLine(); ImGui::RadioButton("depth",     &rastering_preview_mode, 2);

            ImGui::Text("Ray Tracing");
            ImGui::Text("                    "); 

            // 开始渲染
            ImGui::BeginDisabled(rendering.load());
            ImGui::SameLine();
            if (ImGui::Button("Start") && !rendering.load())
            {
                auto assemble = [&]()
                    {
                        // 设置统计数据
                        rendering.store(true);
                        hit_count.store(0);
                        sample_count.store(0);
                        add_info("----------------\nRendering...");
                        rendering_start = steady_clock::now();

                        // 设置参数
                        cam.set_image_width(image_width);
                        cam.set_aspect_ratio(aspect_ratio);
                        cam.set_samples_per_pixel(samples_per_pixel);
                        cam.set_max_depth(max_depth);
                        cam.set_vfov(vfov);
                        cam.set_lookfrom(Point3(lookfrom));
                        cam.set_lookat(Point3(lookat));
                        cam.set_vup(Vec3(vup));
                        cam.set_background(Color(background));
                        cam.set_image_name(image_name);

                        image_data_p2p = cam.initialize();
                    };

                if (!use_preset)
                {
                    if (obj_current_idx == 0)
                    {
                        add_info("No .obj sccene");
                    }
                    else
                    {
                        assemble();

                        t = std::thread(scene_obj, std::cref(cam), std::cref(objs[obj_current_idx]));
                        //t = std::thread(scene_test_triangle, std::cref(cam));

                        if (t.joinable())
                            t.detach();
                    }
                }
                else
                {
                    if (scene_current_idx == 0)
                    {
                        add_info("No preset scene.");
                    }
                    else
                    {
                        assemble();

                        switch (scene_current_idx)
                        {
                        case 1: t = std::thread(scene_checker, std::cref(cam)); break;
                        case 2: t = std::thread(scene_cornell_box, std::cref(cam)); break;
                        case 3: t = std::thread(scene_composite1, std::cref(cam)); break;
                        case 4: t = std::thread(scene_composite2, std::cref(cam)); break;
                        }

                        if (t.joinable())
                            t.detach();
                    }
                }
            }
            ImGui::EndDisabled();
        
            ImGui::BeginDisabled(!rendering.load());
            ImGui::SameLine();
            // 结束渲染
            if (ImGui::Button("Abort") && rendering.load())
            {
                add_info("Aborting...");
                rendering.store(false);
            }
            ImGui::EndDisabled();

            ImGui::BeginDisabled(image_data_p2p == nullptr || *image_data_p2p == nullptr || rendering.load());
            ImGui::SameLine();
            if (ImGui::Button("Clear") && image_data_p2p != nullptr && *image_data_p2p != nullptr && !rendering.load())
            {
                free(*image_data_p2p);
                *image_data_p2p = nullptr;
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Save"))
            {
                if (image_data_p2p == nullptr || *image_data_p2p == nullptr)
                {
                    add_info("No Image.");
                }
                else if (rendering.load())
                {
                    add_info("Still rendering!");
                }
                else
                {
                    add_info("Saving...");
                    cam.save_image();
                    add_info("Image has be saved to ./output/ folder.");
                }
            }

            ImGui::SeparatorText("camera");

            // 输入图片宽度
            ImGui::InputInt("image width", &image_width);
            ImGui::SameLine(); 
            HelpMarker("1~4096");
            if (image_width < 1)
                image_width = 1;
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
            case 0: aspect_ratio = 1      ; break;
            case 1: aspect_ratio = 4. / 3.; break;
            case 2: aspect_ratio = 3. / 2.; break;
            case 3: aspect_ratio = 16./ 9.; break;
            case 4: aspect_ratio = 2      ; break;
            }

            // 设置vfov
            ImGui::DragInt("vfov", &vfov, 1, 1, 179, "%d°", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SameLine();
            HelpMarker(
                "Drag to edit value.\n"
                "Hold SHIFT/ALT for faster/slower edit.\n"
                "Double-click or CTRL+click to input value.");

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
            HelpMarker("1~10000");
            if (samples_per_pixel < 1)
                samples_per_pixel = 1;
            if (samples_per_pixel > 10000)
                samples_per_pixel = 10000;

            // 输入最大深度
            ImGui::InputInt("max depth", &max_depth);
            ImGui::SameLine();
            HelpMarker("1~400");
            if (max_depth < 1)
                max_depth = 1;
            if (max_depth > 400)
                max_depth = 400;

            // 设置相机外参
            ImGui::InputFloat3("lookfrom", lookfrom, "%.1f");
            ImGui::InputFloat3("lookat", lookat, "%.1f");
            ImGui::InputFloat3("vup", vup, "%.1f");

            ImGui::SeparatorText("info");

            std::string info = return_info();
            std::copy(info.begin(), info.end(), text);
            ImGui::InputTextMultiline("info", text, IM_ARRAYSIZE(text), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 12), info_flag);
            
            HelpMarker(
                "The latest 256 information.\n"
                "Each shorter than 128.\n");

            ImGui::End();
        }

        // 渲染
        {
            ImGui::Begin("RENDER", nullptr, custom_window_flag 
                | ImGuiWindowFlags_NoCollapse);

            ImGui::Text("image size = %d x %d", cam.get_image_width(), cam.get_image_height());
            ImGui::Text(("hit count = " + format_num(hit_count.load())).c_str());
            ImGui::Text("average depth = %.2f", (double)hit_count.load() / (sample_count.load() + 1));
            
            if (rendering.load())
            {
                duration = steady_clock::now() - rendering_start;
            }
            auto duration_min = duration_cast<minutes>(duration);
            auto duration_sec = duration_cast<seconds>(duration);
            ImGui::Text("total elapsed time = %d min %d sec", duration_min.count(), duration_sec.count() % 60);

            if (image_data_p2p != nullptr && *image_data_p2p != nullptr)
            {
                // 加载纹理
                bool ret = LoadTextureFromImageData(*image_data_p2p, cam.get_image_width(), cam.get_image_height(), g_pd3dDevice, my_texture_srv_cpu_handle, &my_texture);
                IM_ASSERT(ret);
                // 传递SRV GPU句柄，而不是CPU句柄。传递内部指针值, 转换为ImTextureID。
                ImGui::Image((ImTextureID)my_texture_srv_gpu_handle.ptr, ImVec2((float)cam.get_image_width(), (float)cam.get_image_height()));
            }

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
    if(my_texture)
        my_texture->Release();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}


