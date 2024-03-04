# BitRenderer

软件渲染器，基于C++17。

支持**光栅化**实时漫游和**光线追踪**离线渲染。

选择obj模型和贴图，在光栅化模式下进行漫游，有线框、深度、着色三种模式可选。

漫游确定合适相机位置后，可开启光线追踪离线渲染当前场景，渲染过程实时显示。光追采用蒙特卡洛路径追踪及BVH加速。

## 使用说明

UI按3:7纵向划分为**SETUP**界面和**RENDER**界面（自适应窗口大小变化），另有悬浮的**STATUS**界面（可移动和折叠）。

> 将鼠标悬停在UI界面的各`(?)`标志处可查看对应帮助信息。

- SETUP界面

   - scene区域

     load obj下拉框：下载obj文件并置于`./BitRenderer/load/`文件夹（或其任意子文件夹）下，软件会在此处自动列出以供选择。

     load diffuse map下拉框：下载贴图（JPG, PNG, TGA, BMP, PSD, GIF, HDR, PIC）并置于`./BitRenderer/load/`文件夹（或其任意子文件夹）下，软件会在此处自动列出以供选择。

     tracing with cornell box复选框：勾选后光追时使用康奈尔盒包围obj。

     use preset复选框：勾选后使load obj下拉框及相关设置无效，使preset下拉框生效。取消勾选则效果反之。

     preset下拉框：选择预置场景。（预置场景的几何表示为数学形式，无光栅化）

   - command区域

     Rasterizing Roam按钮：可在**wireframe**、**depth**、**shade**三个光栅化模式中选择其一，并选择是否显示坐标系。

     Ray Tracing按钮组：Start按钮开始光追，Abort按钮中止光追，Clear按钮清除光追结果并重新开始光栅化，Save按钮保存当前渲染结果（光栅化或光追）至`./BitRenderer/output/`文件夹。

   - camera区域

     image width编辑框：编辑图像宽度。

     aspect ratio下拉框：选择宽高比，有1/1、4/3、3/2、16/9、2/1可选。

     vfov编辑框：编辑垂直视场角。可拖动以快速编辑。

     background color颜色编辑框：编辑背景颜色。可点击RGB后的颜色方块打开快速编辑界面。

     samples per pixel编辑框：编辑光追的每像素采样数。

     max depth编辑框：编辑光追的最大弹射深度。

     lookfrom、lookat、vup编辑框：显示及编辑相机的原点、注视点、向上方向。

   - info区域

     打印程序运行信息。

- RENDER界面

   实时显示当前渲染结果（光追过程中和光追结果未清除时显示光追图像，其它情况下显示光栅化图像）。

   **点击图像进行交互，按ESC退出交互**（光追时无法交互）：

   - WS/AD/QE：前后/左右/上下移动相机

   - 鼠标中键：缩放fov

   -  鼠标左键：围绕当前观察点（随相机移动变化）转动视角（第三人称）

   -  鼠标右键：自身转动视角（第一人称）

   实时显示渲染信息于渲染结果上方：

   - image size：显示当前图像尺寸。

   - hit count：显示光追总击中（弹射）次数。

   - average depth：显示光追平均击中（弹射）次数。

   - total elapsed time：显示光追总耗时（加载obj用时+构建BVH用时+渲染用时）。

- STATUS界面

   实时显示状态信息：

   - UI fps：当前UI帧率。（UI后端为Win32+DirectX 12）

   - UI 1/fps：当前UI帧率的倒数，单位为毫秒。

   - sys memory load：系统内存负载，单位为百分比。

   - sys cpu usage：系统cpu占用率（每300毫秒统计一次），单位为百分比。

## 三方库

[nothings/stb](https://github.com/nothings/stb)：图片读写。

[ocornut/imgui](https://github.com/ocornut/imgui)：UI，后端使用Win32+DirectX 12。

[tinyobjloader/tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)：obj加载。

## 参考资料

[ssloy/tinyraytracer](https://github.com/ssloy/tinyraytracer)

[ssloy/tinyrenderer](https://github.com/ssloy/tinyrenderer)

[GAMES101](https://sites.cs.ucsb.edu/~lingqi/teaching/games101.html)

[Ray Tracing in One Weekend Series](https://raytracing.github.io/)

[Crash Course in BRDF Implementation](https://boksajak.github.io/blog/BRDF)

[Morgan McGuire's Computer Graphics Archive](https://casual-effects.com/data/)

## 运行截图

![光追结果](./screenshot/20240304-1.jpg)

![线框模式](./screenshot/20240304-2.jpg)

![深度模式](./screenshot/20240304-3.jpg)

![着色模式](./screenshot/20240304-4.jpg)
