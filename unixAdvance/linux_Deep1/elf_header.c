/***
 * 所谓不动笔墨不读书
 * 今天来好好坐坐一个笔记
 * 深入程序布局内部，增强应用控制能力
 * 
 * 探讨程序的静态布局与动态布局
 * 静态布局：指可执行文件在硬盘上的内部布局
 * 动态布局：程序被系统加载到内存之后的布局
 * 
 * 可执行文件里有什么？
 * 可执行文件包含了源文件编译生成的可执行指令和数据
 * Linux中，二进制可执行文件的标准格式叫做ELF 同时兼容可执行文件和可连接文件
 * 
 * ELF文件包含一个固定长度的文件头和多个可扩展的数据块
 * 
 * 文件头是整个可执行文件的总地图，描述了整个文件的组织结构。
 * 
 * 可扩展数据块分两类: 
 * 1. 链接视图下，数据块的单位是节(Section)，用多个节区头索引所有内容
 * 2. 执行视图下，数据块的单位是段(Segment)，用程序头(Program Header)索引所有的段
 * 
 * ***/

#include <stdio.h>
#include <stdlib.h>

static char static_data[16] = "I am Static Data";
static char raw_static_data[40960];
static const char const_data[16] = "I am Const Data";
// .rodata节区存储常量数据
// 程序运行时，.rodata节区会被加载到与.text节区相同的段内，而且该段的属性如Flags列所显示的，只有R和E没有W

// char *pc = (char*) const_data;
// *pc = 'x'; // 报错 没有修改权限 Segment Violation崩溃
int main(int args,char **argv){
    const int const_value = 100;
    int *ptr = (int*)&const_value;
    *ptr = 200; // 拥有修改权限

    printf("Message in Main\n");
    return 0;
}
/***
 * 组成部分之程序头和节区头
 * 使用readelf命令的-S和-L选项可以分别列出程序头和节区头的内容
 * 
 * Nr 是Section的编号 Name列是段名 Type列表示节区的类型
 * Address表示该节区占据的虚拟内存起始地址 Offset表示该节区在可执行文件中的存放位置
 * Size列说明该节区的大小
 * 
 * readelf -L elf_header 节区头信息
 * Offset列 每个段在文件中的起始地址 VirtAddr列 程序加载后占据的虚拟地址
 * 物理地址PhysAddr列 ...
 * 可以核对这些映射关系
 * 
 * ELF文件的细节结构
 * 可执行文件里有
 * 1. 首先64字节的ELF文件头，其中记录了该文件的类型、目标平台信息，而且索引了整个文件需要的节区头和段头信息
 * 2. 接下来8个程序头，每个56字节，占据了接下来的448字节
 * 3. 紧接着是多个节区，从第512字节开始，依次存放着从.interp到.shstrtab的26节区
 * 4. 从2632字节位置开始存放节区头
 * 5. 最后，从4488字节的位置开始，依次放置.symtab和.strtab节区，这两个节区分别占据了1704字节和644字节
 * 
 * 共6836字节
 * ***/

/**
 * 影响静态布局(节区信息)的因素
 * .text节区
 * 存储源文件编译生成的机器指令。所有的程序逻辑都放在这里
 * 仅有开发者写的程序才能影响
 * 
 * .data节区
 * 所有的全局和静态的已初始化变量会存放在这个节区中
 * 
 * .bss节区
 * 未初始化或初始化为0的全局和静态变量
 * static char raw_static_data[20960];
 * .bss节区在可执行文件中不占据任何空间，加载到内存之后才会被分配内存
 * .bssSiz = FileSiz - MemSiz
 * 该节区的设计初衷就是为了节省目标文件的存储空间。变量未被初始化，或者虽被初始化了
 * 值为0，就没必要浪费空间，再在目标文件中存储大量的0值
 * 
 * .got和.plt节区
 * 存储动态链接用到的全局入口表和跳转表
 * 
 * 用扩展属性执行节区
 * 除了变量类型会影响数据的存储节区以外，GCC还支持GUN C的一个扩展属性执行数据的存储节区
 * 
 * ***/
__attribute__((section("personal"))) char g_array[1024];
/**
 * 如此声明的全局或静态变量，不管有无初始化，都会在可执行文件中占据相应的存储空间，不能再享受.bss段所带来的节省存储空间的好处
 * ***/

/**
 * 控制程序的动态布局
 * 控制ELF文件中程序头的信息
 * 而这些信息由连接器(LD)负责组装
 * 所以控制了连接器的组装行为，即可控制程序的动态布局
 * 
 * 探索链接脚本的功能和用法
 * 确保数据不会意料之外的数组溢出或错误的指针修改
 * 
 * 当意外发生，数据应设置为只读
 * mprotect系统调用保护内容所占的内存页设置为写保护
 * 
 * 
 * 一个链接脚本中最重要的部分是SECTIONS块，里面定义了生成的节区的映射关系
 * 
 * 链接姐哦本中定义的标号只是用来标定一个地址，并不会为它准备额外的存储空间
 * 
 * 如果手上有某个英文版的应用可执行程序，如何对其进行汉化
 * 对相应的区进行修改，汉化
 * 
 * 若要将可执行文件中的某些资源提取出来，该如何写程序
 * 对rodata区的数据进行提取，相应的区的资源提取
 * 
 * 如何实现程序自检，也就是确认自己的代码段数据是没有被人暴力修改过？
 * 链接控制脚本 将逻辑错误尽快暴露 mprotect 写保护
 * 
 * 一旦程序被修改，使用mprotect改成写保护或只读
 * 
 * 如何保护保存再可执行文件中的资源或敏感数据？
 * 自定义保护区，改成只读mprotect
 * **/




