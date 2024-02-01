# BitRenderer

出于学习目的开发的C++软件渲染器。

支持光栅化实时预览和光线追踪离线渲染。

## 使用说明

下载obj文件并置于`./BitRenderer/load/`文件夹下，软件会自动列出以供选择。也可直接选择预置场景。

将鼠标停留在UI界面的`(?)`标志查看帮助信息。

## 三方库

[nothings/stb](https://github.com/nothings/stb)：读写图片。

[ocornut/imgui](https://github.com/ocornut/imgui)：UI，后端使用Win32+DirectX 12。

[tinyobjloader/tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)：加载obj文件

## 参考资料

[ssloy/tinyraytracer](https://github.com/ssloy/tinyraytracer)

[ssloy/tinyrenderer](https://github.com/ssloy/tinyrenderer)

[Ray Tracing in One Weekend Series](https://raytracing.github.io/)

[Crash Course in BRDF Implementation](https://boksajak.github.io/blog/BRDF)

[Morgan McGuire's Computer Graphics Archive](https://casual-effects.com/data/)

