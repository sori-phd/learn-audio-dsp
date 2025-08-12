#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "wavreader.h"
#include "wavwriter.h"

#include "muFFT/fft.h"

#define FRAME_MS 10
#define FLOAT_TO_SHORT_SCALE 32767.0f
#define SHORT_TO_FLOAT_SCALE (1.0f / 32768.0f)
#define FFT_SIZE 1024

float hanning(int n, int N) {
    return 0.5f - 0.5f * cosf(2.0f * M_PI * n / (N - 1));
}

int main() {
    int format, channels, sample_rate, bits_per_sample;
    unsigned int data_length;
    
    void *input = wav_read_open("woman_voice.wav");
    int res = wav_get_header(input, &format, &channels, &sample_rate, &bits_per_sample, &data_length);
    
    void *output = wav_write_open("woman_voice_syn.wav", sample_rate, bits_per_sample, channels);
    
    // 헤더 정보 출력
    printf("샘플링 레이트: %u Hz\n", sample_rate);
    printf("비트 깊이: %u bits\n", bits_per_sample);
    printf("채널 수: %u\n", channels);
    printf("오디오 포맷: %s\n", (format == 1) ? "PCM" : "비 PCM");
    
    if(bits_per_sample != 16 && bits_per_sample != 32) {
        printf("지원하지 않는 WAV 형태\n");
        return 1;
    }
    
    // 한 frame의 길이 및 전체 frame 수 계산
    unsigned int samples_per_frame = sample_rate / 1000 * FRAME_MS;
    unsigned int samples = data_length / bits_per_sample * 8;
    unsigned int total = samples / samples_per_frame;

    // hann window 생성
    int win_samples = samples_per_frame * 2;
    float *win_func = malloc(win_samples * sizeof(float));
    for (int i = 0; i < win_samples; i++)
        win_func[i] = hanning(i, win_samples);
    
    short *input_buffer = malloc(samples_per_frame * sizeof(short));
    short *output_buffer = malloc(samples_per_frame * sizeof(short));
    float *input_buffer_f = malloc(samples_per_frame * sizeof(float));
    float *output_buffer_f = malloc(samples_per_frame * sizeof(float));
    
    float *window_buffer = calloc(win_samples, sizeof(float));
    float *past_buffer = calloc(samples_per_frame, sizeof(float));
    float *past_window_buffer = calloc(samples_per_frame, sizeof(float));
    
    // AVX 사용시 segfault가 발생하는 경우가 있음 
    mufft_plan_1d *fft_plan = mufft_create_plan_1d_c2c(FFT_SIZE, MUFFT_FORWARD, MUFFT_FLAG_CPU_NO_AVX);
    mufft_plan_1d *ifft_plan = mufft_create_plan_1d_c2c(FFT_SIZE, MUFFT_INVERSE, MUFFT_FLAG_CPU_NO_AVX);

    float *fft_real = malloc(FFT_SIZE * 2 * sizeof(float));
    float *fft_complex = malloc(FFT_SIZE * 2 * sizeof(float));

    unsigned int current = 0;
    while (current++ < total) {
        if(bits_per_sample == 32) {
            wav_read_data(input, (unsigned char *)input_buffer_f, samples_per_frame * sizeof(float));
        }
        else {
            wav_read_data(input, (unsigned char *)input_buffer, samples_per_frame * sizeof(short));
            for(int i = 0 ; i < samples_per_frame; i++)
                input_buffer_f[i] = input_buffer[i] * SHORT_TO_FLOAT_SCALE;
        }
        
        // 50% overlap and add with hanning window
        for (int i = 0; i < samples_per_frame; i++) {
            window_buffer[i] = past_buffer[i] * win_func[i];
        }
        for (int i = 0; i < samples_per_frame; i++) {
            window_buffer[i + samples_per_frame] = input_buffer_f[i] * win_func[ i+ samples_per_frame];
        }
        
        // fft, ifft start -->
        memset(fft_real, 0, sizeof(float) * FFT_SIZE * 2);
        memset(fft_complex, 0, sizeof(float) * FFT_SIZE * 2);

        for(int i = 0; i < win_samples; i++){
            fft_real[i*2] = window_buffer[i];
        }

        mufft_execute_plan_1d(fft_plan, fft_complex, fft_real);
        mufft_execute_plan_1d(ifft_plan, fft_real, fft_complex);

        for(int i = 0; i < win_samples; i++){
            window_buffer[i] = fft_real[i*2] / FFT_SIZE;
        }
        // <-- fft, ifft end

        for (int i = 0; i < samples_per_frame; i++) {
            output_buffer_f[i] = window_buffer[i] + past_window_buffer[i];
        }
        
        // clamping
        for(int i = 0 ; i < samples_per_frame; i++){
            if(output_buffer_f[i] > 1.0)
                output_buffer_f[i] = 1.0;
            else if(output_buffer_f[i] < -1.0)
                output_buffer_f[i] = -1.0;
        }
        
        // 과거 값 저장
        memmove(past_window_buffer, &window_buffer[samples_per_frame], samples_per_frame* sizeof(float));
        memmove(past_buffer, input_buffer_f, samples_per_frame* sizeof(float));
        
        if(bits_per_sample == 32) {
            wav_write_data(output, (unsigned char *)output_buffer_f, samples_per_frame * sizeof(float));
        }
        else {
            for(int i = 0 ; i < samples_per_frame; i++)
                output_buffer[i] = output_buffer_f[i] * FLOAT_TO_SHORT_SCALE;
            wav_write_data(output, (unsigned char *)output_buffer, samples_per_frame * sizeof(short));
        }
    }

    wav_read_close(input);
    wav_write_close(output);
    free(win_func);
    free(input_buffer);
    free(output_buffer);
    free(input_buffer_f);
    free(output_buffer_f);
    mufft_free_plan_1d(fft_plan);
    mufft_free_plan_1d(ifft_plan);
    free(fft_real);
    free(fft_complex);

    printf("windowing 완료: woman_voice_syn.wav\n");
    return 0;
}
