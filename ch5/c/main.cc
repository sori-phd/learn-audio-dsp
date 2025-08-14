#define _USE_MATH_DEFINES
#pragma warning(disable: 4146)

#include <iostream>
#include <vector>
#include "wavreader.h"
#include "wavwriter.h"

#include "api/audio/builtin_audio_processing_builder.h"
#include "api/environment/environment_factory.h"
#include "api/audio/audio_processing.h"
#include "modules/audio_processing/vad/voice_activity_detector.h"

#define FRAME_MS 10

int main() {
    int format, channels, sample_rate, bits_per_sample;
    unsigned int data_length;

    // Mic input
    void* input = wav_read_open("woman_voice.wav");
    if (!input) {
        std::cerr << "Cannot open input WAV file.\n";
        return 1;
    }

    // Reference (far-end) input
    void* ref_input = wav_read_open("woman_voice_ref.wav");
    if (!ref_input) {
        std::cerr << "Cannot open reference WAV file.\n";
        wav_read_close(input);
        return 1;
    }

    int res = wav_get_header(input, &format, &channels, &sample_rate, &bits_per_sample, &data_length);
    if (res == -1) {
        std::cerr << "Failed to read WAV header.\n";
        wav_read_close(input);
        wav_read_close(ref_input);
        return 1;
    }

    void* output = wav_write_open("woman_voice_syn.wav", sample_rate, bits_per_sample, channels);
    if (!output) {
        std::cerr << "Cannot open output WAV file.\n";
        wav_read_close(input);
        wav_read_close(ref_input);
        return 1;
    }

    if (bits_per_sample != 16) {
        std::cerr << "Only 16-bit PCM WAV supported.\n";
        wav_read_close(input);
        wav_read_close(ref_input);
        wav_write_close(output);
        return 1;
    }

    unsigned int samples_per_frame = sample_rate / 1000 * FRAME_MS;
    unsigned int samples = data_length / (bits_per_sample / 8);
    unsigned int total_frames = samples / samples_per_frame;

    // AudioProcessing configuration
    webrtc::AudioProcessing::Config ap_config;
    ap_config.echo_canceller.enabled = true;
    ap_config.echo_canceller.mobile_mode = false;

    ap_config.gain_controller1.enabled = true;
    ap_config.gain_controller1.mode = webrtc::AudioProcessing::Config::GainController1::kAdaptiveAnalog;
    ap_config.gain_controller1.target_level_dbfs = 3;
    ap_config.gain_controller1.compression_gain_db = 9;
    ap_config.gain_controller1.enable_limiter = true;

    ap_config.noise_suppression.enabled = true;
    ap_config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kHigh;

    webrtc::scoped_refptr<webrtc::AudioProcessing> apm =
        webrtc::BuiltinAudioProcessingBuilder(ap_config).Build(webrtc::CreateEnvironment());

    webrtc::StreamConfig stream_config(sample_rate, channels);
    webrtc::StreamConfig reference_config(sample_rate, channels);

    std::vector<int16_t> input_frame(samples_per_frame * channels);
    std::vector<int16_t> output_frame(samples_per_frame * channels);

    std::vector<float> reference_frame(samples_per_frame * channels);
    std::vector<const float*> channel_ptrs(channels);

    webrtc::VoiceActivityDetector vad;

    for (int ch = 0; ch < channels; ++ch)
        channel_ptrs[ch] = reference_frame.data() + ch * samples_per_frame;

    for (unsigned int f = 0; f < total_frames; ++f) {
        // Mic input
        wav_read_data(input, reinterpret_cast<unsigned char*>(input_frame.data()), samples_per_frame * channels * sizeof(int16_t));

        // Reference input (float conversion)
        std::vector<int16_t> ref_int16(samples_per_frame * channels);
        wav_read_data(ref_input, reinterpret_cast<unsigned char*>(ref_int16.data()), samples_per_frame * channels * sizeof(int16_t));
        for (size_t i = 0; i < ref_int16.size(); ++i)
            reference_frame[i] = ref_int16[i] / 32768.0f; // int16 -> float

        // Analyze reference for AEC
        apm->AnalyzeReverseStream(channel_ptrs.data(), reference_config);

        // Process mic input with AEC + NS + AGC
        apm->ProcessStream(input_frame.data(), stream_config, stream_config, output_frame.data());
        
        // VAD
        vad.ProcessChunk(output_frame.data(), samples_per_frame, sample_rate);
        const auto& probabilities = vad.chunkwise_voice_probabilities();
        std::cout << "Frame " << f + 1 << "/" << total_frames
		    << " - Voice Probability: " << vad.last_voice_probability() << "\n";

        // Write output
        wav_write_data(output, reinterpret_cast<unsigned char*>(output_frame.data()), samples_per_frame * channels * sizeof(int16_t));
    }

    wav_read_close(input);
    wav_read_close(ref_input);
    wav_write_close(output);

    std::cout << "Processing finished: woman_voice_syn.wav\n";
    return 0;
}
