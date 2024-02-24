#include<iostream>
#include<cstdint>
#include <fstream>
#include <chrono>
#include <random>
#include <cstring>

#include "font.h"
#include "sdl_platform.h"


const unsigned int VIDEO_HEIGHT = 32;
const unsigned int VIDEO_WIDTH = 64;

struct CHIP8 {
    CHIP8();
    uint8_t registers[16]{};
    uint8_t memory[4096]{};
    uint16_t index{};
    uint16_t pc{};
    uint16_t stack[16]{};
    uint8_t sp{};
    uint8_t delayTimer{};
    uint8_t soundTimer{};
    uint8_t keypad[16]{};
    uint32_t video[64 * 32]{};
    uint16_t opcode;

    const unsigned int START_ADDRESS = 0x200;
    const unsigned int FONTSET_START_ADDRESS = 0x50;

    void LoadROM(char const* filename){
        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        if(file.is_open()){
            //Create a buffer to hold contents of ROM
            std::streampos size = file.tellg();
            char* buffer = new char[size];

            //Fill the buffer with everything in the ROM
            file.seekg(0, std::ios::beg);
            file.read(buffer, size);
            file.close();

            //Load ROM contnets into CHIP8 memory
            for(long i = 0; i < size; i++){
                memory[START_ADDRESS + i] = buffer[i];
            }

            //Free buffer pointer
            delete[] buffer;
        }
    }

    std::default_random_engine randGen;
    std::uniform_int_distribution<uint8_t> randByte;

    //Instructions
    void INS_CLS_00E0(){
        std::memset(video, 0, sizeof(video));
    }

    void INS_RET_00EE(){
        sp--;
        pc = stack[sp];
    }

    void INS_JP_1nnn(){
        uint16_t address = opcode & 0x0FFFu;
        pc = address;
    }

    void INS_CALL_2nnn(){
        uint16_t address = opcode & 0x0FFFu;
        stack[sp] = pc;
        sp++;
        pc = address;
    }

    void INS_SE_3xkk(){
        uint8_t Vx = (opcode & 0xF00u) >> 8u;
        uint8_t byte = opcode & 0x00FFu;

        if(registers[Vx] == byte){
            pc += 2;
        }
    }

    void INS_SNE_4xkk(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t byte = opcode & 0x00FFu;

        if(registers[Vx] != byte){
            pc += 2;
        }
    }

    void INS_SE_5xy0(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;

        if(registers[Vx] == registers[Vy]){
            pc += 2;
        }
    }

    void INS_LD_6xkk(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t byte = opcode & 0x00FFu;

        registers[Vx] = byte;
    }

    void INS_ADD_7xkk(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t byte = opcode & 0x00FFu;

        registers[Vx] += byte;
    }

    void INS_LD_8xy0(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;

        registers[Vx] = registers[Vy];
    }

    void INS_OR_8xy1(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;

        registers[Vx] |= registers[Vy];
    }

    void INS_AND_8xy2(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;

        registers[Vx] &= registers[Vy];
    }

    void INS_XOR_8xy3(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;

        registers[Vx] ^= registers[Vy];
    }

    void INS_ADD_8xy4(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;

        uint16_t sum = registers[Vx] + registers[Vy];

        if(sum > 255u){
            registers[0xF] = 1;
        } else {
            registers[0xF] = 0;
        }

        registers[Vx] = sum & 0xFFu;
    }

    void INS_SUB_8xy5(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;

        if (registers[Vx] > registers[Vy]){
            registers[0xF] = 1;
        } else {
            registers[0xF] = 0;
        }

        registers[Vx] -= registers[Vy];
    }

    void INS_SHR_8xy6(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;

        registers[0xF] = (registers[Vx] & 0x1u);
        registers[Vx] >>= 1;
    }

    void INS_SUBN_8xy7(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    
        if(registers[Vy] > registers[Vx]){
            registers[0xF] = 1;
        } else {
            registers[0xF] = 0;
        }

        registers[Vx] = registers[Vy] - registers[Vx];
    }

    void INS_SHL_8xyE(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;

        registers[0xF] = (registers[Vx] & 0x80u) >> 7u;
        registers[Vx] <<= 1;
    }

    void INS_SNE_9xy0(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00u) >> 4u;

