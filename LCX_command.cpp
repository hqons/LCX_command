#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <filesystem>
#include "./LCX/LCX.cpp"
#include <termios.h>
#include <unistd.h>

// LCX操作函数
unsigned char LCXOperation(unsigned char byte, unsigned char key, int D_E, unsigned char IV);
void generateSBox(unsigned char key);
extern unsigned char SBox[256];     // 声明 SBox
extern unsigned char InvSBox[256];  // 声明 InvSBox

unsigned char generateRandomChar() {
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (!urandom) {
        std::cerr << "Failed to open /dev/urandom" << std::endl;
        exit(1);
    }

    unsigned char randomChar;
    urandom.read(reinterpret_cast<char*>(&randomChar), sizeof(randomChar));
    return randomChar;
}

// LCX加密/解密函数（对称加密）
void LCXEncryptDecrypt(std::vector<unsigned char> &data, const std::vector<unsigned char> &password, int D_E, unsigned char IV) {
    size_t passwordLength = password.size();
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = LCXOperation(data[i], password[i % passwordLength], D_E, IV);
    }
}
unsigned int stableHash(const std::string &pin) {
    unsigned int h = 0;
    for (char c : pin) {
        h = (h * 131) + static_cast<unsigned char>(c);  // BKDRHash
    }
    return h;
}

std::vector<unsigned char> derivePasswordFromPIN(const std::string &pin, size_t length = 16) {
    std::vector<unsigned char> password(length);
    unsigned int seed = stableHash(pin);
    std::mt19937 gen(seed);  // 局部 RNG，互不干扰
    for (size_t i = 0; i < length; ++i)
        password[i] = gen() % 256;
    

    return password;
}



// 读取文件
std::vector<unsigned char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "无法打开文件 " << filename << std::endl;
        exit(1);
    }

    return std::vector<unsigned char>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

// 写入文件
void writeFile(const std::string &filename, const std::vector<unsigned char> &data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "无法写入文件 " << filename << std::endl;
        exit(1);
    }

    file.write(reinterpret_cast<const char *>(data.data()), data.size());
}

std::string getHiddenPIN(const std::string& prompt = "请输入PIN码: ") {
    std::string pin;
    std::cout << prompt;

    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::getline(std::cin, pin);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    std::cout << std::endl;
    return pin;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage:\n";
        std::cout << "  Encrypt: " << argv[0] << " encrypt <file_path>\n";
        std::cout << "  Decrypt: " << argv[0] << " decrypt <file_path>\n";
        return 1;
    }

    std::string mode = argv[1];
    std::string filename = argv[2];
    std::filesystem::path filePath(filename);
    std::string directory = filePath.parent_path().empty() ? "." : filePath.parent_path().string();

    if (mode == "encrypt"||mode == "e") {
        std::vector<unsigned char> fileContent = readFile(filename);
        std::string pin = getHiddenPIN("Please enter PIN (recommended 4~12 digits): ");
        std::vector<unsigned char> password = derivePasswordFromPIN(pin);
        unsigned char IV = generateRandomChar();
        unsigned int salt_pos = stableHash(pin) % (fileContent.size() + 1); // 防越界
        

        generateSBox(password[5] ^ password[12]);
        LCXEncryptDecrypt(fileContent, password, 0, IV);
        fileContent.insert(fileContent.begin() + salt_pos, IV);
        
        std::string encryptedFilename = directory + "/encrypted_" + filePath.filename().string();
        writeFile(encryptedFilename, fileContent);
        std::cout << "Encryption complete. File saved as " << encryptedFilename << std::endl;
    }
    else if (mode == "decrypt"||mode=="d") {
        std::vector<unsigned char> fileContent = readFile(filename);
        std::string pin = getHiddenPIN("Please enter PIN: ");
        std::vector<unsigned char> password = derivePasswordFromPIN(pin);

        unsigned int salt_pos = stableHash(pin) % (fileContent.size());
        unsigned char IV = fileContent[salt_pos];
        fileContent.erase(fileContent.begin() + salt_pos);

        generateSBox(password[5] ^ password[12]);
        LCXEncryptDecrypt(fileContent, password, 1, IV);

        std::string decryptedFilename = directory + "/decrypted_" + filePath.filename().string();
        writeFile(decryptedFilename, fileContent);
        std::cout << "Decryption complete. File saved as " << decryptedFilename << std::endl;
    }
    else {
        std::cerr << "Invalid operation. Please use encrypt or decrypt.\n";
        return 1;
    }

    std::fill(std::begin(SBox), std::end(SBox), 0);
    std::fill(std::begin(InvSBox), std::end(InvSBox), 0);
    return 0;
}