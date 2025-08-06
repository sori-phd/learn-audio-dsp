#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#pragma pack(push, 1)  // 구조체 정렬 없이 메모리에 그대로 저장

typedef struct {
    char chunkID[4];      // "RIFF"
    uint32_t chunkSize;
    char format[4];       // "WAVE"
} RIFFHeader;

typedef struct {
    char subchunk1ID[4];  // "fmt "
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} FmtSubchunk;

typedef struct {
    char subchunk2ID[4];  // "data"
    uint32_t subchunk2Size;
    // 뒤에는 PCM 데이터
} DataSubchunk;

#pragma pack(pop)

int main() {
    FILE *input = fopen("woman_voice.wav", "rb");
    FILE *output = fopen("woman_voice_dup.wav", "wb");

    if (!input || !output) {
        perror("파일 열기 실패");
        return 1;
    }

    RIFFHeader riff;
    FmtSubchunk fmt;
    DataSubchunk data;

    // RIFF 헤더 읽기
    fread(&riff, sizeof(RIFFHeader), 1, input);

    if (strncmp(riff.chunkID, "RIFF", 4) != 0 || strncmp(riff.format, "WAVE", 4) != 0) {
        printf("유효한 WAV 파일이 아닙니다.\n");
        fclose(input);
        fclose(output);
        return 1;
    }

    // fmt 청크 읽기
    fread(&fmt, sizeof(FmtSubchunk), 1, input);

    if (strncmp(fmt.subchunk1ID, "fmt ", 4) != 0) {
        printf("fmt 청크가 없습니다.\n");
        fclose(input);
        fclose(output);
        return 1;
    }

    // data 청크 찾기 (중간에 "LIST" 등 다른 청크가 있을 수 있음)
    while (1) {
        fread(&data, sizeof(data.subchunk2ID) + sizeof(data.subchunk2Size), 1, input);
        if (strncmp(data.subchunk2ID, "data", 4) == 0) {
            break;
        } else {
            // 스킵
            fseek(input, data.subchunk2Size, SEEK_CUR);
        }
    }

    // 헤더 정보 출력
    printf("샘플링 레이트: %u Hz\n", fmt.sampleRate);
    printf("비트 깊이: %u bits\n", fmt.bitsPerSample);
    printf("채널 수: %u\n", fmt.numChannels);
    printf("오디오 포맷: %s\n", (fmt.audioFormat == 1) ? "PCM" : "비 PCM");

    // 복사: 파일 처음으로 이동
    fseek(input, 0, SEEK_SET);

    // 파일 전체를 복사
    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), input)) > 0) {
        fwrite(buffer, 1, bytesRead, output);
    }

    fclose(input);
    fclose(output);

    printf("복사 완료: woman_voice_dup.wav\n");
    return 0;
}
