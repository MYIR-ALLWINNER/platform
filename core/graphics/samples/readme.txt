1.
sample code用例：<texture_dma.c>
作用：
（1）演示dma fd纹理导入的方法；
（2）演示window surface的使用方法；
（3）演示pbuffer surface的使用方法；
（4）演示FBO的使用方法；

使用方法
(1)ON_SCREEN宏
打开ON_SCREEN宏，采用的是window surface的渲染方式；
关闭ON_SCREEN宏，采用的是pbuffer + FBO的渲染方式

(2)__ARM__GPU__宏
如果使用的是ARM gpu，请打开__ARM__GPU__宏，其它gpu则关闭此宏

注意：
(1)从FBO直接导出来的渲染结果，图片是颠倒的。
如果应用场景是2D渲染,可以通过调整纹理坐标来解决颠倒的问题；

但如果应用场景是3D渲染必须要进行进一步加工，将其翻转180度。
可以使用G2D来旋转，也可以将本次渲染结果作为纹理，用gpu来进行一次渲染180度的渲染

(2)在开始渲染之前，应保持一个良好的习惯：重新绑定缓冲区、纹理、FRAMEBUFFER等，
特别是FBO，因为在创建和设置FBO时，上下文的纹理是有发生变化的，如果没有及时重新绑定纹理，
极易出错。

(3)在开始设置uniform变量时，要保持一个良好的习惯：激活着色器程序，如果没有在设置uniform变量前激活着色器程序，
则设置无效。

(4)纹理Target问题：如果上下文使用的都是GL_TEXTURE_EXTERNAL_OES纹理，则我们设置相关的纹理时，必须都用GL_TEXTURE_EXTERNAL_OES，
不能使用GL_Texture_2D

(5)背景色的设置(即glClear())必须要在FBO创建之后设置，否则无效

(6)同时采用pbuffer和FBO，使用glSwapBuffer无效； 只采用FBO，不用pbuffer，使用glSwapBuffer会将FBO的内容移除
