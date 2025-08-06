#include <iostream>
#include <vector>
#include <random>

unsigned char SBox[256];
unsigned char InvSBox[256];
// 使用密钥生成 SBox
void generateSBox(unsigned char key)
{
    // 使用 std::random_device 和 std::mt19937 来增强随机性
    std::random_device rd;
    std::mt19937 gen(rd());                                   // 生成随机数生成器
    std::uniform_int_distribution<unsigned char> dis(0, 255); // 随机分布

    // 初始化 SBox 为 0 到 255 的顺序排列
    for (int i = 0; i < 256; ++i)
    {
        SBox[i] = static_cast<unsigned char>(i);
    }

    // 使用密钥初始化随机数生成器的种子
    gen.seed(key);

    // 使用 Fisher-Yates 洗牌算法对 SBox 进行随机排列
    for (int i = 255; i > 0; --i)
    {
        int j = dis(gen) % (i + 1); // 生成 0 到 i 之间的随机索引
        std::swap(SBox[i], SBox[j]);
    }

    // 创建 InvSBox，存储每个字节在 SBox 中的位置
    for (int i = 0; i < 256; ++i)
    {
        InvSBox[SBox[i]] = static_cast<unsigned char>(i);
    }

}

// LCX操作函数
unsigned char LCXOperation(unsigned char byte, unsigned char key, int D_E,unsigned char IV)
{
    if (D_E == 0)
    {
        byte = SBox[byte]; // 加密时使用 SBox
        return byte ^ key^IV; // 执行异或操作
    }
    else
    {
        byte = InvSBox[byte^key^IV]; // 解密时使用 InvSBox
        return byte;
    }

    
}