        if(registers[Vx] != registers[Vy]){
            pc += 2;
        }
    }

    void INS_LD_Annn(){
        uint16_t address = opcode & 0x0FFFu;

        index = address;
    }

    void INS_JP_Bnnn(){
        uint16_t address = opcode & 0x0FFFu;

        pc = registers[0] + address;
    }

    void INS_RND_Cxkk(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t byte = opcode & 0x00FFu;

        registers[Vx] = randByte(randGen) & byte;
    }

    void INS_DRW_Dxyn(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;
        uint8_t height = opcode & 0x000Fu;

        //The ol' wrap around
        uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
        uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

        registers[0xF] = 0;

        for(unsigned int row = 0; row < height; row++){
            uint8_t spriteByte = memory[index + row];

            for(unsigned int col = 0; col < 8; col++){
                uint8_t spritePixel = spriteByte & (0x80u >> col);
                uint32_t* screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

                if(spritePixel){
                    if(*screenPixel == 0xFFFFFFFF){
                        registers[0xF] = 1;
                    }

                    *screenPixel ^= 0xFFFFFFFF;
                }
            
            }
        }
    }

    void INS_SKP_Ex9E(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t key = registers[Vx];

        if(keypad[key]){
            pc += 2;
        }
    }

    void INS_SKNP_ExA1(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t key = registers[Vx];

        if(!keypad[key]){
            pc += 2;
        }
    }

    void INS_LD_Fx07(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
 
        registers[Vx] = delayTimer;
    }

    void INS_LD_Fx0A(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;

        if (keypad[0])
        {
            registers[Vx] = 0;
        }
        else if (keypad[1])
        {
            registers[Vx] = 1;
        }
        else if (keypad[2])
        {
            registers[Vx] = 2;
        }
        else if (keypad[3])
        {
            registers[Vx] = 3;
        }
        else if (keypad[4])
        {
            registers[Vx] = 4;
        }
        else if (keypad[5])
        {
            registers[Vx] = 5;
        }
        else if (keypad[6])
        {
            registers[Vx] = 6;
        }
        else if (keypad[7])
        {
            registers[Vx] = 7;
        }
        else if (keypad[8])
        {
            registers[Vx] = 8;
        }
        else if (keypad[9])
        {
            registers[Vx] = 9;
        }
        else if (keypad[10])
        {
            registers[Vx] = 10;
        }
        else if (keypad[11])
        {
            registers[Vx] = 11;
        }
        else if (keypad[12])
        {
            registers[Vx] = 12;
        }
        else if (keypad[13])
        {
            registers[Vx] = 13;
        }
        else if (keypad[14])
        {
            registers[Vx] = 14;
        }
        else if (keypad[15])
        {
            registers[Vx] = 15;
        }
        else
        {
            pc -= 2;
        }
    }

    void INS_LD_Fx15(){
        uint8_t Vx = (opcode * 0x0F00u) >> 8u;
        delayTimer = registers[Vx];
    }

    void INS_LD_Fx18(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        soundTimer = registers[Vx];
    }

    void INS_ADD_Fx1E(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        index += registers[Vx];
    }

    void INS_LD_Fx29(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t digit = registers[Vx];
        index = FONTSET_START_ADDRESS + (5 * digit);
    }

    void INS_LD_Fx33(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	    uint8_t value = registers[Vx];

        memory[index + 2] = value % 10;
        value /= 10;

        memory[index + 1] = value % 10;
        value /= 10;

        memory[index] = value % 10;
    }

    void INS_LD_Fx55(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;

        for(uint8_t i = 0; i <= Vx; i++){
            memory[index + i] = registers[i];
        }
    }

    void INS_LD_Fx65(){
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;

        for(uint8_t i = 0; i <= Vx; i++){
            registers[i] = memory[index + i];
        }
    }

    typedef void (CHIP8::*Chip8Func)();
	Chip8Func table[0xF + 1];
	Chip8Func table0[0xE + 1];
	Chip8Func table8[0xE + 1];
	Chip8Func tableE[0xE + 1];
	Chip8Func tableF[0x65 + 1];

    void Table0()
	{
		((*this).*(table0[opcode & 0x000Fu]))();
	}

	void Table8()
	{
		((*this).*(table8[opcode & 0x000Fu]))();
	}

	void TableE()
	{
		((*this).*(tableE[opcode & 0x000Fu]))();
	}

	void TableF()
	{
		((*this).*(tableF[opcode & 0x00FFu]))();
	}

	void OP_NULL()
	{}

    //CPU Stuff
    void Cycle(){
        opcode = (memory[pc] << 8u) | memory[pc + 1];

        pc += 2;
        
        ((*this).*(table[(opcode & 0xF000u) >> 12u]))();

        if(delayTimer > 0){
            delayTimer--;
        }

        if(soundTimer > 0){
            soundTimer--;
        }
    }
};

