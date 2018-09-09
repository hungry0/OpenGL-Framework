# OpenGL-Framework
learnopengl.com的学习过程

1. 开发环境是win10+vs2017

2. 环境配置

将Externals中的一些文件夹配置到vs环境中，如下。

xxx\Externals\Glad\Include 添加到 项目->xx属性->配置属性->VC++目录->包含目录
xxx\Externals\Glfw\Include 添加到 项目->xx属性->配置属性->VC++目录->包含目录
xxx\Externals\Glfw\Libs 添加到 项目->xx属性->配置属性->VC++目录->库目录
xxx\Externals\Glfw\Libs 添加到 项目->xx属性->配置属性->VC++目录->库目录
xxx\Externals\OpenGL\Libs 添加到 项目->xx属性->配置属性->VC++目录->库目录

将opengl32.lib、glfw3.lib添加到 项目->xx属性->配置属性->链接器->输入->附加依赖项
