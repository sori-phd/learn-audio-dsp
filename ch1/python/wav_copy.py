import struct
import shutil

def read_wav_header(file):
    # RIFF 헤더 읽기
    riff_header = file.read(12)
    chunk_id, chunk_size, format = struct.unpack('<4sI4s', riff_header)
    if chunk_id != b'RIFF' or format != b'WAVE':
        raise ValueError("유효한 WAV 파일이 아닙니다.")

    # fmt 서브청크 읽기
    fmt_header = file.read(24)
    subchunk1_id, subchunk1_size, audio_format, num_channels, sample_rate, \
    byte_rate, block_align, bits_per_sample = struct.unpack('<4sIHHIIHH', fmt_header)

    if subchunk1_id != b'fmt ':
        raise ValueError("fmt 청크가 없습니다.")

    # data 청크 찾기
    while True:
        subchunk2_header = file.read(8)
        if len(subchunk2_header) < 8:
            raise ValueError("data 청크를 찾을 수 없습니다.")
        subchunk2_id, subchunk2_size = struct.unpack('<4sI', subchunk2_header)
        if subchunk2_id == b'data':
            break
        file.seek(subchunk2_size, 1)  # skip this chunk

    # 헤더 정보 출력
    print(f"샘플링 레이트: {sample_rate} Hz")
    print(f"비트 깊이: {bits_per_sample} bits")
    print(f"채널 수: {num_channels}")
    print(f"오디오 포맷: {'PCM' if audio_format == 1 else '비 PCM'}")

def copy_wav_file(src_path, dst_path):
    with open(src_path, 'rb') as f:
        read_wav_header(f)

    # 전체 파일 복사
    shutil.copyfile(src_path, dst_path)
    print(f"복사 완료: {dst_path}")

if __name__ == '__main__':
    copy_wav_file("../../resource/woman_voice.wav", "woman_voice_dup.wav")

