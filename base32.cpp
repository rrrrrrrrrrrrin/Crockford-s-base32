#include <iostream>
#include <fstream>
#include <cstring>  // For strcmp()

// Encodes data (inStr) with length (len byte) into outStr
// Returns the number of encoded bytes
int base32Encode(const char inStr[], int len, char outStr[]) {
    const char CrockfordBase32Alphabet[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
    int buffer = 0;  // Put bits here
    int bitsCnt = 0;
    int outIndex = 0;

    // Go through each byte
    for (int i = 0; i < len; ++i) {
        buffer = (buffer << 8) | static_cast<unsigned char>(inStr[i]);
        bitsCnt += 8;

        // Chunks of 5 bits
        while (bitsCnt >= 5) {
            int index = (buffer >> (bitsCnt - 5)) & 0x1F;
            outStr[outIndex++] = CrockfordBase32Alphabet[index];
            bitsCnt -= 5;
        }
    }

    // Pad with zeros to form the final 5-bit group if there are remaining bits
    if (bitsCnt > 0) {
        int index = (buffer << (5 - bitsCnt)) & 0x1F;
        outStr[outIndex++] = CrockfordBase32Alphabet[index];
    }

    // Pad with '=' until output length (outIndex) mod 8 is 0
    while (outIndex % 8 != 0) {
        outStr[outIndex++] = '=';
    }

    return outIndex;

}

unsigned char base32Code(char ch) {
    // Return a numeric value of a char
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }

    char character = static_cast<char>(toupper(ch));
    if ((character == 'I') || (character == 'L')) {
        return 1;
    }
    if (character == 'O') {
        return 0;
    }

    const char CrockfordBase32Alphabet[] = "ABCDEFGHJKMNPQRSTVWXYZ";
    if (character >= 'A' && character <= 'Z' && character != 'U') {
        // Excludes 'U'
        for (int i = 0; i < 23; ++i) {  // 23 is the length of letter alphabet part
            if (character == CrockfordBase32Alphabet[i]) {
                return i + 10;
            }
        }
    }
    return 32;
}

bool isBase32(char ch) {
    return base32Code(ch) < 32;
}

/*
    Decode a group of 8 (or less) base32 chars (len)
    Each char encodes 5 bits
    This function returns the numbers of decoded bytes
*/

int base32Decode(char inStr[], int len, char outStr[]) {
    int bits = 0;
    unsigned long long collectedBits = 0;  // Accumulated bits
    int outIndex = 0;

    // Collect bits from each character
    for (int i = 0; i < len; i++) {
        unsigned char value = base32Code(inStr[i]);
        if (!(value < 32)) {
            std::cout << "Invalid character in source file" << std::endl;
            return 0;
        }
        collectedBits = (collectedBits << 5) | value;  // Shifting to left by 5 bits is like multiplying collectedBits by 2^5 (or 32) in binary
        // Bitwise OR with value combines the alredy shifted collectedBits with the new 5-bit value
        // i.e. appends the 5 bits from value into the lower positions (to the right) of collectedBits
        bits += 5;
    }

    // Construct bytes (8 chars) from the collected bits
    while (bits >= 8) {
        outStr[outIndex++] = static_cast<char>((collectedBits >> (bits - 8)) & 0xFF);  // Discard the lower byte by shifting it by (bits) (if bits is 8
                                                                    // or just shift by (bits) to the right
                                                                    // mask the result with & 0xFF (1111 1111) which keeps the rightmost byte 
        bits -= 8;
    }

    return outIndex;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "Use for encoding / decoding:\n";
        std::cout << "\t" << argv[0] << " -e / -d source_file destination_file" << std::endl;
        return 0;
    }

    std::ifstream inFile(argv[2], std::ios_base::binary);
    if (!inFile) {
        std::cout << "Cannot open file" << argv[2] << std::endl;
        return 0;
    }

    std::ofstream outFile(argv[3], std::ios_base::binary);
    if (!outFile) {
        std::cout << "Cannot open file" << argv[3] << std::endl;
        inFile.close();
        return 0;
    }

    if (strcmp(argv[1], "-e") == 0) {
        const int chunk_size = 20;  // Reading from the input file in chunks (blocks)
        char inStr[5 * chunk_size];  // Input buffer: chunk size will be 5*chunk_size bytes
        char outStr[8 * chunk_size];  // Because in Base32 from 5 bytes (8 chars) we get 8 chars from base32,
        // the output (buffer) is a chunk 8*block_size bytes size

        while (inFile.peek() != EOF) {  // std::istream::peek returns a next character in the input sequence 
            inFile.read(inStr, static_cast<std::streamsize>(5) * chunk_size); // Read data as a chunk
            int readBytes = static_cast<int>(inFile.gcount());  // Actual amount of read bytes
            readBytes = base32Encode(inStr, readBytes, outStr);  // outIndex
            outFile.write(outStr, readBytes);
        }
    }
    else {  // if (char(argv[1]) == '-d')
        char inStr[8];  // Buffer for one group (8 bits) of encoded characters
        char outStr[5]; // Buffer for decoded bytes
        int cnt = 0;

        int ch;
        while ((ch = inFile.get()) != EOF) {
            char character = static_cast<char>(ch);

            // Checking if the symbol is from Crockford's Base 32
            // Skip if it isn't
            if (isBase32(character)) {
                inStr[cnt++] = character;
                if (cnt == 8) {
                    // Each 8 chars from base32 are transformed into 5 bytes
                    // Then these 5 bytes are transformed to their solid UTF-8 equivalent (in base32Decode)
                    int decodedBytes = base32Decode(inStr, 8, outStr);  // outIndex = amount of decoded bytes
                    outFile.write(outStr, decodedBytes);
                    cnt = 0;
                }
            }
        }

        // Deal with remaining chars in the last group (inGroup)
        if (cnt > 0) {
            int decodedBytes = base32Decode(inStr, cnt, outStr);  // outIndex = amount of decoded bytes
            outFile.write(outStr, decodedBytes);
        }
    }
    inFile.close();
    outFile.close();
    return 0;
}