//Constructor
CHIP8::CHIP8() : randGen(std::chrono::system_clock::now().time_since_epoch().count()){
    randByte = std::uniform_int_distribution<uint8_t>(0, 255U);
    pc = START_ADDRESS;

    //Load font into memory from 0x50
    for(unsigned int i = 0; i < FONTSET_SIZE; i++){
        memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }

    //Function ptr table for opcodes, instead of a switch statement - for cleaner code
    table[0x0] = &CHIP8::Table0;
    table[0x1] = &CHIP8::INS_JP_1nnn;
    table[0x2] = &CHIP8::INS_CALL_2nnn;
    table[0x3] = &CHIP8::INS_SE_3xkk;
    table[0x4] = &CHIP8::INS_SNE_4xkk;
    table[0x5] = &CHIP8::INS_SE_5xy0;
    table[0x6] = &CHIP8::INS_LD_6xkk;
    table[0x7] = &CHIP8::INS_ADD_7xkk;
    table[0x8] = &CHIP8::Table8;
    table[0x9] = &CHIP8::INS_SNE_9xy0;
    table[0xA] = &CHIP8::INS_LD_Annn;
    table[0xB] = &CHIP8::INS_JP_Bnnn;
    table[0xC] = &CHIP8::INS_RND_Cxkk;
    table[0xD] = &CHIP8::INS_DRW_Dxyn;
    table[0xE] = &CHIP8::TableE;
    table[0xF] = &CHIP8::TableF;

    for(size_t i = 0; i <= 0xE; i++){
        table0[i] = &CHIP8::OP_NULL;
        table8[i] = &CHIP8::OP_NULL;
        tableE[i] = &CHIP8::OP_NULL;
    }

    table0[0x0] = &CHIP8::INS_CLS_00E0;
    table0[0xE] = &CHIP8::INS_RET_00EE;
    
    table8[0x0] = &CHIP8::INS_LD_8xy0;
    table8[0x1] = &CHIP8::INS_OR_8xy1;
    table8[0x2] = &CHIP8::INS_OR_8xy1;
    table8[0x3] = &CHIP8::INS_XOR_8xy3;
    table8[0x4] = &CHIP8::INS_ADD_8xy4;
    table8[0x5] = &CHIP8::INS_SUB_8xy5;
    table8[0x6] = &CHIP8::INS_SHR_8xy6;
    table8[0x7] = &CHIP8::INS_SUBN_8xy7;
    table8[0xE] = &CHIP8::INS_SHL_8xyE;

    tableE[0x1] = &CHIP8::INS_SKNP_ExA1;
    tableE[0xE] = &CHIP8::INS_SKP_Ex9E;

    for(size_t i = 0; i <= 0x65; i++){
        tableF[i] = &CHIP8::OP_NULL;
    }

    tableF[0x07] = &CHIP8::INS_LD_Fx07;
	tableF[0x0A] = &CHIP8::INS_LD_Fx0A;
	tableF[0x15] = &CHIP8::INS_LD_Fx15;
	tableF[0x18] = &CHIP8::INS_LD_Fx18;
	tableF[0x1E] = &CHIP8::INS_ADD_Fx1E;
	tableF[0x29] = &CHIP8::INS_LD_Fx29;
	tableF[0x33] = &CHIP8::INS_LD_Fx33;
	tableF[0x55] = &CHIP8::INS_LD_Fx55;
	tableF[0x65] = &CHIP8::INS_LD_Fx65;

}

int main(int argc, char* argv[]){
    std::cout << "Build Success!" << '\n';

    if(argc != 4){
        std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>\n";
        std::exit(EXIT_FAILURE);
    }

    int videoScale = std::stoi(argv[1]);
    int cycleDelay = std::stoi(argv[2]);
    char const* romFilename = argv[3];

    Platform platform("CHIP8 EMU 9922", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);
    
    CHIP8 chip8;
    chip8.LoadROM(romFilename);

    int videoPitch = sizeof(chip8.video[0]) * VIDEO_WIDTH;

	auto lastCycleTime = std::chrono::high_resolution_clock::now();
	bool quit = false;

	while (!quit)
	{
		quit = platform.ProcessInput(chip8.keypad);

		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();

		if (dt > cycleDelay)
		{
			lastCycleTime = currentTime;

			chip8.Cycle();

			platform.Update(chip8.video, videoPitch);
		}
	}

    return 0;
}