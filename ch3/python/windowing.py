import numpy as np
import soundfile as sf

# 설정
FRAME_MS = 10

# Hanning 창 생성 함수
def hanning(N):
    return 0.5 - 0.5 * np.cos(2.0 * np.pi * np.arange(N) / (N - 1))

# WAV 읽기
data, sample_rate = sf.read("../../resource/woman_voice.wav", dtype='float32')
if data.ndim > 1:  # 스테레오 → 모노
    data = data.mean(axis=1)

# 프레임 관련 계산
samples_per_frame = int(sample_rate * FRAME_MS / 1000)
hop_size = samples_per_frame  # 50% overlap을 위해 frame_len*2 중 절반씩 처리
win_size = samples_per_frame * 2
win = hanning(win_size)

# zero-padding (마지막 프레임까지 처리 가능하게)
pad_len = (len(data) + hop_size - 1) // hop_size * hop_size
data_padded = np.pad(data, (0, pad_len - len(data)), mode='constant')

# 프레임 분할 (2배 길이로 → 이전+현재 결합 형태)
frames = np.lib.stride_tricks.sliding_window_view(data_padded, win_size)[::samples_per_frame]

# 윈도잉
windowed = frames * win

# Overlap-Add: 윈도우 시작 부분만 합산
# 출력 길이 계산
output_len = (len(frames) - 1) * hop_size + win_size
output = np.zeros(output_len, dtype=np.float32)

# NumPy 방식 OLA
for i, frame in enumerate(windowed):
    output[i * hop_size : i * hop_size + win_size] += frame

# 클리핑 방지
output = np.clip(output, -1.0, 1.0)

# 저장
sf.write("woman_voice_syn.wav", output, sample_rate)
print("완료: woman_voice_syn.wav")
