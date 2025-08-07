import whisper

# 가장 작은 모델 로드
model = whisper.load_model("tiny")

# 음성 파일 디코딩
result = model.transcribe("en.wav", language="en")

# 결과 출력
print(result["text"])
