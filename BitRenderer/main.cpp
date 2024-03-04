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

    // 获取桌面可用区域大小（除开任务栏），以最大化窗口
    RECT max_window_rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &max_window_rect, 0);
    int max_window_width  = max_window_rect.right - max_window_rect.left;
    int max_window_height = max_window_rect.bottom - max_window_rect.top;
    HWND hwnd  = ::CreateWindowW(wc.lpszClassName, L"BitRenderer", WS_OVERLAPPEDWINDOW, 
        max_window_rect.left, max_window_rect.top, max_window_width, max_window_height, 
        nullptr, nullptr, wc.hInstance, nullptr);

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
    ImGuiContext* igc = ImGui::CreateContext();

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
    double proportion = .32; // SETUP界面和RENDER界面比例

    // 渲染参数
    bool use_preset = false; // 是否使用预置场景
    bool tracing_with_cornell_box = false; // 渲染obj时是否加上cornell box   
    int rastering_mode = RasteringModeFlags_None;
    int rastering_major_mode = RasteringModeFlags_Wireframe;
    bool use_coordinate_system = false;

    std::vector<fs::path> objs = { "None" };
    int obj_pre_idx = 0;
    int obj_current_idx = 0;
    std::vector<fs::path> maps = { "None" };
    int diffuse_map_pre_idx = 0;
    int diffuse_map_current_idx = 0;
    const char* scenes[] = { "None", "scene_checker", "scene_cornell_box", "scene_composite1", "scene_composite2" };
    int scene_current_idx = 0;
    int image_width = 600;
    const char* aspect_ratios[] = { "1/1", "4/3", "3/2", "16/9", "2/1" };
    int aspect_ratio_pre_idx = 0;
    int aspect_ratio_current_idx = 0;
    double aspect_ratio = 1;
    int samples_per_pixel = 16;
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
    bool refresh_rasterizing = true; // 是否需要刷新光栅化结果，第一帧默认为true，每帧判定结束后置为false，当任意影响光栅化的参数被编辑时置为true
     
    // 统计数据    
    auto tracing_start = steady_clock::now(); // 记录渲染开始时间
    nanoseconds duration(0); // 记录渲染用时
    double cpu_usage = 0;
    auto   cpu_start = steady_clock::now(); // 记录CPU占用率计算间隔
    FILETIME cpu_idle_prev, cpu_kernel_prev, cpu_user_prev; // 记录CPU时间
    GetSystemTimes(&cpu_idle_prev, &cpu_kernel_prev, &cpu_user_prev);

    // 键鼠交互
    auto interaction_point = steady_clock::now(); // 上次处理键鼠交互时间点
    bool enter_interaction = false; // 点击图像进行交互，按ESC退出交互

    // 组装渲染配置
    auto assemble = [&]()
        {
            // 仅在图片尺寸发生变化时需要重新生成图片
            bool new_image = (image_width != cam.get_image_width()) ||
                (aspect_ratio != cam.get_aspect_ratio());

            // 设置参数
            cam.set_image_width(image_width);
            cam.set_aspect_ratio(aspect_ratio);
            cam.set_samples_per_pixel(samples_per_pixel);
            cam.set_max_depth(max_depth);
            cam.set_vfov(vfov);
            cam.set_lookfrom(Point3(lookfrom));
            cam.set_lookat(Point3(lookat));
            cam.set_vup(Vec3(vup));
            cam.set_background(Color3(background));
            cam.set_image_name(image_name);       

            image_data_p2p = cam.initialize(new_image);
        };

    // 设置光追统计数据
    auto tracing_statistics = [&]()
        {
            assemble();
            tracing.store(true);
            hit_count.store(0);
            sample_count.store(0);
            tracing_start = steady_clock::now();
        };

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

        // 获取当前窗口大小（除开标题栏、菜单栏）
        RECT now_window_rect;
        GetClientRect(hwnd, &now_window_rect);
        int now_window_width = now_window_rect.right - now_window_rect.left;
        int now_window_height = now_window_rect.bottom - now_window_rect.top;

        // 设置界面
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always, ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(now_window_width * proportion, now_window_height), ImGuiCond_Always);
        
            ImGui::Begin("SETUP", nullptr, custom_window_flag
                | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);

            if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BeginDisabled(use_preset);

                // 根据kLoadPath文件夹下文件刷新objs数组
                auto objs_new = traverse_path(kLoadPath, std::regex(".*\\.obj$"));
                if (objs != objs_new)
                    objs = std::move(objs_new);

                // 选择obj
                auto combo_obj_str = objs[obj_current_idx].stem().string();
                const char* combo_obj = combo_obj_str.c_str(); // 必须分两步，否则combo_obj指向已经销毁的实例
                if (ImGui::BeginCombo("load obj", combo_obj, flags))
                {
                    for (int n = 0; n < objs.size(); n++)
                    {
                        const bool is_selected = (obj_current_idx == n);
                        if (ImGui::Selectable(objs[n].string().c_str(), is_selected))
                        {
                            obj_pre_idx = obj_current_idx;
                            obj_current_idx = n;
                            if (obj_pre_idx != obj_current_idx)
                            {
                                // 选择新obj时无需再停止光栅化
                                stop_rastering.store(false);
                                refresh_rasterizing = true;
                            }
                        }

                        if (is_selected)
                            ImGui::SetItemDefaultFocus();

                        // obj推荐参数，None选项不指定
                        if (obj_current_idx != 0)
                        {
                            image_width = 600;
                            aspect_ratio_current_idx = 0;
                            samples_per_pixel = 5;
                            max_depth = 5;
                            vfov = 20;
                            float lookfrom_t[3] = { 0, -1, -12 };
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
                    "Please put .obj in .\\load\\ folder.\n"
                    "Available .obj will automatically show in this Checkbox.\n");

                // 根据kLoadPath文件夹下文件刷新maps数组
                auto maps_new = traverse_path(kLoadPath, std::regex(".*\\.(jpg|png|tga|bmp|psd|gif|hdr|pic)$", std::regex_constants::icase));
                if (maps != maps_new)
                    maps = std::move(maps_new);

                // 选择diffuse map
                auto diffuse_map_str = maps[diffuse_map_current_idx].filename().string();
                const char* combo_diffuse_map = diffuse_map_str.c_str();
                if (ImGui::BeginCombo("load diffuse map", combo_diffuse_map, flags))
                {
                    for (int n = 0; n < maps.size(); n++)
                    {
                        const bool is_selected = (diffuse_map_current_idx == n);
                        if (ImGui::Selectable(maps[n].string().c_str(), is_selected))
                        {
                            diffuse_map_pre_idx = diffuse_map_current_idx;
                            diffuse_map_current_idx = n;
                            if (diffuse_map_pre_idx != diffuse_map_current_idx)
                            {
                                // 选择新map时无需再停止光栅化
                                stop_rastering.store(false);
                                refresh_rasterizing = true;
                            }
                        }

                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                HelpMarker(
                    "Please put map in .\\load\\ folder.(JPG, PNG, TGA, BMP, PSD, GIF, HDR, PIC)\n"
                    "Available map will automatically show in this Checkbox.\n"
                    "All red if fail to load.\n");

                ImGui::Checkbox("tracing with cornell box", &tracing_with_cornell_box);
                ImGui::SameLine();
                HelpMarker("Whether add cornell box to surround obj when tracing.");

                ImGui::EndDisabled(); // ImGui::BeginDisabled(use_preset);

                // 是否使用预置场景
                ImGui::Checkbox("use preset", &use_preset);

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
                        {
                            // 选择scene时无需再停止光栅化
                            stop_rastering.store(false);
                            // 此时刷新光栅化只用于显示仅带背景颜色的图片
                            refresh_rasterizing = true;
                            scene_current_idx = n;
                        }

                        if (is_selected)
                            ImGui::SetItemDefaultFocus();

                        // 选择scene时自动将obj置为None
                        obj_current_idx = 0;

                        // 各场景推荐参数
                        if (scene_current_idx != 0)
                        {
                            image_name = std::string(scenes[scene_current_idx]) + ".png";
                        }

                        if (scene_current_idx == 1)
                        {
                            image_width = 600;
                            aspect_ratio_current_idx = 3;
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
                            aspect_ratio_current_idx = 0;
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
                            aspect_ratio_current_idx = 3;
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
                            aspect_ratio_current_idx = 0;
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
                HelpMarker("Choose preset scene.\n");

                ImGui::EndDisabled(); //  ImGui::BeginDisabled(!use_preset);
            }

            if (ImGui::CollapsingHeader("command", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImVec2 window_pos = ImGui::GetWindowPos();
                ImVec2 window_size = ImGui::GetWindowSize();
                ImVec2 window_center = ImVec2(window_pos.x + window_size.x * .5f, window_pos.y + window_size.y * .5f);

                // 选择光栅化模式
                ImGui::Text("Rasterizing Roam");

                refresh_rasterizing |= ImGui::RadioButton("wireframe", &rastering_major_mode, RasteringModeFlags_Wireframe); ImGui::SameLine();
                refresh_rasterizing |= ImGui::RadioButton("depth", &rastering_major_mode, RasteringModeFlags_Depth); ImGui::SameLine();
                refresh_rasterizing |= ImGui::RadioButton("shade", &rastering_major_mode, RasteringModeFlags_Shade); ImGui::SameLine();
                rastering_mode = rastering_major_mode;

                refresh_rasterizing |= ImGui::Checkbox("coordinate system", &use_coordinate_system);
                if (use_coordinate_system)
                    rastering_mode |= RasteringModeFlags_Coordinate_System;
                else
                    rastering_mode &= ~RasteringModeFlags_Coordinate_System;

                // 光线追踪选项
                ImGui::Text("Ray Tracing");

                // 开始光线追踪
                ImGui::BeginDisabled(tracing.load());                
                if (ImGui::Button("Start") && !tracing.load())
                {
                    if (!use_preset)
                    {
                        if (obj_current_idx == 0)
                        {
                            add_info("No .obj sccene");
                        }
                        else
                        {
                            add_info("----------------");
                            add_info("Rendering " + objs[obj_current_idx].stem().string() + "...");

                            tracing_statistics();

                            t = std::thread(scene_obj_trace, std::cref(cam), std::cref(objs[obj_current_idx]), std::cref(maps[diffuse_map_current_idx]), std::cref(tracing_with_cornell_box));
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
                            add_info("----------------");
                            add_info("Rendering "_str + scenes[scene_current_idx] + "...");

                            tracing_statistics();

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

                ImGui::BeginDisabled(!tracing.load());
                ImGui::SameLine();
                // 结束光追
                if (ImGui::Button("Abort") && tracing.load())
                {
                    add_info("Aborting...");
                    tracing.store(false);
                    stop_rastering.store(true);
                }
                ImGui::EndDisabled();

                ImGui::BeginDisabled(image_data_p2p == nullptr || *image_data_p2p == nullptr || tracing.load());
                ImGui::SameLine();
                if (ImGui::Button("Clear") && image_data_p2p != nullptr && *image_data_p2p != nullptr && !tracing.load())
                {
                    cam.clear();
                    stop_rastering.store(false);
                    refresh_rasterizing = true;
                }
                ImGui::EndDisabled();

                ImGui::SameLine();
                if (ImGui::Button("Save"))
                {
                    if (image_data_p2p == nullptr || *image_data_p2p == nullptr)
                    {
                        add_info("No Image.");
                    }
                    else if (tracing.load())
                    {
                        add_info("Still tracing!");
                    }
                    else
                    {
                        add_info("Saving...");
                        cam.save_image();
                        add_info(image_name + " has been saved to ./output/ folder.");
                    }
                }

                ImGui::SameLine();
                HelpMarker("Clear will discard Ray Tracing result and restart Rasterizing.\n");
            }

            if (ImGui::CollapsingHeader("camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // 输入图片宽度
                refresh_rasterizing |= ImGui::InputInt("image width", &image_width);
                ImGui::SameLine();
                HelpMarker("256~1024\n");
                if (image_width < 256)
                    image_width = 256;
                if (image_width > 1024)
                    image_width = 1024;

                // 选择图片宽高比
                const char* combo_aspect_ratio = aspect_ratios[aspect_ratio_current_idx];
                if (ImGui::BeginCombo("aspect ratio", combo_aspect_ratio, flags))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(aspect_ratios); n++)
                    {
                        const bool is_selected = (aspect_ratio_current_idx == n);
                        if (ImGui::Selectable(aspect_ratios[n], is_selected))
                        {
                            aspect_ratio_pre_idx = aspect_ratio_current_idx;
                            aspect_ratio_current_idx = n;
                            if (aspect_ratio_current_idx != aspect_ratio_pre_idx)
                            {
                                refresh_rasterizing = true;
                            }
                        }

                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                switch (aspect_ratio_current_idx)
                {
                case 0: aspect_ratio = 1; break;
                case 1: aspect_ratio = 4. / 3.; break;
                case 2: aspect_ratio = 3. / 2.; break;
                case 3: aspect_ratio = 16. / 9.; break;
                case 4: aspect_ratio = 2; break;
                }

                // 设置vfov
                refresh_rasterizing |= ImGui::DragInt("vfov", &vfov, 1, 1, 179, "%d°", ImGuiSliderFlags_AlwaysClamp);
                ImGui::SameLine();
                HelpMarker(
                    "Drag to edit value.\n"
                    "Hold SHIFT/ALT for faster/slower edit.\n"
                    "Double-click or CTRL+click to input value.\n");

                // 设置背景颜色
                refresh_rasterizing |= ImGui::ColorEdit3("background color", background);
                ImGui::SameLine();
                HelpMarker(
                    "Just background for feature \"Rasterizing Roam - wireframe\"\n"
                    "No effect on feature \"Rasterizing Roam - depth\".\n"
                    "Both background and ambient for feature \"Rasterizing Roam - shade\".\n"
                    "Background color when ray miss scene for \"Ray Tracing\".\n"
                    "\n"
                    "Click on the color square to open a color picker.\n"
                    "Click and hold to use drag and drop.\n"
                    "Right-click on the color square to show options.\n"
                    "CTRL+click on individual component to input value.\n");

                // 输入spp
                ImGui::InputInt("samples per pixel", &samples_per_pixel);
                ImGui::SameLine();
                HelpMarker("1~10000\n");
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
                refresh_rasterizing |= ImGui::InputFloat3("lookfrom", lookfrom, "%.1f");
                refresh_rasterizing |= ImGui::InputFloat3("lookat", lookat, "%.1f");
                refresh_rasterizing |= ImGui::InputFloat3("vup", vup, "%.1f");
                ImGui::SameLine();
                HelpMarker("Up direction of the camera.\n");
            }

            if (ImGui::CollapsingHeader("info", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BeginChild("info", ImVec2(0, 300),
                    ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_HorizontalScrollbar);

                for (ulong i = 0; i < get_info_size(); ++i)
                    ImGui::Text(get_info(i));

                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                    ImGui::SetScrollHereY(1.0f);
                ImGui::EndChild();
            }

            ImGui::End();
        }
           
        // 键鼠交互
        {
            // 距上次处理键鼠交互时长
            double interaction_delta = duration_cast<milliseconds>(steady_clock::now() - interaction_point).count() / 1e3;
            // 键鼠交互（点击图像进行交互，按ESC退出交互）
            // 光追时禁止
            if (enter_interaction && !tracing.load())
            {
                // 前后移动 WS
                if (ImGui::IsKeyDown(ImGuiKey_W))
                {
                    refresh_rasterizing = true;
                    cam.move_front_back(interaction_delta);
                }
                if (ImGui::IsKeyDown(ImGuiKey_S))
                {
                    refresh_rasterizing = true;
                    cam.move_front_back(-interaction_delta);
                }

                // 左右移动 AD
                if (ImGui::IsKeyDown(ImGuiKey_A))
                {
                    refresh_rasterizing = true;
                    cam.move_left_right(interaction_delta);
                }
                if (ImGui::IsKeyDown(ImGuiKey_D))
                {
                    refresh_rasterizing = true;
                    cam.move_left_right(-interaction_delta);
                }

                // 上下移动 QE
                if (ImGui::IsKeyDown(ImGuiKey_Q))
                {
                    refresh_rasterizing = true;
                    cam.move_up_down(interaction_delta);
                }
                if (ImGui::IsKeyDown(ImGuiKey_E))
                {
                    refresh_rasterizing = true;
                    cam.move_up_down(-interaction_delta);
                }

                // 鼠标中键 缩放fov                  
                if (double fov_scale = ImGui::GetIO().MouseWheel; fov_scale != 0)
                {
                    refresh_rasterizing = true;
                    cam.zoom_fov(fov_scale);
                }

                // 鼠标左键 围绕观察点转动视角（第三人称）
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    refresh_rasterizing = true;
                    cam.view_third_person(io.MouseDelta.x, io.MouseDelta.y);
                }

                // 鼠标右键 自身转动视角（第一人称）
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
                {
                    refresh_rasterizing = true;
                    cam.view_first_person(io.MouseDelta.x, io.MouseDelta.y);
                }

                if (ImGui::IsKeyDown(ImGuiKey_Escape))
                    enter_interaction = false;

                // 更新相机参数的改变到UI上
                vfov = cam.get_vfov();
                float lookfrom_t[3] = { cam.get_lookfrom().x(), cam.get_lookfrom().y(), cam.get_lookfrom().z() };
                memcpy(lookfrom, lookfrom_t, 3 * sizeof(float));
                float lookat_t[3] = { cam.get_lookat().x(), cam.get_lookat().y(), cam.get_lookat().z() };
                memcpy(lookat, lookat_t, 3 * sizeof(float));
                float vup_t[3] = { cam.get_vup().x(), cam.get_vup().y(), cam.get_vup().z() };
                memcpy(vup, vup_t, 3 * sizeof(float));
            }
            interaction_point = steady_clock::now();
        }

        // 渲染界面
        {
            assemble();

            // 只有当相机外参改变时刷新光栅化结果
            if (refresh_rasterizing)
            {
                // 只有在同时满足以下情况时才进行光栅化
                // 未开始光线追踪
                // 没有停止光栅化（完成一个obj的光线追踪离线渲染且没有clear时停止光栅化）
                // 选择的是不为None的obj
                if (!tracing.load() && !stop_rastering.load())
                {
                    cam.clear();
                    if (!use_preset && obj_current_idx != 0)
                    {
                        scene_obj_rasterize(cam, objs[obj_current_idx], maps[diffuse_map_current_idx], rastering_mode);
                    }
                }
                refresh_rasterizing = false; // 置为false，下一帧重新判定
            }

            ImGui::SetNextWindowPos(ImVec2(now_window_width * proportion, 0), ImGuiCond_Always, ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(now_window_width * (1 - proportion), now_window_height), ImGuiCond_Always);

            ImGui::Begin("RENDER", nullptr, custom_window_flag
                | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);

            ImGui::Text("image size = %d x %d", cam.get_image_width(), cam.get_image_height());
            ImGui::Text(("hit count = " + format_num(hit_count.load())).c_str());
            ImGui::Text("average depth = %.2f", (double)hit_count.load() / (sample_count.load() + 1));

            if (tracing.load())
            {
                duration = steady_clock::now() - tracing_start;
            }
            auto duration_min = duration_cast<minutes>(duration);
            auto duration_sec = duration_cast<seconds>(duration);
            ImGui::Text("total elapsed time = %d min %d sec", duration_min.count(), duration_sec.count() % 60);

            // 加载纹理以在UI实时显示渲染结果
            if (image_data_p2p != nullptr && *image_data_p2p != nullptr)
            {
                bool ret = LoadTextureFromImageData(*image_data_p2p, cam.get_image_width(), cam.get_image_height(), g_pd3dDevice, my_texture_srv_cpu_handle, &my_texture);
                IM_ASSERT(ret);
                ImVec4 border_col = ImVec4(0, 0, 0, 0);
                // 处于交互状态时显示高亮边框
                if (enter_interaction)
                    border_col = ImVec4(.667, .8, .957, 1);
                // 传递SRV GPU句柄，而不是CPU句柄。传递内部指针值, 转换为ImTextureID。
                HelpMarker("Click image to interact and press ESC to exit interaction.\n");
                ImGui::Image(
                    (ImTextureID)my_texture_srv_gpu_handle.ptr,
                    ImVec2((float)cam.get_image_width(), (float)cam.get_image_height()),
                    ImVec2(0, 0),
                    ImVec2(1, 1),
                    ImVec4(1, 1, 1, 1),
                    border_col
                );
                // 图片组件是否被点击
                if (ImGui::IsItemClicked())
                    enter_interaction = true;
            }

            ImGui::End();
        }

        // 状态界面
        {
            ImGui::Begin("STATUS", nullptr, custom_window_flag);

            ImGui::Text("UI fps = %.2f", io.Framerate);
            ImGui::Text("UI 1/fps = %.2f ms", 1000. / io.Framerate);
            ImGui::Text("sys memory load = %lld%%", get_memory_load());

            // 每300ms统计一次cpu占用率
            auto cpu_now = steady_clock::now();
            if (duration_cast<milliseconds>(cpu_now - cpu_start).count() > 300)
            {
                cpu_usage = get_cpu_usage(cpu_idle_prev, cpu_kernel_prev, cpu_user_prev);
                cpu_start = steady_clock::now();
            }

            ImGui::Text("sys cpu usage = %.2f%%", cpu_usage);
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


